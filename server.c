#include "server.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int resolve(const char *host, unsigned short port, struct addrinfo** addr) {
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = AI_PASSIVE,
	};
	char port_buf[8];
	snprintf(port_buf, sizeof port_buf, "%u", port);
	return getaddrinfo(host, port_buf, &hints, addr);
}

int server_bindtoip(const struct server *server, int fd) {
	if(server->bindaddr.v4.sin_family != AF_UNSPEC)
		return bind(fd, (struct sockaddr*) &server->bindaddr, server->bindaddrsz);
	return 0;
}

int server_waitclient(struct server *server, struct client* client) {
	socklen_t clen = sizeof client->addr;
	return ((client->fd = accept(server->fd, (void*)&client->addr, &clen)) == -1)*-1;
}

int server_setup(struct server *server, const char* listenip, unsigned short port) {
	struct addrinfo *ainfo = 0;
	if(resolve(listenip, port, &ainfo)) return -1;
	struct addrinfo* p;
	int listenfd;
	for(p = ainfo; p; p = p->ai_next) {
		if((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
			continue;
		int yes = 1;
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if(bind(listenfd, p->ai_addr, p->ai_addrlen) < 0) {
			close(listenfd);
			listenfd = -1;
			continue;
		}
		break;
	}
	if(listenfd < 0) return -2;
	freeaddrinfo(ainfo);
	if(listen(listenfd, SOMAXCONN) < 0) {
		close(listenfd);
		return -3;
	}
	server->fd = listenfd;
	if(!resolve(listenip, 0, &ainfo)) {
		server->bindaddrsz = ainfo->ai_addrlen;
		memcpy(&server->bindaddr, ainfo->ai_addr, ainfo->ai_addrlen);
		freeaddrinfo(ainfo);
	} else server->bindaddr.v4.sin_family = AF_UNSPEC;
	return 0;
}
