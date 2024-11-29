#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>

#include <butil/butil.h>
#include <argon2.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#define SHL_MAX_ARGC 16
#define SHL_MAX_ARG_SIZE 128
#define SHL_SALT_SIZE 64
#define SHL_HASH_SIZE 64
typedef uint8_t shl_salt [SHL_SALT_SIZE];
typedef uint8_t shl_hash [SHL_HASH_SIZE];

#define SHL_FLAG_HIDE_PASSWORD_OFF 01
void shl_set_flag(uint8_t flag);
int shl_get_flag(uint8_t flag);

void shl_crypt_gen_salt(shl_salt salt) __attribute__((nonnull(1)));

void shl_crypt_gen_hash(const char* input, const shl_salt salt, shl_hash hash) __attribute__((nonnull(1, 2, 3)));

void shl_write_password(const char* file_path, shl_salt salt, shl_hash hash) __attribute__((nonnull(1, 2, 3)));

void shl_authenticate(const char* file_path, const char* passphrase) __attribute__((nonnull(1, 2)));

void shl_reinterpret_args(int* argc_ptr, char*** argv_ptr) __attribute__((nonnull(1, 2)));

void shl_terminal_disable_raw(void);

// return 1 if not a terminal 0 otherwise
int shl_terminal_enable_raw(void);

// return NULL if not a terminal
char* shl_get_password_raw(void);

void shl_create_password(const char* password_path, const char* password_raw) __attribute__((nonnull(1, 2)));

void shl_delete_password(const char* password_path) __attribute__((nonnull(1)));

void shl_break_shell(void);
