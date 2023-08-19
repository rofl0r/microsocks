/*
   MicroSocks - multithreaded, small, efficient SOCKS5 server.

   Copyright (C) 2017 rofl0r.

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

*/

#define _GNU_SOURCE
#include <unistd.h>
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>

#include "sblist.h"
#include "server.h"
#include "sockssrv.h"

/* timeout in microseconds on resource exhaustion to prevent excessive
   cpu usage. */
#ifndef FAILURE_TIMEOUT
#define FAILURE_TIMEOUT 64
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifdef PTHREAD_STACK_MIN
#define THREAD_STACK_SIZE MAX(8*1024, PTHREAD_STACK_MIN)
#else
#define THREAD_STACK_SIZE 64*1024
#endif

#if defined(__APPLE__)
#undef THREAD_STACK_SIZE
#define THREAD_STACK_SIZE 64*1024
#elif defined(__GLIBC__) || defined(__FreeBSD__) || defined(__sun__)
#undef THREAD_STACK_SIZE
#define THREAD_STACK_SIZE 32*1024
#endif

static int quiet;
static const char* auth_user;
static const char* auth_pass;
static sblist* auth_ips;
static pthread_rwlock_t auth_ips_lock = PTHREAD_RWLOCK_INITIALIZER;
static const struct server* server;

struct thread {
    pthread_t pt;
    struct client client;
    enum socksstate state;
    volatile int  done;
};

struct service_addr {
    enum socks5_addr_type type;
    char* host;
    unsigned short port;
};

#ifndef CONFIG_LOG
#define CONFIG_LOG 1
#endif
#if CONFIG_LOG
/* we log to stderr because it's not using line buffering, i.e. malloc which would need
   locking when called from different threads. for the same reason we use dprintf,
   which writes directly to an fd. */
#define dolog(...) do { if(!quiet) dprintf(2, __VA_ARGS__); } while(0)
#else
static void dolog(const char* fmt, ...) { }
#endif

static int parse_addrport(unsigned char *buf, size_t n, enum socks5_socket_type socktype, union sockaddr_union* addr) {
    if (n < 2) return -EC_GENERAL_FAILURE;
    int af = AF_INET;
    int offset = 0;
    size_t minlen = 1 + 4 + 2, l;
    char namebuf[256];

    switch(buf[0]) {
        case SOCKS5_IPV6: /* ipv6 */
            af = AF_INET6;
            minlen = 1 + 16 + 2;
            /* fall through */
        case SOCKS5_IPV4: /* ipv4 */
            if(n < minlen) return -EC_GENERAL_FAILURE;
            if(namebuf != inet_ntop(af, buf+1, namebuf, sizeof namebuf))
                return -EC_GENERAL_FAILURE; /* malformed or too long addr */
            break;
        case SOCKS5_DNS: /* dns name */
            l = buf[1];
            minlen = 1 + (1 + l) + 2 ;
            if(n < minlen) return -EC_GENERAL_FAILURE;
            memcpy(namebuf, buf+1+1, l);
            namebuf[l] = 0;
            break;
        default:
            return -EC_ADDRESSTYPE_NOT_SUPPORTED;
    }
    
    unsigned short port = (buf[minlen-2] << 8) | buf[minlen-1];
    struct addrinfo *remote;
    if (socktype == TCP_SOCKET) {
        /* there's no suitable errorcode in rfc1928 for dns lookup failure */
        if(resolve_tcp(namebuf, port, &remote)) return -EC_GENERAL_FAILURE;
    } else if (socktype == UDP_SOCKET) {
        if(resolve_udp(namebuf, port, &remote)) return -EC_GENERAL_FAILURE;
    } else {
        abort();
    }

    memcpy(addr, remote->ai_addr, remote->ai_addrlen);
    freeaddrinfo(remote);
    return minlen;
}

static int parse_socks_request_header(unsigned char *buf, size_t n, int* cmd, union sockaddr_union* svc_addr) {
    if(n < 3) return -EC_GENERAL_FAILURE;
    if(buf[0] != VERSION) return -EC_GENERAL_FAILURE;
    if(buf[1] != CONNECT && buf[1] != UDP_ASSOCIATE) return -EC_COMMAND_NOT_SUPPORTED; /* we support only CONNECT and UDP ASSOCIATE method */
    *cmd = buf[1];
    if(buf[2] != RSV) return -EC_GENERAL_FAILURE; /* malformed packet */

    int socktype = *cmd == CONNECT? TCP_SOCKET : UDP_SOCKET;
    int ret = parse_addrport(buf + 3, n - 3, socktype, svc_addr);
    if (ret < 0) return ret;
    return EC_SUCCESS;
}

static int connect_socks_target(union sockaddr_union* remote_addr, struct client *client) {
    int fd = socket(SOCKADDR_UNION_AF(remote_addr), SOCK_STREAM, 0);
    if(fd == -1) {
        eval_errno:
        if(fd != -1) close(fd);
        switch(errno) {
            case ETIMEDOUT:
                return -EC_TTL_EXPIRED;
            case EPROTOTYPE:
            case EPROTONOSUPPORT:
            case EAFNOSUPPORT:
                return -EC_ADDRESSTYPE_NOT_SUPPORTED;
            case ECONNREFUSED:
                return -EC_CONN_REFUSED;
            case ENETDOWN:
            case ENETUNREACH:
                return -EC_NET_UNREACHABLE;
            case EHOSTUNREACH:
                return -EC_HOST_UNREACHABLE;
            case EBADF:
            default:
            perror("socket/connect");
            return -EC_GENERAL_FAILURE;
        }
    }
    if(connect(fd, SOCKADDR_UNION_ADDRESS(remote_addr), SOCKADDR_UNION_LENGTH(remote_addr)) == -1)
        goto eval_errno;

    if(CONFIG_LOG) {
        char clientname[256];
        int af = SOCKADDR_UNION_AF(&client->addr);
        void *ipdata = SOCKADDR_UNION_ADDRESS(&client->addr);
        inet_ntop(af, ipdata, clientname, sizeof clientname);
        char targetname[256];
        af = SOCKADDR_UNION_AF(remote_addr);
        ipdata = SOCKADDR_UNION_ADDRESS(remote_addr);
        inet_ntop(af, ipdata, targetname, sizeof targetname);
        dolog("client[%d] %s: connected to %s:%d\n", client->fd, clientname, 
            targetname, SOCKADDR_UNION_PORT(remote_addr));
    }
    return fd;
}

static int is_authed(union sockaddr_union *client, union sockaddr_union *authedip) {
    int af = SOCKADDR_UNION_AF(authedip);
    if(af == SOCKADDR_UNION_AF(client)) {
        size_t cmpbytes = af == AF_INET ? 4 : 16;
        void *cmp1 = SOCKADDR_UNION_ADDRESS(client);
        void *cmp2 = SOCKADDR_UNION_ADDRESS(authedip);
        if(!memcmp(cmp1, cmp2, cmpbytes)) return 1;
    }
    return 0;
}

static int is_in_authed_list(union sockaddr_union *caddr) {
    size_t i;
    for(i=0;i<sblist_getsize(auth_ips);i++)
        if(is_authed(caddr, sblist_get(auth_ips, i)))
            return 1;
    return 0;
}

static void add_auth_ip(union sockaddr_union *caddr) {
    sblist_add(auth_ips, caddr);
}

static enum authmethod check_auth_method(unsigned char *buf, size_t n, struct client*client) {
    if(buf[0] != 5) return AM_INVALID;
    size_t idx = 1;
    if(idx >= n ) return AM_INVALID;
    int n_methods = buf[idx];
    idx++;
    while(idx < n && n_methods > 0) {
        if(buf[idx] == AM_NO_AUTH) {
            if(!auth_user) return AM_NO_AUTH;
            else if(auth_ips) {
                int authed = 0;
                if(pthread_rwlock_rdlock(&auth_ips_lock) == 0) {
                    authed = is_in_authed_list(&client->addr);
                    pthread_rwlock_unlock(&auth_ips_lock);
                }
                if(authed) return AM_NO_AUTH;
            }
        } else if(buf[idx] == AM_USERNAME) {
            if(auth_user) return AM_USERNAME;
        }
        idx++;
        n_methods--;
    }
    return AM_INVALID;
}

static void send_auth_response(int fd, int version, enum authmethod meth) {
    unsigned char buf[2];
    buf[0] = version;
    buf[1] = meth;
    write(fd, buf, 2);
}

static ssize_t send_response(int fd, enum errorcode ec, union sockaddr_union* addr) {
    // IPv6 takes 22 bytes, which is the longest
    char buf[4 + 16 + 2] = {VERSION, ec, RSV};
    size_t len = 0;
    if (SOCKADDR_UNION_AF(addr) == AF_INET) {
        buf[3] = SOCKS5_IPV4;
        memcpy(buf+4, SOCKADDR_UNION_ADDRESS(addr), 4);
        buf[8] = SOCKADDR_UNION_PORT(addr) >> 8;
        buf[9] = SOCKADDR_UNION_PORT(addr) | 0xFF;
        len = 10;
    } else if (SOCKADDR_UNION_AF(addr) == AF_INET6) {
        buf[3] = SOCKS5_IPV6;
        memcpy(buf+4, SOCKADDR_UNION_ADDRESS(addr), 16);
        buf[20] = SOCKADDR_UNION_PORT(addr) >> 8;
        buf[21] = SOCKADDR_UNION_PORT(addr) | 0xFF;
        len = 22;
    } else {
        abort();
    }
    return write(fd, buf, len);
}

static void send_error(int fd, enum errorcode ec) {
    /* position 4 contains ATYP, the address type, which is the same as used in the connect
       request. we're lazy and return always IPV4 address type in errors. */
    char buf[10] = { 5, ec, 0, 1 /*AT_IPV4*/, 0,0,0,0, 0,0 };
    write(fd, buf, 10);
}

static void copyloop(int fd1, int fd2) {
    struct pollfd fds[2] = {
        [0] = {.fd = fd1, .events = POLLIN},
        [1] = {.fd = fd2, .events = POLLIN},
    };

    while(1) {
        /* inactive connections are reaped after 15 min to free resources.
           usually programs send keep-alive packets so this should only happen
           when a connection is really unused. */
        switch(poll(fds, 2, 60*15*1000)) {
            case 0:
                return;
            case -1:
                if(errno == EINTR || errno == EAGAIN) continue;
                else perror("poll");
                return;
        }
        int infd = (fds[0].revents & POLLIN) ? fd1 : fd2;
        int outfd = infd == fd2 ? fd1 : fd2;
        char buf[1024];
        ssize_t sent = 0, n = read(infd, buf, sizeof buf);
        if(n <= 0) return;
        while(sent < n) {
            ssize_t m = write(outfd, buf+sent, n-sent);
            if(m < 0) return;
            sent += m;
        }
    }
}

// caller must free socks5_addr manually
static ssize_t extract_udp_data(char* buf, ssize_t n, void** socks5_addr, union sockaddr_union* target_addr, char** data) {
    if (n < 3) return -EC_GENERAL_FAILURE;
    if (buf[0] != RSV || buf[1] != RSV) return -EC_GENERAL_FAILURE;
    if (buf[2] != RSV) return -EC_GENERAL_FAILURE;  // framentation not supported

    buf += 3, n -= 3;
    int offset = parse_addrport(buf, n, UDP_SOCKET, target_addr);
    if (offset < 0) {
        return offset;
    }
    *socks5_addr = malloc(offset);
    memcpy(*socks5_addr, buf, offset);
    *data = buf + offset;
    n -= offset;
    return n;
}

// the returned buffer must be manually freed
static ssize_t prepare_udp_data(char* data, ssize_t n1, int reserved_size, union sockaddr_union* client_addr) {
    const reserved = 0;
    char buf[reserved];
    buf[0] = RSV, buf[1]= RSV;

    return 0;
}

struct fd_socks5addr {
    int fd;
    void* socks5_addr;
    size_t addr_len;
};

int compare_fd_socks5addr_by_fd(char* item1, char* item2) {
    struct fd_socks5addr* i1 = ( struct fd_socks5addr*)item1;
    struct fd_socks5addr* i2 = ( struct fd_socks5addr*)item2;
    if (i1->fd == i2->fd) return 0;
    return 1;
}

int compare_fd_socks5addr_by_addr(char* item1, char* item2) {
    struct fd_socks5addr* i1 = ( struct fd_socks5addr*)item1;
    struct fd_socks5addr* i2 = ( struct fd_socks5addr*)item2;
    if (i1->addr_len == i2->addr_len && \
        memcmp(i1->socks5_addr, i2->socks5_addr, i1->addr_len) == 0) return 0;
    return 1;
}

static void copy_loop_udp(int tcp_fd, int udp_fd) {
    // add tcp_fd and udp_fd to poll    
    int poll_fds = 2;
    struct pollfd fds[1024] = {
        [0] = {.fd = tcp_fd, .events = POLLIN},
        [1] = {.fd = udp_fd, .events = POLLIN},
    };

    ssize_t n;
    struct fd_socks5addr item;
    // RSV(2) + FRAG(1) + ATYP(1) + DST.ADDR(1 + MAX_DNS_LEN) + DST.PORT(2)
    const max_header_size = 2 + 1 + 1 + 1 + MAX_DNS_LEN + 2;

    struct sblist* sock_list = sblist_new(sizeof(struct fd_socks5addr), 2);
    while(1) {
        switch(poll(fds, poll_fds, 60*15*1000)) {
            case 0:
                return;
            case -1:
                if(errno == EINTR || errno == EAGAIN) continue;
                else perror("poll");
                return;
        }
        char buf[4096];  // support up to 4K worth of UDP message
        // TCP socket
        if (fds[0].revents & POLLIN) {
            n = read(fds[0].fd, buf, sizeof(buf) - 1);
            if (n == 0) {
                // SOCKS5 TCP connection closed
                goto UDP_LOOP_END;
            }
            if (n == -1) {
                if(errno == EINTR || errno == EAGAIN) continue;
                else perror("read");
                goto UDP_LOOP_END;
            }
            buf[n - 1] = '\0';
            dprintf(1, "received unexpectedly from TCP socket after UDP associate: %s", buf);
        }
        // client UDP socket
        if (fds[1].revents & POLLIN) {
            union sockaddr_union addr;
            n = recv(udp_fd, buf, sizeof(buf) - 1, 0);
            if (n == -1) {
                if(errno == EINTR || errno == EAGAIN) continue;
                perror("recv");
                goto UDP_LOOP_END;
            }
        
            union sockaddr_union target_addr;
            void* socks5_addr;
            char* data;
            n = extract_udp_data(buf, n, &target_addr, &socks5_addr, &data);
            if (n < 0) {
                dprintf(2, "failed to extract udp data, %d", n);
                goto UDP_LOOP_END;
            }
            if (n == 0) {
                dprintf(2, "malformed udp packet with no data, %d", n);
            } else {
                int send_fd = 0;
                item.socks5_addr = socks5_addr;
                item.addr_len = strlen(socks5_addr);
                int idx = sblist_search(sock_list, &item, compare_fd_socks5addr_by_addr);
                if (idx != -1) {
                    struct fd_socks5addr* item = (struct fd_socks5addr*)sblist_item_from_index(sock_list, idx);
                    send_fd = item->fd;
                    item->socks5_addr = NULL;
                    free(socks5_addr);
                } else {
                    // create a new socket
                    int fd = socket(SOCKADDR_UNION_AF(&target_addr), SOCK_DGRAM, 0);
                    if (connect(fd, (const struct sockaddr*)&target_addr, sizeof(target_addr))) {
                        perror("connect");
                        send_error(tcp_fd, EC_GENERAL_FAILURE);
                        // just out of fd1
                    }
                    // save the data into buffer list
                    item.fd = fd;
                    // no need to free socks5_addr now
                    item.socks5_addr = socks5_addr;
                    item.addr_len = strlen(socks5_addr);
                    sblist_add(sock_list, &item);
                    // add to polling fds
                    fds[poll_fds].fd = fd;
                    fds[poll_fds].events = POLL_IN;
                    poll_fds++;
                    send_fd = fd;
                }
                n = send(send_fd, data, n, 0);
                if (n < 0) {
                    perror("sendto");
                    goto UDP_LOOP_END;
                }
            }
        }

        int i;
        for (i = 0; i < poll_fds; i++) {
            if (fds[i].revents & POLLIN) {
                item.fd = fds[i].fd;
                int idx = sblist_search(sock_list, &item, compare_fd_socks5addr_by_fd);
                if (idx == -1) {
                    perror("socket not found");
                    goto UDP_LOOP_END;
                }
                struct fd_socks5addr *item = (struct fd_socks5addr*)sblist_item_from_index(sock_list, idx);
                void* socks5_addr = item->socks5_addr;
                int len = item->addr_len;

                int header_size = 2 + 1 + len;
                buf[0] = RSV, buf[1] = RSV;
                buf[2] = 0; // FRAG
                memcpy(buf + 3, item->socks5_addr, len);
                n = recv(fds[i].fd, buf + header_size, sizeof(buf) - header_size, 0);
                if(n <= 0) {
                    perror("read from middle_fd");
                    goto UDP_LOOP_END;
                }
                ssize_t m = write(udp_fd, buf, header_size + n);
                if(m < 0) {
                    perror("write to udp_fd");
                    goto UDP_LOOP_END;
                }
            }
        }
    }
UDP_LOOP_END:
    int i;
    for (i = 2; i < poll_fds; i++) close(fds[i].fd);
}

static enum errorcode check_credentials(unsigned char* buf, size_t n) {
    if(n < 5) return EC_GENERAL_FAILURE;
    if(buf[0] != 1) return EC_GENERAL_FAILURE;
    unsigned ulen, plen;
    ulen=buf[1];
    if(n < 2 + ulen + 2) return EC_GENERAL_FAILURE;
    plen=buf[2+ulen];
    if(n < 2 + ulen + 1 + plen) return EC_GENERAL_FAILURE;
    char user[256], pass[256];
    memcpy(user, buf+2, ulen);
    memcpy(pass, buf+2+ulen+1, plen);
    user[ulen] = 0;
    pass[plen] = 0;
    if(!strcmp(user, auth_user) && !strcmp(pass, auth_pass)) return EC_SUCCESS;
    return EC_NOT_ALLOWED;
}

unsigned short pick_random_port() { return 10000; }

int udp_svc_setup(union sockaddr_union* client_addr) {
    int fd = socket(SOCKADDR_UNION_AF(client_addr), SOCK_DGRAM, 0);
    if(fd == -1) {
        eval_errno:
        if(fd != -1) close(fd);
        switch(errno) {
            case ETIMEDOUT:
                return -EC_TTL_EXPIRED;
            case EPROTOTYPE:
            case EPROTONOSUPPORT:
            case EAFNOSUPPORT:
                return -EC_ADDRESSTYPE_NOT_SUPPORTED;
            case ECONNREFUSED:
                return -EC_CONN_REFUSED;
            case ENETDOWN:
            case ENETUNREACH:
                return -EC_NET_UNREACHABLE;
            case EHOSTUNREACH:
                return -EC_HOST_UNREACHABLE;
            case EBADF:
            default:
                perror("socket/connect");
                return -EC_GENERAL_FAILURE;
        }
    }
    if (connect(fd, (const struct sockaddr*)client_addr, sizeof(union sockaddr_union))) {
        perror("connect");
        return -1;
    }

    return fd;
}

static void* clientthread(void *data) {
    struct thread *t = data;
    t->state = SS_1_CONNECTED;
    unsigned char buf[1024];
    ssize_t n;
    int ret;
    // for CONNECT, this is target TCP address
    // for UDP ASSOCIATE, this is client UDP address
    union sockaddr_union address, local_addr;
    struct addrinfo* remote;

    enum authmethod am;
    while((n = recv(t->client.fd, buf, sizeof buf, 0)) > 0) {
        switch(t->state) {
            case SS_1_CONNECTED:
                am = check_auth_method(buf, n, &t->client);
                if(am == AM_NO_AUTH) t->state = SS_3_AUTHED;
                else if (am == AM_USERNAME) t->state = SS_2_NEED_AUTH;
                send_auth_response(t->client.fd, 5, am);
                if(am == AM_INVALID) goto breakloop;
                break;
            case SS_2_NEED_AUTH:
                ret = check_credentials(buf, n);
                send_auth_response(t->client.fd, 1, ret);
                if(ret != EC_SUCCESS)
                    goto breakloop;
                t->state = SS_3_AUTHED;
                if(auth_ips && !pthread_rwlock_wrlock(&auth_ips_lock)) {
                    if(!is_in_authed_list(&t->client.addr))
                        add_auth_ip(&t->client.addr);
                    pthread_rwlock_unlock(&auth_ips_lock);
                }
                break;
            case SS_3_AUTHED:
                int cmd;
                ret = parse_socks_request_header(buf, n, &cmd, &address);
                if (ret != EC_SUCCESS) {
                    goto breakloop;
                }
                
                if (cmd == CONNECT) {
                    ret = connect_socks_target(&address, &t->client);
                    if(ret < 0) {
                        send_error(t->client.fd, ret*-1);
                        goto breakloop;
                    }
                    int remotefd = ret;
                    socklen_t len = sizeof(union sockaddr_union);
                    if (getsockname(remotefd, (struct sockaddr*)&local_addr, &len)) return -EC_GENERAL_FAILURE;
                    if (-1 == send_response(t->client.fd, EC_SUCCESS, &local_addr)) {
                        close(remotefd);
                        goto breakloop;
                    }
                    copyloop(t->client.fd, remotefd);
                    close(remotefd);
                    goto breakloop;
                } else if (cmd == UDP_ASSOCIATE) {
                    int fd = udp_svc_setup(&address);
                    if(fd <= 0) {
                        send_error(t->client.fd, fd*-1);
                        goto breakloop;
                    }

                    socklen_t len = sizeof(union sockaddr_union);
                    if (getsockname(fd, (struct sockaddr*)&local_addr, &len)) return -EC_GENERAL_FAILURE;
                    if (-1 == send_response(t->client.fd, EC_SUCCESS, &local_addr)) {
                        close(fd);
                        goto breakloop;
                    }
                    copy_loop_udp(t->client.fd, fd);
                    close(fd);
                    goto breakloop;
                } else {
                    // should not be here
                    abort();
                }
        }
    }
breakloop:
    if (!remote) freeaddrinfo(remote);

    close(t->client.fd);
    t->done = 1;

    return 0;
}

static void collect(sblist *threads) {
    size_t i;
    for(i=0;i<sblist_getsize(threads);) {
        struct thread* thread = *((struct thread**)sblist_get(threads, i));
        if(thread->done) {
            pthread_join(thread->pt, 0);
            sblist_delete(threads, i);
            free(thread);
        } else
            i++;
    }
}

static int usage(void) {
    dprintf(2,
        "MicroSocks SOCKS5 Server\n"
        "------------------------\n"
        "usage: microsocks -1 -q -i listenip -p port -u user -P password -b bindaddr\n"
        "all arguments are optional.\n"
        "by default listenip is 0.0.0.0 and port 1080.\n\n"
        "option -q disables logging.\n"
        "option -b specifies which ip outgoing connections are bound to\n"
        "option -1 activates auth_once mode: once a specific ip address\n"
        "authed successfully with user/pass, it is added to a whitelist\n"
        "and may use the proxy without auth.\n"
        "this is handy for programs like firefox that don't support\n"
        "user/pass auth. for it to work you'd basically make one connection\n"
        "with another program that supports it, and then you can use firefox too.\n"
    );
    return 1;
}

/* prevent username and password from showing up in top. */
static void zero_arg(char *s) {
    size_t i, l = strlen(s);
    for(i=0;i<l;i++) s[i] = 0;
}

int main(int argc, char** argv) {
    int ch;
    const char *listenip = "0.0.0.0";
    unsigned port = 1080;
    while((ch = getopt(argc, argv, ":1qi:p:u:P:")) != -1) {
        switch(ch) {
            case '1':
                auth_ips = sblist_new(sizeof(union sockaddr_union), 8);
                break;
            case 'q':
                quiet = 1;
                break;
            case 'u':
                auth_user = strdup(optarg);
                zero_arg(optarg);
                break;
            case 'P':
                auth_pass = strdup(optarg);
                zero_arg(optarg);
                break;
            case 'i':
                listenip = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case ':':
                dprintf(2, "error: option -%c requires an operand\n", optopt);
                /* fall through */
            case '?':
                return usage();
        }
    }
    if((auth_user && !auth_pass) || (!auth_user && auth_pass)) {
        dprintf(2, "error: user and pass must be used together\n");
        return 1;
    }
    if(auth_ips && !auth_pass) {
        dprintf(2, "error: auth-once option must be used together with user/pass\n");
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);
    struct server s;
    sblist *threads = sblist_new(sizeof (struct thread*), 8);
    if(server_setup(&s, listenip, port)) {
        perror("server_setup");
        return 1;
    }
    server = &s;

    while(1) {
        collect(threads);
        struct client c;
        struct thread *curr = malloc(sizeof (struct thread));
        if(!curr) goto oom;
        curr->done = 0;
        if(server_waitclient(&s, &c)) {
            dolog("failed to accept connection\n");
            free(curr);
            usleep(FAILURE_TIMEOUT);
            continue;
        }
        curr->client = c;
        if(!sblist_add(threads, &curr)) {
            close(curr->client.fd);
            free(curr);
            oom:
            dolog("rejecting connection due to OOM\n");
            usleep(FAILURE_TIMEOUT); /* prevent 100% CPU usage in OOM situation */
            continue;
        }
        pthread_attr_t *a = 0, attr;
        if(pthread_attr_init(&attr) == 0) {
            a = &attr;
            pthread_attr_setstacksize(a, THREAD_STACK_SIZE);
        }
        if(pthread_create(&curr->pt, a, clientthread, curr) != 0)
            dolog("pthread_create failed. OOM?\n");
        if(a) pthread_attr_destroy(&attr);
    }
}
