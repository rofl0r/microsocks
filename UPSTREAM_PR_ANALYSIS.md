# Upstream microsocks Open Pull Requests Analysis

> Source: https://github.com/rofl0r/microsocks/pulls
> Date: 2026-02-01
> Total open PRs: 11

## Features (8 PRs)

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

### PR #95 — Allow building with Windows + MinGW (Draft)
- **Author:** ccuser44 | **Created:** 2025-12-07
- **URL:** https://github.com/rofl0r/microsocks/pull/95
- **Files:** `dprintf.c` (new), `server.h`, `sockssrv.c`, `wsa2unix.h` (new) (+136/-7 lines)
- **Summary:** Adds Windows cross-compilation support via MinGW. Provides a `dprintf()` implementation, a `wsa2unix.h` compatibility header mapping Winsock error codes to Unix equivalents, conditional includes for `winsock2.h`/`ws2tcpip.h`, and replaces `poll()` with `WSAPoll()` on Windows. Author notes it is "mostly functional" with known `dprintf` caveats.

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

### PR #64 — Add `-t` option for idle exit timeout
- **Author:** chetan-reddy | **Created:** 2023-08-18
- **URL:** https://github.com/rofl0r/microsocks/pull/64
- **Files:** `sockssrv.c` (+41/-3 lines)
- **Summary:** Adds a `-t` flag specifying an idle exit timeout in seconds. When no connections arrive within the timeout period and no active threads exist, the server exits automatically. Uses non-blocking sockets via `fcntl()` and `poll()`. Useful for on-demand launches in resource-constrained environments. Tested on Linux and macOS.

### PR #38 — Enable SO_MARK support on Linux via compile-time flag
- **Author:** grandrew | **Created:** 2021-06-14
- **URL:** https://github.com/rofl0r/microsocks/pull/38
- **Files:** `README.md`, `sockssrv.c` (+39 lines)
- **Summary:** Adds a `-m <mark_id>` option to mark outgoing packets with Linux `SO_MARK` for policy-based routing. Enabled via a compile-time `SOMARK` flag. Includes README documentation with example commands showing how to route connections through specific network interfaces (e.g., `tun1`).

### PR #29 — Allow binding to another (non-default) interface
- **Author:** tahajahangir | **Created:** 2020-09-25
- **URL:** https://github.com/rofl0r/microsocks/pull/29
- **Files:** `README.md`, `sockssrv.c` (+9/-2 lines)
- **Summary:** Adds a `-B` flag using `SO_BINDTODEVICE` to bind outgoing sockets to a specific network interface. A simpler, Linux-only predecessor to PR #79, which adds cross-platform support for the same feature.

## Improvements (2 PRs)

### PR #90 — Add timestamps to log output
- **Author:** ZsBT | **Created:** 2025-09-04
- **URL:** https://github.com/rofl0r/microsocks/pull/90
- **Files:** `sockssrv.c` (+17/-1 lines)
- **Summary:** Converts the `dolog` macro into a `static inline` function that prepends `[YYYY-MM-DD HH:MM:SS]` timestamps using `localtime_r()` and `vdprintf()`. Also adds a startup log message displaying the listening address and port.

### PR #70 — Print timestamps in logs
- **Author:** Xenapte | **Created:** 2023-12-19
- **URL:** https://github.com/rofl0r/microsocks/pull/70
- **Files:** `sockssrv.c` (+6/-1 lines)
- **Summary:** Adds a `LOGTS()` macro that prepends `[MM-DD HH:MM:SS]` timestamps to log output using `strftime()` and `localtime_r()`. A lighter-weight predecessor to PR #90 which uses a different timestamp format (`YYYY-MM-DD`) and a different implementation approach.

## Fixes (1 PR)

### PR #86 — Fix minor nits in the manual page
- **Author:** ppentchev | **Created:** 2025-02-14
- **URL:** https://github.com/rofl0r/microsocks/pull/86
- **Files:** `microsocks.1` (+22/-13 lines)
- **Summary:** Fixes formatting issues in the man page: removes a stray `.Oc` bracket, improves grammar/punctuation in option descriptions (`-i`, `-w`), and applies the FreeBSD documentation convention of starting each sentence on a new line.

## Notable Observations

- **PR #93 and #96 overlap:** Both implement SOCKS5 forwarding rules. PR #96 by lwb1978 explicitly builds on PR #93 by ohwgiles and is more comprehensive (+305 lines vs +126 lines), adding authentication support and socket timeouts.
- **PR #29 and #79 overlap:** Both add `-B` bind-to-device support. PR #29 (2020) is Linux-only using `SO_BINDTODEVICE`; PR #79 (2024) is cross-platform, adding macOS support via `IP_BOUND_IF`/`IPV6_BOUND_IF`.
- **PR #70 and #90 overlap:** Both add timestamp prefixes to log output. PR #70 (2023) uses a macro with `[MM-DD HH:MM:SS]` format; PR #90 (2025) uses a function with `[YYYY-MM-DD HH:MM:SS]` format and also adds a startup message.
- **PR #95 (Windows/MinGW)** is a Draft PR, marked by its author as "mostly functional" with known caveats around `dprintf`.
- **PR #29 is the oldest** open PR (Sept 2020, over 5 years old).
- **PR #86 (man page fixes)** is the most straightforward and lowest-risk change.
- **Three pairs of related PRs** exist (#29/#79, #70/#90, #93/#96), where later PRs supersede or extend earlier ones.
