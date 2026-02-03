# Simple Concurrent HTTP Server ✅

A small, cross-platform (POSIX / Windows) concurrent HTTP server written in C that serves a static `index.html` file from the working directory.

---

## Features 🔧

- Minimal HTTP server example (`server.c`)  
- Loads `index.html` once into memory and serves it to multiple clients concurrently
- Uses threads (pthreads on POSIX, CreateThread on Windows) for concurrent clients
- Per-client receive timeouts can be set to avoid slow-client stalls

---

## Quick start 🚀

1. Open a terminal in the `http-server/` directory and make sure `index.html` is present.
2. Build:

- Linux (gcc):

```sh
gcc server.c -o server -pthread

./server.exe
```

- Windows (MYSYS2 MinGW64)

This project must be built using MSYS2 MinGW64.
Do not use Windows CMD or PowerShell.

```bash
pacman -S mingw-w64-x86_64-gcc make   # installing make

cd /c/path/to/project

make

./server.exe
```

---

## Configuration & notes 💡

- Port: the default port is **8080** (`PORT` in `server.c`). Change and rebuild if you want a different port.

- Timeouts: to avoid a slow client blocking a thread, set a receive timeout on the **accepted** client socket (use `SO_RCVTIMEO`):
  - POSIX: use a `struct timeval` (seconds + microseconds)
  - Windows: use a `DWORD` timeout in milliseconds

- Run the server from the `http-server` directory so `index.html` is found by the server.

- Current implementation is minimal. It does not:
  - parse HTTP requests beyond logging the request text
  - support multiple routes or MIME type detection beyond `text/html`
  - implement robust error handling or request limits

