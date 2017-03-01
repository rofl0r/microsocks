#ifndef SERVER_H
#define SERVER_H

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
	int fd;
};

int resolve(const char *host, unsigned short port, struct addrinfo** addr);
int server_waitclient(struct server *server, struct client* client);
int server_setup(struct server *server, const char* listenip, unsigned short port);

#endif

