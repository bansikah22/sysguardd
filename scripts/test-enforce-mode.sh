#!/bin/bash
# Test enforce mode for sysguardd
# This test validates that the daemon logs enforcement actions when in enforce mode

set -e

CONTEXT="kind-sysguardd-test"
NAMESPACE="kube-system"
DAEMON_LABEL="app=sysguardd"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;36m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $*"; }
log_success() { echo -e "${GREEN}[PASS]${NC} $*"; }
log_error() { echo -e "${RED}[FAIL]${NC} $*"; }

# Get the first sysguardd pod
get_pod() {
    kubectl get pods -n "$NAMESPACE" --context "$CONTEXT" -l "$DAEMON_LABEL" -o jsonpath='{.items[0].metadata.name}' 2>/dev/null
}

# Inject an event into the daemon in enforce mode
inject_event_enforce() {
    local pod="$1"
    local pid="$2"
    local ppid="$3"
    local exe="$4"
    shift 4
    local args=("$@")
    
    local event="$pid $ppid $exe"
    for arg in "${args[@]}"; do
        event="$event $arg"
    done
    
    log_info "Injecting event in ENFORCE mode: $event"
    
    # Run daemon in enforce mode
    kubectl exec -n "$NAMESPACE" --context "$CONTEXT" "$pod" -- bash -c "echo '$event' | /usr/local/bin/sysguardd daemon --mode enforce --policy /etc/sysguardd/default.policy 2>&1" 2>/dev/null
}

main() {
    log_info "Starting enforce mode validation test"
    log_info "Context: $CONTEXT | Namespace: $NAMESPACE"
    
    local pod=$(get_pod)
    if [ -z "$pod" ]; then
        log_error "No sysguardd pod found"
        return 1
    fi
    
    log_success "Found pod: $pod"
    
    # Test 1: Enforce mode with denied executable
    log_info "=== Test: Enforce Mode - Denied Executable (/bin/bash) ==="
    local output=$(inject_event_enforce "$pod" 2000 1 /bin/bash "-i")
    
    if echo "$output" | grep -q '"decision":"deny"'; then
        log_success "Decision: DENY (correct)"
    else
        log_error "Expected deny decision"
    fi
    
    # Check if enforced field is true
    if echo "$output" | grep -q '"enforced":true'; then
        log_success "Enforced: true (daemon attempted to terminate process)"
    else
        log_info "Enforced status: $(echo "$output" | grep -o '"enforced":[^,]*')"
    fi
    
    # Print audit output
    log_info "Audit log output:"
    echo "$output" | grep -o '{".*}' || echo "$output"
    
    echo
    
    # Test 2: Enforce mode with allowed executable (should still be allowed)
    log_info "=== Test: Enforce Mode - Allowed Executable (/bin/ls) ==="
    local output2=$(inject_event_enforce "$pod" 3000 1 /bin/ls "-la")
    
    if echo "$output2" | grep -q '"decision":"allow"'; then
        log_success "Decision: ALLOW (correct)"
    else
        log_error "Expected allow decision for /bin/ls"
    fi
    
    # Enforced should be false since it was allowed
    if echo "$output2" | grep -q '"enforced":false'; then
        log_success "Enforced: false (allowed process, no termination attempted)"
    else
        log_info "Enforced status: $(echo "$output2" | grep -o '"enforced":[^,]*')"
    fi
    
    log_info "Audit log output:"
    echo "$output2" | grep -o '{".*}' || echo "$output2"
    
    log_success "=== Enforce Mode Validation Complete ==="
}

main "$@"
