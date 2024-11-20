#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <butil/butil.h>
#include <argon2.h>

int main(int argc, char* argv[])
{
   argc--;
   argv++;

   for (int i = 0; i < argc; i++)
   {
      char c;
      for (int j = 0; (c = argv[i][j]) != '\0'; j++)
      {
         if (c == '.' || c == '/' || c == '\\')
            die("'%c' is disallowed", c);
      }
   }

   if (argc > 0)
   {
      if (strcmp(argv[0], "break") == 0)
      {
         if (argc == 2)
         {
            uint8_t key_hash[32];
            uint8_t salt[16];
            memset( salt, 0x00, 16);

            argon2_error_codes hrv = argon2i_hash_raw(2, (1<<16), 1, "root", 4, salt, 16, key_hash, 32);
            if (hrv)
               die("argon2i_hash_raw() = %d", hrv);

            uint8_t key_str[65];
            for (size_t i = 0; i < 32; i++)
            {
               sprintf(&key_str[i * 2], "%02x", key_hash[i]);
            }
            key_hash[64] = '\0';

            if (argv[1][0] == '-' && argv[1][1] == 'p')
            {
               if (setenv("BPASSKEY", key_str, 1))
                  pdie("setenv($BPASSKEY)");
               if (setenv("BPASSVAL", argv[1] + 2, 1))
                  pdie("setenv($BPASSKEY)");
               if (setenv("BPASSDEL", "0", 1))
                  pdie("setenv($BPASSDEL)");

               if (system("checkpass"))
                  pdie("checkpass");

               return 0;
            }
            else if (argv[1][0] == '-' && argv[1][1] == 'd')
            {
               if (setenv("BPASSKEY", key_str, 1))
                  pdie("setenv($BPASSKEY)");
               if (setenv("BPASSVAL", argv[1] + 2, 1))
                  pdie("setenv($BPASSKEY)");
               if (setenv("BPASSDEL", "1", 1))
                  pdie("setenv($BPASSDEL)");

               if (system("checkpass"))
                  pdie("system()");
               return 0;
            } 
         }
      }
   }

   printf("hi\n");

	return 0;
}
