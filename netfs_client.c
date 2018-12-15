/**
 * netfs_client.h
 *
 * Implementation of the netfs client file system. Based on the fuse 'hello'
 * example here: https://github.com/libfuse/libfuse/blob/master/example/hello.c
 */

#define FUSE_USE_VERSION 31

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
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
#include <sys/stat.h>

#include "common.h"
#include "logging.h"
#include "net.h"


#define TEST_DATA "hello world!\n"

/* Command line options */
static struct options {
    int show_help;
    int port;
} options;

#define OPTION(t, p) { t, offsetof(struct options, p), 1 }

/* Command line option specification. We can add more here. If we're interested
 * in a string, specify --opt=%s .*/
static const struct fuse_opt option_spec[] = {
    OPTION("-h", show_help),
    OPTION("--help", show_help),
    OPTION("--port=%d", port),
    FUSE_OPT_END
};

static int netfs_getattr(
        const char *path, struct stat *stbuf, struct fuse_file_info *fi) {

    LOG("getattr: %s\n", path);
    /* Clear the stat buffer */
    memset(stbuf, 0, sizeof(struct stat));

    /* By default, we will return 0 from this function (success) */
    int res = 0;
    int result;
    struct netfs_msg_header req_header = { 0 };
    req_header.msg_type = MSG_GETATTR;
    req_header.msg_len = strlen(path) + 1;
    int server_fd = connect_to("localhost", DEFAULT_PORT);
    write_len(server_fd, &req_header, sizeof(struct netfs_msg_header));
    write_len(server_fd, path, req_header.msg_len);
    read_len(server_fd, &result, sizeof(int));
    if(result<0){
        LOG("ERROR result less than 0: %d\n", result);
        close(server_fd);
        return -1;
    }

    read_len(server_fd, stbuf, sizeof(struct stat));

    if(stbuf->st_uid ==1) {
            stbuf->st_uid = getuid();
        }
    stbuf->st_mode = (mode_t) (~0222 & stbuf->st_mode);
    close(server_fd);
    return res;
}


static int netfs_readdir(
        const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
        struct fuse_file_info *fi, enum fuse_readdir_flags flags) {

    LOG("readdir: %s\n", path);
    //START WORKING HERE

    /* By default, we will return 0 from this function (success) */
    int res = 0;

    struct netfs_msg_header req_header = { 0 };
    req_header.msg_type = MSG_READDIR;
    req_header.msg_len = strlen(path) + 1;

    //HARD CODES FIX TO CHANGE BASED ON USER INPUT
    int server_fd = connect_to("localhost", DEFAULT_PORT);
    //we should check if server_fd is less than 0 here...
    write_len(server_fd, &req_header, sizeof(struct netfs_msg_header));
    write_len(server_fd, path, req_header.msg_len);

    uint16_t reply_len;
    char reply_path[1024] = { 0 };
    do {
        read_len(server_fd, &reply_len, sizeof(uint16_t));
        read_len(server_fd, reply_path, reply_len);
        filler(buf, reply_path, NULL, 0, 0);
    } while (reply_len > 0);
    close(server_fd);
    return res;
}

static int netfs_open(const char *path, struct fuse_file_info *fi) {

    LOG("open: %s\n", path);

    /* By default, we will return 0 from this function (success) */
    int res = 0;
    int result;
    struct netfs_msg_header req_header = { 0 };
    req_header.msg_type = MSG_GETOPEN;
    req_header.msg_len = strlen(path) + 1;
    int server_fd = connect_to("localhost", DEFAULT_PORT);
    // //we should check if server_fd is less than 0 here...
    write_len(server_fd, &req_header, sizeof(struct netfs_msg_header));
    write_len(server_fd, path, req_header.msg_len);  
    read_len(server_fd, &result, sizeof(int));
    if(result<0){
        LOG("ERROR result != 0: %d\n", result);
        close(server_fd);
        return -1;
    }
    close(server_fd);
    return res;
}

static int netfs_read(
        const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {

    LOG("read: %s\n", path);



    /* By default, we will return 0 from this function (success) */
    int res = 0;
    struct netfs_msg_header req_header = { 0 };
    req_header.msg_type = MSG_READ;
    req_header.msg_len = strlen(path) + 1;
    int server_fd = connect_to("localhost", DEFAULT_PORT);
    write_len(server_fd, &req_header, sizeof(struct netfs_msg_header));
    write_len(server_fd, path, req_header.msg_len);
    write_len(server_fd, &offset, sizeof(off_t));
    write_len(server_fd, &size, sizeof(size_t));
    read_len(server_fd, &size, sizeof(size_t));
    read_len(server_fd, buf, size);
    close(server_fd);
    return size;
}

/* This struct maps file system operations to our custom functions defined
 * above. */
static struct fuse_operations netfs_client_ops = {
    .getattr = netfs_getattr,
    .readdir = netfs_readdir,
    .open = netfs_open,
    .read = netfs_read,
};

static void show_help(char *argv[]) {
    printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
    printf("File-system specific options:\n"
            "    --port=<n>          Port number to connect to\n"
            "                        (default: %d)"
            "\n", DEFAULT_PORT);
}

int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    /* Set up default options: */
    options.port = DEFAULT_PORT;

    /* Parse options */
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1) {
        return 1;
    }

    if (options.show_help) {
        show_help(argv);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0] = (char*) "";
    }

    return fuse_main(args.argc, args.argv, &netfs_client_ops, NULL);
}
