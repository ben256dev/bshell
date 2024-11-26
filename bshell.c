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

__attribute__((nonnull(1, 2, 3))) void write_password(const char* file_path, shl_salt salt, shl_hash hash)
{
   FILE* fd = xfopen(file_path, "w");
   fprintf(fd, "%.*s\n", SHL_SALT_SIZE, salt);
   fprintf(fd, "%.*s\n", SHL_HASH_SIZE, hash);
   fclose(fd);
}
__attribute__((nonnull(1, 2))) void authenticate(const char* file_path, const char* passphrase)
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

int main(int argc, char* argv[])
{
   argc--;
   argv++;

   if (argc < 1 || strcmp("-c", argv[0]) != 0)
      die();

   argc--;
   argv++;

   if (argc != 1)
      die();

   char* raw_argv = argv[0];
   char* new_argv[SHL_MAX_ARGC] = {NULL};
   char* token_ptr = new_argv[0];

   for (int i = 0; i < SHL_MAX_ARG_SIZE && argc <= SHL_MAX_ARGC; i++)
   {
      char c = raw_argv[i];
      if (c == '.' || c == '/' || c == '\\')
         die("'%c' is disallowed", c);
      else if (c == '\0')
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
      else if (token_ptr == NULL && !isspace(c))
      {
         token_ptr = raw_argv + i;
      }
      else if (token_ptr != NULL)
      {
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

      raw_argv[i] = c;
   }

   argv = new_argv;

   for (int i = 0; i < argc; i++)
   {
      puts(argv[i]);
   }

   if (argc > 0)
   {
      if (strcmp(argv[0], "break") == 0)
      {
         if (argc == 2)
         {
            const char* home_dir = getenv("HOME");
            if (home_dir == NULL)
               die("HOME == NULL");

            size_t path_len = snprintf(NULL, 0, "%s/.bshell/root", home_dir) + 1;
            char* password_path = xmalloc(path_len);
            snprintf(password_path, path_len, "%s/.bshell/root", home_dir);

            struct stat statbuff;
            int dne = stat(password_path, &statbuff);

            if (dne == 0 && !S_ISREG(statbuff.st_mode))
               die("\033[1;5;31mirregular:%o\033[0m", statbuff.st_mode);

            if (argv[1][0] == '-' && argv[1][1] == 'p' && argv[1][2] != '\0')
            {
               const char* password_raw = argv[1] + 2;
               if (dne)
               {
                  shl_salt salt;
                  shl_crypt_gen_salt(salt);

                  shl_hash hash;
                  shl_crypt_gen_hash(password_raw, salt, hash);

                  write_password(password_path, salt, hash);
                  puts("\033[1;36mcreated\033[0m");
                  return 0;
               }
               authenticate(password_path, password_raw);
               puts("\033[1;32msuccess\033[0m");

               const char *bash_path = "/bin/bash";
               char* bash_argv[2] = { "-bash", NULL };
               if (execvp(bash_path, bash_argv) == -1)
                  pdie("execvp()");
               return 0;
            }
            else if (argv[1][0] == '-' && argv[1][1] == 'd')
            {
               const char* password_raw = argv[1] + 2;
               if (dne)
               {
                  puts("\033[1;2;31munknown\033[0m");
                  return 0;
               }
               authenticate(password_path, password_raw);
               if (remove(password_path))
                  pdie("remove()");
               puts("\033[1;35mdeleted\033[0m");
               return 0;
            } 
         }
      }
   }

	return 0;
}
