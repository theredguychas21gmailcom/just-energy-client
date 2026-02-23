/**
 * @file client_tcp.c
 * @brief TCP client implementation
 *
 * Implementation of the TCP client interface defined in client_tcp.h.
 * See client_tcp.h for detailed API documentation.
 */
#include "client_tcp.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

ClientTCP* client_tcp_create() {
    ClientTCP* tcp = malloc(sizeof(ClientTCP));
    if (!tcp) {
        return NULL;
    }
    tcp->fd = -1;
    return tcp;
}

void client_tcp_destroy(ClientTCP* tcp) {
    if (!tcp) {
        return;
    }
    client_tcp_close(tcp);
    free(tcp);
}

/* int client_tcp_connect(ClientTCP* tcp, const char* host, int port,
                       int timeout_ms) {
    if (!tcp || !host) {
        return -1;
    }

    if (tcp->fd >= 0) {
        return -1;
    }

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo  hints = {0};
    struct addrinfo* res   = NULL;

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int gai_result = getaddrinfo(host, port_str, &hints, &res);
    if (gai_result != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(gai_result));
        return -1;
    }

    int fd = -1;
    for (struct addrinfo* rp = res; rp; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) {
            continue;
        }

        int flags = fcntl(fd, F_GETFL, 0);
        // fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        int connect_result = connect(fd, rp->ai_addr, rp->ai_addrlen);

        if (connect_result == 0) {
            break;
        }

        if (errno == EINPROGRESS) {
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(fd, &write_fds);

            struct timeval tv;
            tv.tv_sec  = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;

            int select_result = select(fd + 1, NULL, &write_fds, NULL, &tv);

            if (select_result > 0) {
                int       error     = 0;
                socklen_t error_len = sizeof(error);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &error_len) ==
                        0 &&
                    error == 0) {
                    break;
                }
            }
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);

    if (fd < 0) {
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    tcp->fd = fd;
    return 0;
} */

int client_tcp_connect_async(ClientTCP* tcp, const char* host, int port) {
    if (!tcp || !host || tcp->fd >= 0) return -1;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, port_str, &hints, &res) != 0) return -1;

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    int rc = connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    tcp->fd = fd;
    if (rc == 0) {
        tcp->state = TCP_STATE_CONNECTED;
        return 0; 
    } else if (errno == EINPROGRESS) {
        tcp->state = TCP_STATE_CONNECTING;
        return 1; 
    }

    close(fd);
    tcp->fd = -1;
    return -1; 
}

ssize_t client_tcp_send_async(ClientTCP* tcp, const void* data, size_t len) {
    if (tcp->fd < 0 || tcp->state != TCP_STATE_CONNECTED) return -1;
    ssize_t sent = send(tcp->fd, data, len, MSG_NOSIGNAL);
    
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; 
        }
        return -1;
    }

    if ((size_t)sent < len) {
        size_t remaining = len - sent;
    }

    return sent;
}

int client_tcp_recv(ClientTCP* tcp, void* buffer, size_t len, int timeout_ms) {
    if (!tcp || tcp->fd < 0) 
        return -1;

    ssize_t received = recv(tcp->fd, buffer, len, 0);

    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; 
        }
        return -1; 
    }

    if (recieved == 0) {
        tcp->state = TCP_STATE_DISCONNECTED;
        return -1
    }

    return (int)received;
}

void client_tcp_close(ClientTCP* tcp) {
    if (!tcp || tcp->fd < 0) {
        return;
    }
    close(tcp->fd);
    tcp->fd = -1;
}
