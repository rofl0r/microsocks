#include "server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>

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

int resolve_sa(const char *host, unsigned short port, union sockaddr_union *res) {
	struct addrinfo *ainfo = 0;
	int ret;
	SOCKADDR_UNION_AF(res) = AF_UNSPEC;
	if((ret = resolve(host, port, &ainfo))) return ret;
	memcpy(res, ainfo->ai_addr, ainfo->ai_addrlen);
	freeaddrinfo(ainfo);
	return 0;
}

int bindtoip(int fd, union sockaddr_union *bindaddr) {
	socklen_t sz = SOCKADDR_UNION_LENGTH(bindaddr);
	if(sz)
		return bind(fd, (struct sockaddr*) bindaddr, sz);
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
	int listenfd = -1;
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
	freeaddrinfo(ainfo);
	if(listenfd < 0) return -2;
	if(listen(listenfd, SOMAXCONN) < 0) {
		close(listenfd);
		return -3;
	}
	server->fd = listenfd;
	return 0;
}

int server_setup_unix(struct server *server, const char* path) {
	socklen_t addrlen = offsetof(struct sockaddr_un, sun_path) + strlen(path) + 1;
	if(addrlen < strlen(path)) { // not possible in practice
		errno = ENAMETOOLONG;
		return -1;
	}

	struct sockaddr_un *addr = calloc(1, addrlen);
	if(!addr) {
		return -1;
	}
	addr->sun_family = AF_UNIX;
	strcpy(addr->sun_path, path);

	int listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listenfd < 0) {
		free(addr);
		return -2;
	}

	// note: binding to "" lets the kernel choose a random address, just like IP port 0
	if(bind(listenfd, (struct sockaddr*)addr, addrlen) < 0) {
		close(listenfd);
		free(addr);
		return -2;
	}
	free(addr);

	if(listen(listenfd, SOMAXCONN) < 0) {
		close(listenfd);
		return -3;
	}

	server->fd = listenfd;
	return 0;
}
