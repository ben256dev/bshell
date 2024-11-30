// chat gpt wrote all this ssh garbage :)

#include "ssh_exec.h"

int rc;
int sock;
struct sockaddr_in sockin;
LIBSSH2_SESSION *session = NULL;
LIBSSH2_CHANNEL *channel = NULL;
struct hostent *host;
fd_set fds;
struct timeval timeout;
ssize_t nread;
char buffer[1024];
int exitcode;

struct termios original_termios;

void shl_client_enable_raw_mode()
{
    struct termios raw;

    tcgetattr(STDIN_FILENO, &original_termios);
    raw = original_termios;

    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void shl_client_disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

int shl_client_exec(const char* hostname, const char* username, const char* publickey, const char* privatekey, const char* command)
{
   /* Initialize libssh2 */
   rc = libssh2_init(0);
   if (rc != 0) {
     fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
     return 1;
   }

   /* Resolve the host name */
   host = gethostbyname(hostname);
   if (!host) {
     fprintf(stderr, "Could not resolve hostname %s\n", hostname);
     libssh2_exit();
     return 1;
   }

   /* Create a socket and connect */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   sockin.sin_family = AF_INET;
   sockin.sin_port = htons(22);
   sockin.sin_addr.s_addr = *((unsigned long *)host->h_addr);

   if (connect(sock, (struct sockaddr*)(&sockin), sizeof(struct sockaddr_in)) != 0) {
     perror("Failed to connect");
     close(sock);
     libssh2_exit();
     return 1;
   }

   /* Create a session instance */
   session = libssh2_session_init();
   if (!session) {
     fprintf(stderr, "Could not initialize SSH session\n");
     close(sock);
     libssh2_exit();
     return 1;
   }

   /* Start the session */
   rc = libssh2_session_handshake(session, sock);
   if (rc) {
     fprintf(stderr, "SSH session handshake failed: %d\n", rc);
     goto cleanup;
   }

   /* Authenticate using public key */
   rc = libssh2_userauth_publickey_fromfile(session, username, publickey, privatekey, NULL);
   if (rc) {
     fprintf(stderr, "Authentication by public key failed\n");
     goto cleanup;
   }

   /* Open a channel */
   channel = libssh2_channel_open_session(session);
   if (!channel) {
     fprintf(stderr, "Unable to open channel\n");
     goto cleanup;
   }

   /* Request a PTY */
   rc = libssh2_channel_request_pty(channel, "xterm-256color");
   if (rc) {
     fprintf(stderr, "Failed requesting PTY: %d\n", rc);
     goto cleanup;
   }

   /* Execute the command */
   rc = libssh2_channel_exec(channel, command);
   if (rc) {
     fprintf(stderr, "Unable to execute command: %d\n", rc);
     goto cleanup;
   }

   /* Set up non-blocking I/O */
   libssh2_channel_set_blocking(channel, 0);
   int fd = sock;
   int maxfd = fd;

   signal(SIGINT, shl_client_disable_raw_mode);
   signal(SIGTERM, shl_client_disable_raw_mode);
   atexit(shl_client_disable_raw_mode);

   shl_client_enable_raw_mode();

   /* Prepare for select */
   while (1) {
     FD_ZERO(&fds);
     FD_SET(fd, &fds);
     FD_SET(STDIN_FILENO, &fds);

     timeout.tv_sec = 0;
     timeout.tv_usec = 100000;

     rc = select(maxfd + 1, &fds, NULL, NULL, &timeout);
     if (rc < 0) {
         perror("select");
         break;
     }

     /* Read from the SSH channel */
     if (FD_ISSET(fd, &fds)) {
         nread = libssh2_channel_read(channel, buffer, sizeof(buffer));
         if (nread == LIBSSH2_ERROR_EAGAIN) {
             continue;
         } else if (nread < 0) {
             fprintf(stderr, "Error reading from channel: %zd\n", nread);
             break;
         } else if (nread == 0) {
             /* EOF */
             break;
         } else {
             fwrite(buffer, 1, nread, stdout);
             fflush(stdout);
         }
     }

     /* Write user input to the SSH channel */
     if (FD_ISSET(STDIN_FILENO, &fds)) {
         nread = read(STDIN_FILENO, buffer, sizeof(buffer));
         if (nread > 0) {
             char *ptr = buffer;
             ssize_t nwritten;
             while (nread > 0) {
                 nwritten = libssh2_channel_write(channel, ptr, nread);
                 if (nwritten < 0) {
                     fprintf(stderr, "Error writing to channel: %zd\n", nwritten);
                     break;
                 }
                 ptr += nwritten;
                 nread -= nwritten;
             }
         } else if (nread == 0) {
             /* EOF from stdin */
             break;
         } else {
             perror("read");
             break;
         }
     }

     /* Check if the channel is closed */
     if (libssh2_channel_eof(channel)) {
         break;
     }
   }

   /* Wait for the channel to close */
   libssh2_channel_wait_closed(channel);

   /* Get the exit code */
   exitcode = libssh2_channel_get_exit_status(channel);

   cleanup:
   if (channel) {
     libssh2_channel_free(channel);
     channel = NULL;
   }

   if (session) {
     libssh2_session_disconnect(session, "Normal Shutdown");
     libssh2_session_free(session);
   }

   close(sock);
   libssh2_exit();

   return 0;
}
