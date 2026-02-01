# Upstream microsocks Open Pull Requests Analysis

> Source: https://github.com/rofl0r/microsocks/pulls
> Date: 2026-02-01
> Total open PRs: 7

## Features (5 PRs)

### PR #98 — Add Dockerfile and GitHub Actions workflow
- **Author:** meanwhile131 | **Created:** 2026-01-06
- **URL:** https://github.com/rofl0r/microsocks/pull/98
- **Files:** `.dockerignore`, `.github/workflows/build.yml`, `.gitignore`, `Dockerfile`, `README.md` (+85 lines)
- **Summary:** Adds a multi-stage Dockerfile (Alpine builder to scratch runtime) and a CI/CD workflow that builds Docker images for 8 architectures (386, amd64, arm/v6, arm/v7, arm64/v8, ppc64le, riscv64, s390x). Images are published to GHCR and statically-built binaries are uploaded as artifacts.

### PR #96 — Add SOCKS5 forwarding rules support
- **Author:** lwb1978 | **Created:** 2025-12-22
- **URL:** https://github.com/rofl0r/microsocks/pull/96
- **Files:** `sockssrv.c` (+305/-7 lines)
- **Summary:** Builds on PR #93's concept. Adds a `-f` flag for forwarding rules that selectively route matching destinations through upstream SOCKS5 proxies with optional authentication. Includes upstream handshake logic, socket timeouts (5s), and `-V` version flag.

### PR #95 — Allow building with Windows + MinGW
- **Author:** ccuser44 | **Created:** 2025-12-07
- **URL:** https://github.com/rofl0r/microsocks/pull/95
- **Files:** `dprintf.c` (new), `server.h`, `sockssrv.c`, `wsa2unix.h` (new) (+136/-7 lines)
- **Summary:** Adds Windows cross-compilation support via MinGW. Provides a `dprintf()` implementation, a `wsa2unix.h` compatibility header mapping Winsock error codes to Unix equivalents, conditional includes for `winsock2.h`/`ws2tcpip.h`, and replaces `poll()` with `WSAPoll()` on Windows.

### PR #93 — Forwarding rules
- **Author:** ohwgiles | **Created:** 2025-10-04
- **URL:** https://github.com/rofl0r/microsocks/pull/93
- **Files:** `sockssrv.c` (+126/-3 lines)
- **Summary:** Original implementation of selective forwarding rules using the syntax `match_name:match_port,[user:pass@]upstream_name:upstream_port,remote_name:remote_port`. Allows microsocks to act as a gateway to remote private networks via upstream SOCKS5 servers. PR #96 is a more complete evolution of this work.

### PR #79 — Add `-B` bind device option
- **Author:** peppergrayxyz | **Created:** 2024-09-28
- **URL:** https://github.com/rofl0r/microsocks/pull/79
- **Files:** `Makefile`, `README.md`, `bind2device.c` (new), `bind2device.h` (new), `sockssrv.c` (+73/-4 lines)
- **Summary:** Adds a `-B` flag to bind outgoing sockets to a specific network interface. Uses `SO_BINDTODEVICE` on Linux and `IP_BOUND_IF`/`IPV6_BOUND_IF` on BSD/macOS. Includes a no-op stub for unsupported platforms. Follows up on issue #29.

## Improvements (1 PR)

### PR #90 — Add timestamps to log output
- **Author:** ZsBT | **Created:** 2025-09-04
- **URL:** https://github.com/rofl0r/microsocks/pull/90
- **Files:** `sockssrv.c` (+17/-1 lines)
- **Summary:** Converts the `dolog` macro into a `static inline` function that prepends `[YYYY-MM-DD HH:MM:SS]` timestamps using `localtime_r()` and `vdprintf()`. Also adds a startup log message displaying the listening address and port.

## Fixes (1 PR)

### PR #86 — Fix minor nits in the manual page
- **Author:** ppentchev | **Created:** 2025-02-14
- **URL:** https://github.com/rofl0r/microsocks/pull/86
- **Files:** `microsocks.1` (+22/-13 lines)
- **Summary:** Fixes formatting issues in the man page: removes a stray `.Oc` bracket, improves grammar/punctuation in option descriptions (`-i`, `-w`), and applies the FreeBSD documentation convention of starting each sentence on a new line.

## Notable Observations

- **PR #93 and #96 overlap:** Both implement SOCKS5 forwarding rules. PR #96 by lwb1978 explicitly builds on PR #93 by ohwgiles and is more comprehensive (+305 lines vs +126 lines), adding authentication support and socket timeouts.
- **PR #79 is the oldest** (Sept 2024), adding bind-to-device support — a frequently requested feature (issue #29).
- **PR #95 (Windows/MinGW)** is marked by its author as "mostly functional" with known caveats around `dprintf`.
- **PR #86 (man page fixes)** is the most straightforward and lowest-risk change.
