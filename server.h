#ifndef SERVER_H
#define SERVER_H

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>
#include <netdb.h>

//RcB: DEP "server.c"

union sockaddr_union {
	struct sockaddr_in  v4;
	struct sockaddr_in6 v6;
};

struct client {
	union sockaddr_union addr;
	int fd;
};

struct server {
	union sockaddr_union bindaddr;
	int fd;
	socklen_t bindaddrsz;
};

int resolve(const char *host, unsigned short port, struct addrinfo** addr);
int server_bindtoip(const struct server *server, int fd);
int server_waitclient(struct server *server, struct client* client);
int server_setup(struct server *server, const char* listenip, unsigned short port);

#endif

