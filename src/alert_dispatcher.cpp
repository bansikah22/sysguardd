#include "sysguardd/alert_dispatcher.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace sysguardd {

// ---- Severity helpers -------------------------------------------------------

int AlertDispatcher::severity_level(const std::string& s) {
  if (s == "warning") return kLevelWarning;
  if (s == "critical") return kLevelCritical;
  return kLevelInfo;
}

// ---- JSON helper (local) ----------------------------------------------------

namespace {

std::string alert_json_escape(const std::string& value) {
  std::string out;
  out.reserve(value.size() * 2);
  for (unsigned char c : value) {
    if (c <= 0x1F) {
      char buf[7];
      snprintf(buf, sizeof(buf), "\\u%04x", c);
      out += buf;
      continue;
    }
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\x7F': out += "\\u007f"; break;
      default: out += static_cast<char>(c);
    }
  }
  return out;
}

// ---- HTTP POST sender -------------------------------------------------------

// Sends a plain HTTP/1.1 POST request over a TCP socket.
// Only http:// is supported (HTTPS requires a TLS library).
bool send_http_post(const std::string& url, const std::string& body) {
  if (url.size() < 7 || url.substr(0, 7) != "http://") {
    std::cerr << "alert: only http:// webhook URLs are supported\n";
    return false;
  }

  const std::string rest = url.substr(7);
  std::string host;
  std::string port_str = "80";
  std::string path = "/";

  const auto slash_pos = rest.find('/');
  const std::string hostport = (slash_pos == std::string::npos) ? rest : rest.substr(0, slash_pos);
  if (slash_pos != std::string::npos) {
    path = rest.substr(slash_pos);
  }

  const auto colon_pos = hostport.find(':');
  if (colon_pos == std::string::npos) {
    host = hostport;
  } else {
    host = hostport.substr(0, colon_pos);
    port_str = hostport.substr(colon_pos + 1);
    // Validate port is purely numeric (CWE-20)
    for (char c : port_str) {
      if (c < '0' || c > '9') {
        std::cerr << "alert: invalid port in webhook URL\n";
        return false;
      }
    }
  }

  if (host.empty()) {
    std::cerr << "alert: empty host in webhook URL\n";
    return false;
  }

  struct addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;

  struct addrinfo* res = nullptr;
  if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0 || res == nullptr) {
    std::cerr << "alert: failed to resolve webhook host: " << host << "\n";
    return false;
  }

  const int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    freeaddrinfo(res);
    std::cerr << "alert: socket creation failed\n";
    return false;
  }

  // 5-second timeouts prevent the worker from blocking indefinitely
  struct timeval tv{5, 0};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  if (connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
    freeaddrinfo(res);
    close(fd);
    std::cerr << "alert: failed to connect to webhook host: " << host << "\n";
    return false;
  }
  freeaddrinfo(res);

  std::ostringstream req;
  req << "POST " << path << " HTTP/1.1\r\n"
      << "Host: " << host << ":" << port_str << "\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n"
      << "\r\n"
      << body;

  const auto req_str = req.str();
  const char* data = req_str.c_str();
  ssize_t sent = 0;
  const ssize_t total = static_cast<ssize_t>(req_str.size());
  while (sent < total) {
    const ssize_t n = send(fd, data + sent, static_cast<size_t>(total - sent), MSG_NOSIGNAL);
    if (n <= 0) break;
    sent += n;
  }

  // Drain response to allow clean TCP close; ignore content
  char buf[512];
  while (recv(fd, buf, sizeof(buf), 0) > 0) {}
  close(fd);
  return sent == total;
}

// ---- Webhook payload --------------------------------------------------------

// Produces a Slack / Teams-compatible incoming webhook payload.
// Newlines in the "text" field use the JSON literal \n so the message renders
// as multiple lines in most chat platforms.
std::string build_webhook_payload(const AlertEvent& evt) {
  std::ostringstream ss;
  ss << "{\"text\":\"*[sysguardd][" << alert_json_escape(evt.severity) << "]*"
     << " Process event on `" << alert_json_escape(evt.node_id) << "`"
     << "\\nexe: `" << alert_json_escape(evt.exe) << "`"
     << "\\npid: " << evt.pid
     << "\\nreason: " << alert_json_escape(evt.reason);
  if (!evt.action.empty()) {
    ss << "\\naction: " << alert_json_escape(evt.action);
  }
  ss << "\\nevent_id: " << alert_json_escape(evt.event_id);
  if (!evt.policy_version.empty()) {
    ss << "\\npolicy_version: " << alert_json_escape(evt.policy_version);
  }
  ss << "\"}";
  return ss.str();
}

}  // namespace

// ---- AlertDispatcher --------------------------------------------------------

AlertDispatcher::AlertDispatcher(const AlertConfig& cfg, std::string node_id,
                                 std::string policy_version)
    : cfg_(cfg),
      node_id_(std::move(node_id)),
      policy_version_(std::move(policy_version)),
      min_level_(severity_level(cfg.min_severity)),
      tokens_(cfg.rate_limit_per_minute),
      last_refill_(std::chrono::system_clock::now()) {
  worker_ = std::thread(&AlertDispatcher::worker_loop, this);
}

AlertDispatcher::~AlertDispatcher() {
  {
    std::lock_guard<std::mutex> lk(mu_);
    stop_ = true;
  }
  cv_.notify_one();
  worker_.join();
}

void AlertDispatcher::dispatch(AlertEvent event) {
  if (severity_level(event.severity) < min_level_) {
    return;
  }

  // Enrich with dispatcher context before queuing
  event.node_id = node_id_;
  event.policy_version = policy_version_;

  std::lock_guard<std::mutex> lk(mu_);
  if (queue_.size() >= kMaxQueueSize) {
    // Drop the oldest entry rather than blocking the caller (daemon runtime safety)
    queue_.pop_front();
  }
  queue_.push_back(std::move(event));
  cv_.notify_one();
}

bool AlertDispatcher::should_send(const AlertEvent& evt) {
  const auto now = std::chrono::system_clock::now();

  // Refill token bucket once per minute
  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_).count();
  if (elapsed_ms >= 60000) {
    tokens_ = cfg_.rate_limit_per_minute;
    last_refill_ = now;
  }

  if (tokens_ <= 0) {
    return false;  // Rate-limited: drop to protect downstream
  }

  // Deduplicate: same exe + severity within the configured window
  const std::string key = evt.exe + ":" + evt.severity;
  const auto it = dedupe_map_.find(key);
  if (it != dedupe_map_.end()) {
    const auto age_sec =
        std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
    if (age_sec < cfg_.dedupe_window_sec) {
      return false;
    }
  }

  dedupe_map_[key] = now;
  --tokens_;
  return true;
}

void AlertDispatcher::worker_loop() {
  while (true) {
    AlertEvent evt;
    {
      std::unique_lock<std::mutex> lk(mu_);
      cv_.wait(lk, [this] { return stop_ || !queue_.empty(); });
      if (stop_ && queue_.empty()) {
        return;
      }
      evt = std::move(queue_.front());
      queue_.pop_front();
    }

    if (!should_send(evt)) {
      continue;
    }

    send_webhook(evt);
  }
}

void AlertDispatcher::send_webhook(const AlertEvent& evt) {
  if (cfg_.webhook_url.empty()) {
    return;
  }
  const auto payload = build_webhook_payload(evt);
  if (!send_http_post(cfg_.webhook_url, payload)) {
    std::cerr << "alert: webhook delivery failed for event " << evt.event_id << "\n";
  }
}

}  // namespace sysguardd
