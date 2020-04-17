MicroSocks - multithreaded, small, efficient SOCKS5 server.
===========================================================
![Build / Release](https://github.com/OnceUponALoop/microsocks/workflows/Build%20/%20Release/badge.svg?branch=master)

A SOCKS5 service that you can run on your remote boxes to tunnel connections
through them, if for some reason SSH doesn't cut it for you.

It's very lightweight, and very light on resources too:

for every client, a thread with a stack size of 8KB is spawned.
the main process basically doesn't consume any resources at all.

the only limits are the amount of file descriptors and the RAM.

It's also designed to be robust: it handles resource exhaustion
gracefully by simply denying new connections, instead of calling abort()
as most other programs do these days.

another plus is ease-of-use: no config file necessary, everything can be
done from the command line and doesn't even need any parameters for quick
setup.

History
-------

This is the successor of "rocksocks5", and it was written with
different goals in mind:

- prefer usage of standard libc functions over homegrown ones
- no artificial limits
- do not aim for minimal binary size, but for minimal source code size,
  and maximal readability, reusability, and extensibility.

as a result of that, ipv4, dns, and ipv6 is supported out of the box
and can use the same code, while rocksocks5 has several compile time
defines to bring down the size of the resulting binary to extreme values
like 10 KB static linked when only ipv4 support is enabled.

still, if optimized for size, *this* program when static linked against musl
libc is not even 50 KB. that's easily usable even on the cheapest routers.

Build
------------------------

To build the microsocks from source

1. Install dependencies

    We need the autotools and autotool-archive as dependencies, in a CentOS/RHEL system this can be installed as follows
    
    ```
    yum install autoconf automake autotools-archive
    ```

1. Build

    ```
    ./autogen.sh
    ./configure
    make
    ```

2. Create distribution tarball

    ```
    make dist
    ```

3. [Optional] Test Installation

    ```
    make distcheck
    ```

Build rpm package
------------------------
We can use the [tito project](https://github.com/dgoodwin/tito) to build an rpm from the git repo directly as follows.

1. Install Dependencies

    ```
    yum install tito
    ```

1. Run tito in test mode

    ```
    tito --rpm --offline --test
    ```

1. Run tito after making changes and checking in code

    ``` bash
    tito tag
    tito build --rpm
    ```

    We can also use tito to chain into mock to create rpms for other dists.

    ``` bash
    tito build                 \
      --rpm                    \
      --builder mock           \
      --arg mock=epel-7-x86_64 \
      --output /tmp/results
    ```

1. Run mock to generate rpm for other distributions

    Github Actions doesn't support Fedora variants so we're unable to use tito but we can still
    leverage mock for multiple-dist support by calling it directly

    ``` bash
    # Create SRPM
    mock                     \
      -r epel-7-x86_64       \
      --buildsrpm            \
      --spec microsocks.spec \
      --sources .            \
      --resultdir=/tmp/results

    # Create RPM
    mock \
      -r epel-7-x86_64 \
      --rebuild /tmp/results/*.src.rpm  \
      --resultdir=/tmp/results
    ```
    
Usage
------------------------
```
Usage:
  microsocks [options]
Options:
  -b            Bind outgoing connections to the listening ip
                (defined by -i)

  -i <ip addr>  The ip address the server listens to for connections
                (default: 0.0.0.0)

  -p <port num> The port the server listens to for connections
                (default: 1080)

  -u <username> Authentication username, used to auth proxy clients
                (default: not set)

  -P <password> Authentication password, used to auth proxy clients
                (default: not set)
                
  -1            Activates auth_once mode. 
                Once a specific ip address successfully authenticates with 
                user/pass, it is added to a whitelist and may use the proxy
                without auth.
                This is handy for programs like firefox that don't support 
                user/pass auth. for it to work you'd basically make one 
                connection with another program that supports it, and then 
                you can use firefox too.
                (default: disabled)
```

Systemd
------------------------

Systemd support will automatically be enabled if support is detected during the build. When enabled a service unit will be installed in `/usr/lib/systemd/system/microsocks.service`.

Service usage examples

- Start the service

    ```
    systemctl start microsocks.service
    ```

- Stop the service
    
    ```
    systemctl stop microsocks.service
    ```

- Check service status

    ```
    systemctl status microsocks.service
    ```

- Change service options

    The command line arguments the service will use when started are defined in `/etc/sysconfig/microsocks`

    To change the options edit the file and restart the service.



