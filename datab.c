#include <libssh2.h>

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *hostname = "144.202.88.33";
static const char *commandline = "uptime";
static const char *pubkey = "/home/ben256/.ssh/id_rsa.pub";
static const char *privkey = "/home/ben256/.ssh/id_rsa";

static int waitsocket(libssh2_socket_t socket_fd, LIBSSH2_SESSION *session)
{
    struct timeval timeout;
    int rc;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
    int dir;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO(&fd);

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
    FD_SET(socket_fd, &fd);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    /* now make sure we wait in the correct direction */
    dir = libssh2_session_block_directions(session);

    if(dir & LIBSSH2_SESSION_BLOCK_INBOUND)
        readfd = &fd;

    if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
        writefd = &fd;

    rc = select((int)(socket_fd + 1), readfd, writefd, NULL, &timeout);

    return rc;
}

int main(int argc, char *argv[])
{
   uint32_t hostaddr;
   libssh2_socket_t sock;
   struct sockaddr_in sin;
   const char *fingerprint;
   int rc;
   LIBSSH2_SESSION *session = NULL;
   LIBSSH2_CHANNEL *channel;
   int exitcode;
   char *exitsignal = (char *)"none";
   ssize_t bytecount = 0;
   size_t len;
   LIBSSH2_KNOWNHOSTS *nh;
   int type;

   rc = libssh2_init(0);
   if(rc) {
      fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
      return 1;
   }

   hostaddr = inet_addr(hostname);

   /* Ultra basic "connect to port 22 on localhost".  Your code is
   * responsible for creating the socket establishing the connection
   */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock == LIBSSH2_INVALID_SOCKET) {
      fprintf(stderr, "failed to create socket.\n");
      return -1;
   }

   sin.sin_family = AF_INET;
   sin.sin_port = htons(22);
   sin.sin_addr.s_addr = hostaddr;
   if(connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in))) {
      fprintf(stderr, "failed to connect.\n");
      return -1;
   }

   printf("connected successfully\n");

	return 0;
}
