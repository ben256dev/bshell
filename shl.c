#include "shl.h"

uint8_t shl_flags = 0;

void shl_set_flag(uint8_t flag)
{
   shl_flags |= flag;
}

int shl_get_flag(uint8_t flag)
{
   return (int)(shl_flags & flag);
}

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
      die("%d:%s", argc, argv[0]);

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

struct termios original_termios;

void shl_terminal_disable_raw(void)
{
   if (tcsetattr(STDIN_FILENO, TCSANOW, &original_termios) == -1)
      pdie("tcsetattr()");
}

int shl_terminal_enable_raw(void)
{
   if (!isatty(STDIN_FILENO))
      return 1;

   if (tcgetattr(STDIN_FILENO, &original_termios) == -1)
      pdie("tcgetattr()");

   atexit(shl_terminal_disable_raw);

   struct termios raw_termios = original_termios;

   raw_termios.c_lflag &= ~(ECHO | ICANON);
   raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
   raw_termios.c_oflag &= ~(OPOST);
   raw_termios.c_cflag &= ~(CS8);
   raw_termios.c_cc[VMIN] = 1;
   raw_termios.c_cc[VTIME] = 0;

   if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios) == -1)
      pdie("tcsetattr()");

   return 0;
}

char* shl_get_password_raw(void)
{
   size_t password_len = 0;
   size_t password_size = 16;
   char* password_raw = xmalloc(password_size);

   if (shl_terminal_enable_raw())
      return NULL;

   for (;;)
   {
      int c;
      c = getchar();
      if (c == '\n' || c == '\r' || c == EOF)
         break;
      else if (c == 127)
      {
         if (password_len > 0)
         {
            password_len--;
            putchar('\b');
            putchar(' ');
            putchar('\b');
         }
         continue;
      }

      if (password_len >= password_size)
      {
         password_size *= 2;
         password_raw = xrealloc(password_raw, password_size);
      }
      password_raw[password_len] = c;

      (shl_get_flag(SHL_FLAG_HIDE_PASSWORD_OFF)) ?
         putchar(c) : putchar('*');

      password_len++;
   }
   password_raw[password_len] = '\0';
   printf("\r\n");
   shl_terminal_disable_raw();

   return password_raw;
}

__attribute__((nonnull(1, 2))) void shl_create_password(const char* password_path, const char* password_raw)
{
   shl_salt salt;
   shl_crypt_gen_salt(salt);
   shl_hash hash;
   shl_crypt_gen_hash(password_raw, salt, hash);
   shl_write_password(password_path, salt, hash);

   puts("\033[1;36mcreated\033[0m");
}

__attribute__((nonnull(1))) void shl_delete_password(const char* password_path)
{
   if (remove(password_path))
      pdie("remove()");
   puts("\033[1;35mdeleted\033[0m");
}

void shl_break_shell(void)
{
   const char *bash_path = "/bin/bash";
   char* bash_argv[2] = { "-bash", NULL };
   if (execvp(bash_path, bash_argv) == -1)
      pdie("execvp()");
}
