# lx - Lightweight Root Access Tool

Simple alternative to sudo/doas for minimal systems.

## Features
- Password authentication using system crypt()
- Single config file (/etc/lx.conf)
- No external dependencies
- Minimal codebase (<200 lines)

## Installation
```sh
make
sudo make install


# lx — Lightweight Root Access Tool  

[![License](https://img.shields.io/badge/license-BSD-green)](LICENSE)  
*A simple, secure way to delegate root commands without bloat.*  

✔ **Shadow password auth**  
✔ **Single config file** (`/etc/lx.conf`)  
✔ **Zero PAM/systemd dependencies**  
✔ **Designed for minimal distros/embedded systems**  
