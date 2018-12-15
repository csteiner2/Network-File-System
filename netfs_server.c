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
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include "common.h"
#include "logging.h"
#include "net.h"

void handle_request(int fd) {
    struct netfs_msg_header req_header = { 0 };
    read_len(fd, &req_header, sizeof(struct netfs_msg_header));
    LOG("Handling request: [type %d; length %lld]\n",
        req_header.msg_type,
        req_header.msg_len);
    uint16_t type = req_header.msg_type;
    if (type == MSG_READDIR) {
        //define largest in common.h
        char path[1024] = { 0 };
        read_len(fd, path, req_header.msg_len);
        LOG("readdir: %s\n", path);

        char full_path[1024] = { 0 };
        strcpy(full_path, ".");
        strcat(full_path, path);

        DIR *directory;
        if ((directory = opendir(full_path)) == NULL) {
            perror("opendir");
            close(fd);
            return;
        }

        uint16_t len;
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL) {
            len = strlen(entry->d_name) +1;
            write_len(fd, &len, sizeof(uint16_t));
            write_len(fd, entry->d_name, len);
           // printf("-> %s\n", entry->d_name);
        }
        len = 0;
        write_len(fd, &len, sizeof(uint16_t));

        closedir(directory);
        close(fd);
        return;
    
    }else if(type == MSG_GETATTR) {
        char path[1024] = { 0 };
        read_len(fd, path, req_header.msg_len);
       // read_len(fd, path, req_header->msg_len);
        LOG("getattr: %s\n", path);
        char full_path[1024] = { 0 };
        strcpy(full_path, ".");
        strcat(full_path, path);

        struct stat *stbuf = malloc(sizeof(struct stat));
        int result = stat(full_path, stbuf);
        if (result == -1) {
            perror("result from stat is -l\n");
            free(stbuf);
            close(fd);
            return;
        }

        write_len(fd, &result, sizeof(int));
        //int uid = stbuf->uid_t;
        if(stbuf->st_uid != getuid()){
            stbuf->st_uid = 0;
        } else{
            stbuf->st_uid = 1;
        }
        
        //if its not sent by the user send a zero
        // stbuf->st_mode = (mode_t) (~0222 & remote_st.st_mode);
        if(result != -1){
            write_len(fd, stbuf, sizeof(struct stat));
        }

        free(stbuf);
        close(fd);
        return;
    }else if(type == MSG_GETOPEN){
        char path[1024] = { 0 };
        read_len(fd, path, req_header.msg_len);
        LOG("getopen: %s\n", path);
        char full_path[1024] = { 0 };
        strcpy(full_path, ".");
        strcat(full_path, path);
        int result = open(full_path, O_RDONLY);
        write_len(fd, &result, sizeof(int));
        close(fd);
        return;
    }else{
        LOG("%s\n","ERROR: Unknown request type\n");
    }
}

//ps aux | grep netfs 

int main(int argc, char *argv[]) {

    /* This starter code will initialize the server port and wait to receive a
     * message. */
    //chdir to the directory you're going to serve
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

    // fork for each connection
    while (true) {
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

        handle_request(client_fd);
        /* Now that the connection is established, we should probably read() from
        * the socket... */
    }
    return 0; 
}
