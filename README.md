MicroSocks - multithreaded, small, efficient SOCKS5 server.
===========================================================

A SOCKS5 service that you can run on your remote boxes to tunnel connections
through them, if for some reason SSH doesn't cut it for you.

It's very lightweight, and very light on resources too:

For every client, a thread with a stack size of 8KB is spawned.
The main process basically doesn't consume any resources at all.

The only limits are the amount of file descriptors and the RAM.

It's also designed to be robust: it handles resource exhaustion
gracefully by simply denying new connections, instead of calling abort()
as most other programs do these days.

Another plus is ease-of-use: no config file necessary, everything can be
done from the command line and doesn't even need any parameters for quick
setup.

History
-------

This is the successor of "rocksocks5", and it was written with
different goals in mind:

- Prefer usage of standard libc functions over homegrown ones
- No artificial limits
- Do not aim for minimal binary size, but for minimal source code size,
  and maximal readability, reusability, and extensibility.

As a result of that, ipv4, dns, and ipv6 is supported out of the box
and can use the same code, while rocksocks5 has several compile time
defines to bring down the size of the resulting binary to extreme values
like 10 KB static linked when only ipv4 support is enabled.

Still, if optimized for size, *this* program when static linked against musl
libc is not even 50 KB. That's easily usable even on the cheapest routers.

Command-line Options
--------------------

    microsocks -1 -i listenip -p port -u user -P password -b bindaddr

All arguments are optional.
By default listenip is 0.0.0.0 and port 1080.

Option -1 activates auth_once mode: once a specific ip address
authed successfully with user/pass, it is added to a whitelist
and may use the proxy without auth.
This is handy for programs like firefox that don't support
user/pass auth. For it to work you'd basically make one connection
with another program that supports it, and then you can use firefox too.
For example, authenticate once using curl:

    curl --socks5 user:password@listenip:port anyurl


Supported SOCKS5 Features
-------------------------
- Authentication: none, password, one-time
- IPv4, IPv6, DNS
- TCP (no UDP at this time)


How to Compile & Install
------------------------

**Compile and Install on Debian 10 Linux server**

    cd /tmp
    apt-get update
    apt-get install git gcc make -y
    git clone https://github.com/rofl0r/microsocks.git
    cd microsocks
    make

It will generate the binary file ```microsocks``` in /tmp/microsocks/ directory.

To disable logging, instead of only ```make```, use this command (reference [#43](https://github.com/rofl0r/microsocks/issues/43)):

    CPPFLAGS=-DCONFIG_LOG=0 make

If you want to copy/install ```microsocks``` binary file in /usr/bin directory, type:

    make install

**Compile on Microsoft Windows**

You can use [Cygwin](https://www.cygwin.com/) to compile MicroSocks on Windows:

    $ make
    cc  -Wall -std=c99   -c -o sockssrv.o sockssrv.c
    cc  -Wall -std=c99   -c -o server.o server.c
    cc  -Wall -std=c99   -c -o sblist.o sblist.c
    sblist.c:8:2: warning: #warning "your C library sucks." [-Wcpp]
     #warning "your C library sucks."
      ^~~~~~~
    cc  -Wall -std=c99   -c -o sblist_delete.o sblist_delete.c
    cc  sockssrv.o server.o sblist.o sblist_delete.o -lpthread -o microsocks
    
    $ ./microsocks.exe
    client[4] 127.0.0.1: connected to x.x.x.x:443
    
More details on [#4](https://github.com/rofl0r/microsocks/issues/4#issuecomment-408890039)
