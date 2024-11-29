#include "shl.h"

int main(int argc, char* argv[])
{
   shl_reinterpret_args(&argc, &argv);

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

            if (argv[1][0] == '-' && argv[1][1] == 'p')
            {
               char* password_raw;
               if (argv[1][2] == '\0')
               {
                  size_t password_len = 0;
                  size_t password_size = 16;
                  password_raw = xmalloc(password_size);

                  shl_terminal_enable_raw();
                  for (; ;)
                  {
                     int c;
                     c = getchar();
                     if (c == '\n' || c == '\r' || c == EOF)
                        break;

                     if (password_len >= password_size)
                     {
                        password_size *= 2;
                        password_raw = xrealloc(password_raw, password_size);
                     }
                     password_raw[password_len] = c;
                     putchar('*');

                     password_len++;
                  }
                  password_raw[password_len] = '\0';
                  printf("\r\n");
                  shl_terminal_disable_raw();

               }
               else
                  password_raw = argv[1] + 2;

               if (dne)
               {
                  shl_salt salt;
                  shl_crypt_gen_salt(salt);

                  shl_hash hash;
                  shl_crypt_gen_hash(password_raw, salt, hash);

                  shl_write_password(password_path, salt, hash);
                  puts("\033[1;36mcreated\033[0m");
                  return 0;
               }
               shl_authenticate(password_path, password_raw);
               puts("\033[1;32msuccess\033[0m");

               const char *bash_path = "/bin/bash";
               char* bash_argv[2] = { "-bash", NULL };
               if (execvp(bash_path, bash_argv) == -1)
                  pdie("execvp()");

               free(password_raw);
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
               shl_authenticate(password_path, password_raw);
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
