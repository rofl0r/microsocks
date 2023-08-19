#ifndef SOCKSSRV_H
#define SOCKSSRV_H

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

enum socksstate {
    SS_1_CONNECTED,
    SS_2_NEED_AUTH, /* skipped if NO_AUTH method supported */
    SS_3_AUTHED,
};

enum authmethod {
    AM_NO_AUTH = 0,
    AM_GSSAPI = 1,
    AM_USERNAME = 2,
    AM_INVALID = 0xFF
};

enum errorcode {
    EC_SUCCESS = 0,
    EC_GENERAL_FAILURE = 1,
    EC_NOT_ALLOWED = 2,
    EC_NET_UNREACHABLE = 3,
    EC_HOST_UNREACHABLE = 4,
    EC_CONN_REFUSED = 5,
    EC_TTL_EXPIRED = 6,
    EC_COMMAND_NOT_SUPPORTED = 7,
    EC_ADDRESSTYPE_NOT_SUPPORTED = 8,
    EC_BIND_IP_NOT_PROVIDED = 9,
};

enum socks5_cmd {
    CONNECT = 1,
    UDP_ASSOCIATE = 3,
};

const int VERSION = 5;
const int RSV = 0;

enum socks5_addr_type {
    SOCKS5_ADDR_UNKNOWN = 0,
    SOCKS5_IPV4 = 1,
    SOCKS5_DNS = 3,
    SOCKS5_IPV6 = 4,
};

enum socks5_socket_type {
    TCP_SOCKET = 1,
    UDP_SOCKET =2,
};

#define MAX_DNS_LEN  (2 << 8 - 1)

#endif
