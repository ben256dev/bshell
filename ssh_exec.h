#pragma once

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

void shl_client_enable_raw_mode();

void shl_client_disable_raw_mode();

int shl_client_exec(const char* hostname, const char* username, const char* publickey, const char* privatekey, const char* command);
