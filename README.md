# lx - Lightweight Privilege Escalation

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
