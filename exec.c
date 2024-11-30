#include <libssh2.h>
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libssh2.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <termios.h>

int main(int argc, char *argv[]) {
    const char *hostname = "ben256.com";
    const char *username = "datab";
    const char *publickey = "/home/ben256/.ssh/id_rsa.pub";
    const char *privatekey = "/home/ben256/.ssh/id_rsa";
    const char *command = "-c \"break -p\"";

     int rc;
    int sock;
    struct sockaddr_in sin;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel;

    // Initialize libssh2
    rc = libssh2_init(0);
    if (rc != 0) {
        fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
        return 1;
    }

    // Resolve hostname to IP address
    struct hostent *host;
    host = gethostbyname(hostname);
    if (!host) {
        fprintf(stderr, "Failed to resolve hostname\n");
        libssh2_exit();
        return 1;
    }

    // Create a socket and connect to the SSH server
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        libssh2_exit();
        return 1;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr = *((struct in_addr *)host->h_addr);

    if (connect(sock, (struct sockaddr *)(&sin), sizeof(struct sockaddr_in)) != 0) {
        perror("connect");
        close(sock);
        libssh2_exit();
        return 1;
    }

    // Initialize a session instance
    session = libssh2_session_init();
    if (!session) {
        fprintf(stderr, "Could not initialize SSH session\n");
        close(sock);
        libssh2_exit();
        return 1;
    }

    // Set blocking mode
    libssh2_session_set_blocking(session, 1);

    // Start the session
    rc = libssh2_session_handshake(session, sock);
    if (rc) {
        fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
        return 1;
    }

    // Authenticate using public key
    rc = libssh2_userauth_publickey_fromfile(session, username, publickey, privatekey, NULL);
    if (rc) {
        fprintf(stderr, "Authentication by public key failed\n");
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
        return 1;
    }

    // Open a session channel
    channel = libssh2_channel_open_session(session);
    if (!channel) {
        fprintf(stderr, "Unable to open a session channel\n");
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
        return 1;
    }

    // Request a PTY
    rc = libssh2_channel_request_pty(channel, "xterm"); // Use a terminal type supported by your server
    if (rc) {
        fprintf(stderr, "Failed to request PTY: %d\n", rc);
        libssh2_channel_free(channel);
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
        return 1;
    }

    // Start a shell session
    rc = libssh2_channel_shell(channel);
    if (rc) {
        fprintf(stderr, "Unable to start shell: %d\n", rc);
        libssh2_channel_free(channel);
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
        return 1;
    }

    // Write commands to the shell
    const char *commands = "ls\npwd\nexit\n";
    ssize_t n = libssh2_channel_write(channel, commands, strlen(commands));
    if (n < 0) {
        fprintf(stderr, "Error writing to channel: %zd\n", n);
    }

    // Read the output of the commands
    char buffer[1024];
    while (1) {
        n = libssh2_channel_read(channel, buffer, sizeof(buffer));
        if (n > 0) {
            fwrite(buffer, 1, n, stdout);
            fflush(stdout);
        } else if (n == LIBSSH2_ERROR_EAGAIN) {
            continue; // Wait for more data
        } else {
            break; // EOF or error
        }
    }

    // Close the channel
    libssh2_channel_close(channel);
    libssh2_channel_free(channel);
    channel = NULL;

    // Disconnect and free the session
    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);

    // Close the socket
    close(sock);

    // Cleanup libssh2
    libssh2_exit();

    return 0;
}
