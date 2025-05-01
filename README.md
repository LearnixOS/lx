# lx — Lightweight Root Access Tool  

[![License](https://img.shields.io/badge/license-BSD-green)](LICENSE)  
*A simple, secure way to delegate root commands without bloat.*  

✔ **Shadow password auth**  

✔ **Single config file** `/etc/lx.conf` -> permit username 

✔ **Zero PAM/systemd dependencies**  

✔ **Designed for minimal distros/embedded systems**  

## Installation
```sh
git clone https://github.com/LearnixOS/lx && cd lx && sudo make clean install
```
### Persistence
If you want the optional feature of persistence like doas/sudo, you can run `make PERSIST=1`. 
For those who don't know: persistence is basically a "grace period", you input your password once and it doesn't ask for it again for the next seconds/minutes. 
Be warned that this is not very "safe", and especially how it is implemented in its current state. It just checks the UID and timestamp, it doesn't 
separate based on the tty, yet. **YOU HAVE BEEN WARNED**

**NOTE**: This is experimental, if you encounter any bugs please report them.
