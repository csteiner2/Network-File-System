#ifndef _NET_H_
#define _NET_H_

#include <unistd.h>
#include <stdint.h>

enum msg_types {
	MSG_READDIR = 1,
};

struct __attribute__((__packed__)) netfs_msg_header {
    uint64_t msg_len;
    uint16_t msg_type;
};

int connect_to(char *hostname, int port);
ssize_t read_len(int fd, void *buf, size_t length);
ssize_t write_len(int fd, void *buf, size_t length);

#endif