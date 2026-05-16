# Maintainer Guide

This section is intended for contributors and release maintainers.

If you are an end user, start with [Quick Start](USER_GUIDE.md).

## Scope

Maintainer documentation covers:

- Architecture and internal component model
- Helm packaging and OCI chart publishing
- Event injection and deeper validation workflows
- Release planning and roadmap references

## Core Maintainer Pages

- [Architecture](ARCHITECTURE.md)
- [Components](COMPONENTS.md)
- [Helm Distribution](HELM_DISTRIBUTION.md)
- [Helm Quick Reference](HELM_QUICK_REFERENCE.md)
- [Helm Publishing Setup](HELM_PUBLISHING_SETUP.md)
- [Event Injection Testing](EVENT_INJECTION_TESTING.md)
- [Roadmap](ROADMAP.md)

## Documentation Standards

Keep docs professional and audience-specific:

- Put installation and run commands in end-user guides first.
- Keep implementation details in maintainer pages.
- Prefer copy-paste-ready commands over placeholders.
- Mark unstable or planned features explicitly.
- Keep Kubernetes examples production-safe (Secrets for webhook URLs).
