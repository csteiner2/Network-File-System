/**
 * netfs_server.h
 *
 * NetFS file server implementation.
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h> 
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "logging.h"

struct __attribute__((__packed__)) netfs_msg_header {
    uint64_t msg_len;
    uint16_t msg_type;
};

int main(int argc, char *argv[]) {

    /* This starter code will initialize the server port and wait to receive a
     * message. */

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DEFAULT_PORT);
    if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(socket_fd, 10) == -1) {
        perror("listen");
        return 1;
    }

    LOG("Listening on port %d\n", DEFAULT_PORT);

    struct sockaddr_storage client_addr = { 0 };
    socklen_t slen = sizeof(client_addr);

    int client_fd = accept(
            socket_fd,
            (struct sockaddr *) &client_addr,
            &slen);

    if (client_fd == -1) {
        perror("accept");
        return 1;
    }

    char remote_host[INET_ADDRSTRLEN];
    inet_ntop(
            client_addr.ss_family,
            (void *) &(((struct sockaddr_in *) &client_addr)->sin_addr),
            remote_host,
            sizeof(remote_host));
    LOG("Accepted connection from %s\n", remote_host);

    /* Now that the connection is established, we should probably read() from
     * the socket... */

    return 0; 
}
