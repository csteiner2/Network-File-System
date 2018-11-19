
CFLAGS += -Wall -g
LDFLAGS +=
client_flags += -I/usr/include/fuse3 -lpthread -lfuse3 -D_FILE_OFFSET_BITS=64

all: netfs_client netfs_server

netfs_client: netfs_client.c common.h logging.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(client_flags) $^ -o $@

clean:
	rm -f netfs_client netfs_server

