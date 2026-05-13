# SysGuardd Roadmap

## Purpose
This roadmap defines delivery phases and gates so the core security path is implemented and validated before any differentiator is added.

## Phase 1: Baseline Build
- Implement kernel event capture for process execution.
- Implement ring-buffer transport and user-space ingestion.
- Implement deterministic policy evaluation.
- Implement kill-path mitigation and audit emission.

## Phase 2: Baseline Validation
- Functional tests for allow and deny process flows.
- Latency tests for decision and mitigation path.
- Stability tests under sustained process spawn load.
- False-positive analysis in monitor mode before broad enforcement.

## Phase 3: Unique Feature (After Baseline Pass)
The unique feature is intentionally deferred until Phase 1 and Phase 2 are complete.

Entry criteria:
- Core capabilities implemented and passing tests.
- Performance and reliability baseline measured.
- Enforcement behavior validated in canary rollout.

Candidate direction:
- Trust Gradient Mode: policy decisions can attach confidence levels and context evidence to each block event, enabling safer progressive hardening without losing deterministic enforcement.

## Decision Rule
- Do not start Phase 3 work until all entry criteria are complete and reviewed.
- Keep the unique feature behind a flag until production readiness is confirmed.
