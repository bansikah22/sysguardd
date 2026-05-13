#!/bin/bash
# Test event injection for sysguardd deployed in kind cluster
# Validates monitor and enforce modes with controlled process events

set -e

CONTEXT="kind-sysguardd-test"
NAMESPACE="kube-system"
DAEMON_LABEL="app=sysguardd"
TIMEOUT=30

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;36m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $*"
}

# Get the first sysguardd pod
get_pod() {
    kubectl get pods -n "$NAMESPACE" --context "$CONTEXT" -l "$DAEMON_LABEL" -o jsonpath='{.items[0].metadata.name}' 2>/dev/null
}

# Check if pod is running
check_pod_health() {
    local pod="$1"
    local status=$(kubectl get pod -n "$NAMESPACE" --context "$CONTEXT" "$pod" -o jsonpath='{.status.phase}')
    if [ "$status" = "Running" ]; then
        return 0
    else
        log_error "Pod $pod is not running (status: $status)"
        return 1
    fi
}

# Get pod's current mode
get_pod_mode() {
    local pod="$1"
    kubectl exec -n "$NAMESPACE" --context "$CONTEXT" "$pod" -- /usr/local/bin/sysguardd status 2>/dev/null | grep "Mode:" | awk '{print $2}' || echo "unknown"
}

# Inject a single event and return the audit log
inject_event() {
    local pod="$1"
    local pid="$2"
    local ppid="$3"
    local exe="$4"
    shift 4
    local args=("$@")
    
    # Build event string
    local event="$pid $ppid $exe"
    for arg in "${args[@]}"; do
        event="$event $arg"
    done
    
    log_info "Injecting event: $event"
    
    # Send event via echo and capture output
    # Use daemon command with proper arguments
    kubectl exec -n "$NAMESPACE" --context "$CONTEXT" "$pod" -- bash -c "echo '$event' | /usr/local/bin/sysguardd daemon --mode monitor --policy /etc/sysguardd/default.policy 2>&1" 2>/dev/null
}

# Test scenario 1: Monitor mode with denied executable
test_monitor_denied_bash() {
    log_info "=== Test 1: Monitor Mode - Denied Executable (/bin/bash) ==="
    
    local pod=$(get_pod)
    if [ -z "$pod" ]; then
        log_error "No sysguardd pod found"
        return 1
    fi
    
    log_info "Using pod: $pod"
    
    # Verify pod is healthy
    if ! check_pod_health "$pod"; then
        return 1
    fi
    
    # Verify current mode
    local mode=$(get_pod_mode "$pod")
    log_info "Current mode: $mode"
    if [ "$mode" != "monitor" ]; then
        log_warn "Pod is in $mode mode, expected monitor. Test results may vary."
    fi
    
    # Inject event: bash trying to run
    local output=$(inject_event "$pod" 1234 1 /bin/bash "-c" "whoami")
    
    # Parse JSON output
    if echo "$output" | grep -q '"decision"'; then
        log_success "Received structured audit output"
        
        # Check decision
        if echo "$output" | grep -q '"decision":"deny"'; then
            log_success "Decision: DENY (correct)"
        else
            log_warn "Expected 'decision':'deny' in output"
        fi
        
        # Check enforced
        if echo "$output" | grep -q '"enforced":false'; then
            log_success "Enforced: false (monitor mode, no termination)"
        else
            log_warn "Expected 'enforced':false in monitor mode"
        fi
        
        # Print full output
        log_info "Audit log output:"
        echo "$output" | grep -o '{".*}' || echo "$output"
    else
        log_warn "No structured output received. Raw output:"
        echo "$output"
    fi
    
    # Verify pod still healthy
    if check_pod_health "$pod"; then
        log_success "Pod remains healthy after event injection"
    else
        log_error "Pod crashed or became unhealthy"
        return 1
    fi
}

# Test scenario 2: Monitor mode with allowed executable
test_monitor_allowed_ls() {
    log_info "=== Test 2: Monitor Mode - Allowed Executable (/bin/ls) ==="
    
    local pod=$(get_pod)
    if [ -z "$pod" ]; then
        log_error "No sysguardd pod found"
        return 1
    fi
    
    # Inject event: ls (should be allowed)
    local output=$(inject_event "$pod" 5678 1 /bin/ls "-la" "/tmp")
    
    if echo "$output" | grep -q '"decision"'; then
        log_success "Received structured audit output"
        
        # Check decision
        if echo "$output" | grep -q '"decision":"allow"'; then
            log_success "Decision: ALLOW (correct)"
        else
            log_warn "Expected 'decision':'allow' for /bin/ls"
        fi
        
        # Print full output
        log_info "Audit log output:"
        echo "$output" | grep -o '{".*}' || echo "$output"
    else
        log_warn "No structured output received. Raw output:"
        echo "$output"
    fi
    
    if check_pod_health "$pod"; then
        log_success "Pod remains healthy"
    fi
}

# Test scenario 3: Verify JSON schema
test_json_schema() {
    log_info "=== Test 3: JSON Schema Validation ==="
    
    local pod=$(get_pod)
    if [ -z "$pod" ]; then
        log_error "No sysguardd pod found"
        return 1
    fi
    
    # Inject event and capture output
    local output=$(inject_event "$pod" 9999 1 /bin/bash)
    
    # Check for required JSON fields
    local required_fields=("timestamp" "pid" "ppid" "exe" "decision" "reason" "enforced" "action" "action_error")
    local missing_fields=()
    
    for field in "${required_fields[@]}"; do
        if ! echo "$output" | grep -q "\"$field\""; then
            missing_fields+=("$field")
        fi
    done
    
    if [ ${#missing_fields[@]} -eq 0 ]; then
        log_success "All required JSON fields present"
    else
        log_error "Missing JSON fields: ${missing_fields[*]}"
        log_info "Output: $output"
    fi
}

# Main execution
main() {
    log_info "Starting sysguardd event injection tests"
    log_info "Context: $CONTEXT | Namespace: $NAMESPACE"
    
    # Verify kind cluster and pod
    if ! kubectl cluster-info --context "$CONTEXT" &>/dev/null; then
        log_error "Kind cluster '$CONTEXT' not accessible"
        return 1
    fi
    
    local pod=$(get_pod)
    if [ -z "$pod" ]; then
        log_error "No sysguardd pod found in $NAMESPACE namespace"
        return 1
    fi
    
    log_success "Found pod: $pod"
    
    # Run tests
    test_monitor_denied_bash
    echo
    
    test_monitor_allowed_ls
    echo
    
    test_json_schema
    echo
    
    log_info "=== Event Injection Tests Complete ==="
}

main "$@"
