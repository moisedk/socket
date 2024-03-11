/* socket.c: Simple Socket Functions */

#include "spidey.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * Allocate socket, bind it, and listen to specified port.
 **/
int
socket_listen(const char *port)
{
    struct addrinfo  hints = {
        .ai_family = AF_UNSPEC, // Return both IPv4 and IPv6
        .ai_socktype = SOCK_STREAM, // Use TCP
        .ai_flags = AI_PASSIVE // Socket is used for accepting connection on all interfaces.
    };
    struct addrinfo *results;
    int    socket_fd = -1;

    /* Lookup server address information */
    int status;
    if ((status = getaddrinfo(NULL, port, &hints, &results)) < 0 ) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(status));
        return status;
    }
    /* For each server entry, allocate socket and try to connect */
    for (struct addrinfo *p = results; p != NULL && socket_fd < 0; p = p->ai_next) {
	    /* Allocate socket */
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            fprintf("Failed to make socket: %s\n", strerror(errno));
            continue;
        }
        /* Bind socket */
        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) < 0) {
            fprintf("Failed to bind socket: %s\n", strerror(errno));
            close(socket_fd);
            socket_fd = -1;
            continue;
        }
    	/* Listen to socket */
        if (listen(socket_fd, SOMAXCONN) < 0) {
            fprintf("Failed to listen: %s\n", strerror(errno));
            close(socket_fd);
            socket_fd = -1;
            continue;
        }
    }

    freeaddrinfo(results);
    return socket_fd;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
