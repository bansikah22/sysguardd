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

#include <openssl/err.h>
#include <openssl/ssl.h>

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

bool has_header_control_chars(const std::string& value) {
  for (unsigned char c : value) {
    if (c == '\r' || c == '\n' || c == 0) {
      return true;
    }
  }
  return false;
}

int parse_http_status_code(const std::string& response) {
  const auto line_end = response.find("\r\n");
  const std::string first_line = line_end == std::string::npos ? response : response.substr(0, line_end);
  const auto first_space = first_line.find(' ');
  if (first_space == std::string::npos) {
    return 0;
  }
  const auto second_space = first_line.find(' ', first_space + 1);
  const std::string code_str =
      second_space == std::string::npos
          ? first_line.substr(first_space + 1)
          : first_line.substr(first_space + 1, second_space - (first_space + 1));
  if (code_str.size() != 3) {
    return 0;
  }
  for (char c : code_str) {
    if (c < '0' || c > '9') {
      return 0;
    }
  }
  return std::stoi(code_str);
}

// ---- HTTP/HTTPS POST sender -------------------------------------------------

struct ParsedWebhookUrl {
  std::string scheme;
  std::string host;
  std::string port;
  std::string path;
};

bool parse_webhook_url(const std::string& url, ParsedWebhookUrl& parsed) {
  if (url.rfind("http://", 0) == 0) {
    parsed.scheme = "http";
  } else if (url.rfind("https://", 0) == 0) {
    parsed.scheme = "https";
  } else {
    std::cerr << "alert: only http:// and https:// webhook URLs are supported\n";
    return false;
  }

  const std::string rest = url.substr(parsed.scheme.size() + 3);
  parsed.port = parsed.scheme == "https" ? "443" : "80";
  parsed.path = "/";

  const auto slash_pos = rest.find('/');
  const std::string hostport = (slash_pos == std::string::npos) ? rest : rest.substr(0, slash_pos);
  if (slash_pos != std::string::npos) {
    parsed.path = rest.substr(slash_pos);
  }

  const auto colon_pos = hostport.find(':');
  if (colon_pos == std::string::npos) {
    parsed.host = hostport;
  } else {
    parsed.host = hostport.substr(0, colon_pos);
    parsed.port = hostport.substr(colon_pos + 1);
    if (parsed.port.empty()) {
      std::cerr << "alert: invalid empty port in webhook URL\n";
      return false;
    }
    // Validate port is purely numeric (CWE-20)
    for (char c : parsed.port) {
      if (c < '0' || c > '9') {
        std::cerr << "alert: invalid port in webhook URL\n";
        return false;
      }
    }

    size_t pos = 0;
    unsigned long parsed_port = 0;
    try {
      parsed_port = std::stoul(parsed.port, &pos, 10);
    } catch (const std::exception&) {
      std::cerr << "alert: invalid port in webhook URL\n";
      return false;
    }
    if (pos != parsed.port.size() || parsed_port < 1 || parsed_port > 65535) {
      std::cerr << "alert: port out of range in webhook URL\n";
      return false;
    }
  }

  if (parsed.host.empty()) {
    std::cerr << "alert: empty host in webhook URL\n";
    return false;
  }
  if (has_header_control_chars(parsed.host) || has_header_control_chars(parsed.port) ||
      has_header_control_chars(parsed.path)) {
    std::cerr << "alert: invalid control characters in webhook URL\n";
    return false;
  }

  return true;
}

bool send_http_post_socket(const ParsedWebhookUrl& parsed, const std::string& body) {
  struct addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;

  struct addrinfo* res = nullptr;
  if (getaddrinfo(parsed.host.c_str(), parsed.port.c_str(), &hints, &res) != 0 || res == nullptr) {
    std::cerr << "alert: failed to resolve webhook host: " << parsed.host << "\n";
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
    std::cerr << "alert: failed to connect to webhook host: " << parsed.host << "\n";
    return false;
  }
  freeaddrinfo(res);

  std::ostringstream req;
  req << "POST " << parsed.path << " HTTP/1.1\r\n"
      << "Host: " << parsed.host << ":" << parsed.port << "\r\n"
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

  std::string response;
  char buf[512];
  ssize_t n = 0;
  while ((n = recv(fd, buf, sizeof(buf), 0)) > 0) {
    if (response.size() < 2048) {
      response.append(buf, static_cast<size_t>(n));
    }
  }
  close(fd);
  if (sent != total) {
    return false;
  }

  const int status = parse_http_status_code(response);
  if (status == 0) {
    std::cerr << "alert: invalid HTTP response from webhook\n";
    return false;
  }
  if (status < 200 || status > 299) {
    std::cerr << "alert: webhook returned HTTP " << status << "\n";
    return false;
  }
  return true;
}

bool send_https_post_socket(const ParsedWebhookUrl& parsed, const std::string& body) {
  struct addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;

  struct addrinfo* res = nullptr;
  if (getaddrinfo(parsed.host.c_str(), parsed.port.c_str(), &hints, &res) != 0 || res == nullptr) {
    std::cerr << "alert: failed to resolve webhook host: " << parsed.host << "\n";
    return false;
  }

  const int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    freeaddrinfo(res);
    std::cerr << "alert: socket creation failed\n";
    return false;
  }

  struct timeval tv{5, 0};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  if (connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
    freeaddrinfo(res);
    close(fd);
    std::cerr << "alert: failed to connect to webhook host: " << parsed.host << "\n";
    return false;
  }
  freeaddrinfo(res);

  SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
  if (ctx == nullptr) {
    close(fd);
    std::cerr << "alert: failed to initialize TLS context\n";
    return false;
  }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
  if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
    SSL_CTX_free(ctx);
    close(fd);
    std::cerr << "alert: failed to load system TLS trust store\n";
    return false;
  }

  SSL* ssl = SSL_new(ctx);
  if (ssl == nullptr) {
    SSL_CTX_free(ctx);
    close(fd);
    std::cerr << "alert: failed to create TLS session\n";
    return false;
  }

  SSL_set_fd(ssl, fd);
  SSL_set_tlsext_host_name(ssl, parsed.host.c_str());
  SSL_set1_host(ssl, parsed.host.c_str());

  if (SSL_connect(ssl) != 1) {
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(fd);
    std::cerr << "alert: TLS handshake failed for webhook host: " << parsed.host << "\n";
    return false;
  }

  std::ostringstream req;
  req << "POST " << parsed.path << " HTTP/1.1\r\n"
      << "Host: " << parsed.host << ":" << parsed.port << "\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n"
      << "\r\n"
      << body;

  const auto req_str = req.str();
  const char* data = req_str.c_str();
  size_t sent = 0;
  while (sent < req_str.size()) {
    const int n = SSL_write(ssl, data + sent, static_cast<int>(req_str.size() - sent));
    if (n <= 0) break;
    sent += static_cast<size_t>(n);
  }

  std::string response;
  char buf[512];
  int n = 0;
  while ((n = SSL_read(ssl, buf, sizeof(buf))) > 0) {
    if (response.size() < 2048) {
      response.append(buf, static_cast<size_t>(n));
    }
  }

  bool ok = sent == req_str.size();
  if (ok) {
    const int status = parse_http_status_code(response);
    if (status == 0) {
      std::cerr << "alert: invalid HTTP response from webhook\n";
      ok = false;
    } else if (status < 200 || status > 299) {
      std::cerr << "alert: webhook returned HTTP " << status << "\n";
      ok = false;
    }
  }
  SSL_shutdown(ssl);
  SSL_free(ssl);
  SSL_CTX_free(ctx);
  close(fd);
  return ok;
}

bool send_webhook_request(const std::string& url, const std::string& body) {
  ParsedWebhookUrl parsed;
  if (!parse_webhook_url(url, parsed)) {
    return false;
  }

  if (parsed.scheme == "https") {
    return send_https_post_socket(parsed, body);
  }

  return send_http_post_socket(parsed, body);
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
      last_refill_(std::chrono::steady_clock::now()) {
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
  const auto refill_now = std::chrono::steady_clock::now();

  const auto elapsed = refill_now - last_refill_;
  if (elapsed >= std::chrono::minutes(1)) {
    tokens_ = cfg_.rate_limit_per_minute;
    const auto minutes_elapsed = std::chrono::duration_cast<std::chrono::minutes>(elapsed);
    last_refill_ += minutes_elapsed;
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
  if (!send_webhook_request(cfg_.webhook_url, payload)) {
    std::cerr << "alert: webhook delivery failed for event " << evt.event_id << "\n";
  }
}

}  // namespace sysguardd
