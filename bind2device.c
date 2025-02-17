#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#define _GNU_SOURCE
#define _DARWIN_C_SOURCE

#include <errno.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "bind2device.h"

#if (defined(IP_BOUND_IF) || defined(IPV6_BOUND_IF))

int bind2device(int sockfd, int socket_family, const char *device)
{
	int ifindex = if_nametoindex(device);
	if (ifindex == 0)
		return -1;
	switch (socket_family)
	{
#if defined(IPV6_BOUND_IF)
	case AF_INET6:
		return setsockopt(sockfd, IPPROTO_IPV6, IPV6_BOUND_IF, &ifindex, sizeof(ifindex));
#endif
#if defined(IP_BOUND_IF)
	case AF_INET:
		return setsockopt(sockfd, IPPROTO_IP, IP_BOUND_IF, &ifindex, sizeof(ifindex));
#endif
	default: // can't bind to interface for selected socket_family: operation not supported on socket
		errno = EOPNOTSUPP;
		return -1;
	}
}

#elif defined(SO_BINDTODEVICE)

int bind2device(int sockfd, int socket_family, const char *device)
{
	return setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, device, strlen(device) + 1);
}

#else
#pragma message "Platform does not support bind2device, generating stub."

int bind2device(int sockfd, int socket_family, const char *device)
{
	errno = ENOSYS; // unsupported platform: not implemented
	return -1;
}

#endif
