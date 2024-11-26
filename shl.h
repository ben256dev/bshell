#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

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

__attribute__((nonnull(1))) void shl_crypt_gen_salt(shl_salt salt)
{
   shl_hash salt_tmp;
   if (RAND_bytes(salt_tmp, SHL_SALT_SIZE/2) != 1)
      die("RAND_bytes: %s", ERR_get_error());

   for (size_t i = 0; i < SHL_SALT_SIZE; i++)
      sprintf(&salt[i * 2], "%02x", salt_tmp[i]);
}
__attribute__((nonnull(1, 2, 3))) void shl_crypt_gen_hash(const char* input, const shl_salt salt, shl_hash hash)
{
   shl_hash hash_tmp;
   argon2_error_codes hrv = argon2i_hash_raw(2, (1<<16), 1, input, strlen(input), salt, SHL_SALT_SIZE, hash_tmp, SHL_HASH_SIZE/2);
   if (hrv)
      die("argon2i_hash_raw() = %d", hrv);

   for (size_t i = 0; i < SHL_HASH_SIZE/2; i++)
      sprintf(&hash[i * 2], "%02x", hash_tmp[i]);
}

__attribute__((nonnull(1, 2, 3))) void shl_write_password(const char* file_path, shl_salt salt, shl_hash hash)
{
   FILE* fd = xfopen(file_path, "w");
   fprintf(fd, "%.*s\n", SHL_SALT_SIZE, salt);
   fprintf(fd, "%.*s\n", SHL_HASH_SIZE, hash);
   fclose(fd);
}
__attribute__((nonnull(1, 2))) void shl_authenticate(const char* file_path, const char* passphrase)
{
   shl_salt disk_salt;
   shl_hash disk_hash;

   FILE* fd = xfopen(file_path, "r");

   int rb;
   rb = fread(disk_salt, 1, SHL_SALT_SIZE, fd);
   if (rb != SHL_SALT_SIZE)
      die("fgets()");
   
   if (fgetc(fd) != '\n')
      die("fgetc()");

   rb = fread(disk_hash, 1, SHL_HASH_SIZE, fd);
   if (rb != SHL_SALT_SIZE)
      die("fgets()");

   fclose(fd);

   shl_hash user_hash;
   shl_crypt_gen_hash(passphrase, disk_salt, user_hash);

   if (strncmp(disk_hash, user_hash, SHL_HASH_SIZE) == 0)
      return;

   puts("\033[1;2;31minvalid\033[0m");
   exit(EXIT_SUCCESS);
}

__attribute__((nonnull(1, 2))) void shl_reinterpret_args(int* argc_ptr, char*** argv_ptr)
{
   int argc = *argc_ptr;
   char** argv = *argv_ptr;

   argc--;
   argv++;

   if (argc < 1 || strcmp("-c", argv[0]) != 0)
      die();

   argc--;
   argv++;

   if (argc != 1)
      die();

   static char* new_argv[SHL_MAX_ARGC] = {NULL};
   char* raw_argv = argv[0];
   char* token_ptr = new_argv[0];
   int   sanitize = 1;

   for (int i = 0; i < SHL_MAX_ARG_SIZE && argc <= SHL_MAX_ARGC; i++)
   {
      char c = raw_argv[i];
      if (sanitize)
      {
         if (c == '.' || c == '/')
            die("%d:'%c' is disallowed", i, c);
         if (i > 0 && token_ptr && token_ptr[0] == '-' && token_ptr[1] == 'p')
            sanitize = 0;
      }
      if (c == '\0')
      {
         if (token_ptr != NULL)
         {
            new_argv[argc - 1] = token_ptr;
            token_ptr = NULL;
         }
         else
            argc--;
         break;
      }
      else if (token_ptr != NULL && isspace(c))
      {
         sanitize = 1;
         c = '\0';
         new_argv[argc - 1] = token_ptr;
         argc++;
         token_ptr = NULL;
         if (argc > SHL_MAX_ARGC)
         {
            argc = SHL_MAX_ARGC;
            break;
         }
      }
      else if (token_ptr == NULL && !isspace(c))
      {
         token_ptr = raw_argv + i;
      }

      raw_argv[i] = c;
   }

   (*argc_ptr) = argc;
   (*argv_ptr) = new_argv;
}
