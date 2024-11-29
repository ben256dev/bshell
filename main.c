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

            if (argv[1][1] == 'P' || argv[1][1] == 'D')
            {
               argv[1][1] = tolower(argv[1][1]);
               shl_set_flag(SHL_FLAG_HIDE_PASSWORD_OFF);
            }

            char* password_raw;
            if (argv[1][0] == '-' && (argv[1][1] == 'p' || argv[1][1] == 'd'))
            {
               password_raw = (argv[1][2] == '\0') ?
                  shl_get_password_raw() : argv[1] + 2;

               if (dne)
               {
                  (argv[1][1] == 'p') ?
                     shl_create_password(password_path, password_raw) : puts("\033[1;2;31munknown\033[0m");
                  return 0;
               }

               shl_authenticate(password_path, password_raw);

               (argv[1][1] == 'p') ?
                  shl_break_shell() : shl_delete_password(password_path);

               free(password_raw);
               return 0;
            }
         }
      }
   }

	return 0;
}
