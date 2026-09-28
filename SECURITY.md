# Security Policy

Security vulnerabilities are defects that can affect confidentiality,
integrity, or availability across a trust boundary. Ordinary crashes, build
failures, recognition quality issues, and feature requests should use the
normal issue tracker once the repository is published.

No dedicated private security contact is configured yet. After publication,
prefer GitHub's private vulnerability reporting mechanism if it is enabled for
the repository. If it is not enabled, use a non-public maintainer contact made
available on the repository or maintainer profile. Do not post exploit details
publicly before a private reporting channel has been established.

Reports should include the affected revision, environment, minimal
reproduction, impact, and any relevant sanitizer or debugger output. Do not
include model weights, personal audio, credentials, or unrelated private data.

Dependency issues in whisper.cpp, GGML, Tcl, compilers, drivers, or CUDA may
need to be reported to their respective maintainers after confirming where the
problem originates.
