#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <butil/butil.h>


int main(int argc, char* argv[])
{
   const char* passkey = getenv("BPASSKEY");
   if (passkey == NULL)
      die("BPASSKEY == NULL");

   const char* passsal = getenv("BPASSSAL");
   if (passsal == NULL)
      die("BPASSSAL == NULL");

   const char* passdel = getenv("BPASSDEL");
   if (passdel == NULL)
      die("BPASSDEL == NULL");

   const char* passval = getenv("BPASSVAL");
   if (passval == NULL)
      die("BPASSVAL == NULL");
   size_t passlen = strlen(passval);

   const char* home_dir = getenv("HOME");
   if (home_dir == NULL)
      die("HOME == NULL");

   size_t path_len = snprintf(NULL, 0, "%s/.bshell/%s", home_dir, passkey) + 1;
   char* password_path = xmalloc(path_len);
   snprintf(password_path, path_len, "%s/.bshell/%s", home_dir, passkey);

   if (passdel[0] != '0' && passdel[0] != '1')
      die();

   struct stat statbuff;
   int rv = stat(password_path, &statbuff);

   if (rv == 0 && !S_ISREG(statbuff.st_mode))
      die("\033[1;5;31mirregular:%o\033[0m", statbuff.st_mode);

   if (rv)
   {
      if (passdel[0] == '1')
      {
         puts("\033[1;2;31munknown\033[0m");
         return 0;
      }
      write_password(password_path, passval, passlen, passsal);
      puts("\033[1;36mcreated\033[0m");
      return 0;
   }
   else
   {
      authenticate(password_path, passval, passlen);
      if (passdel[0] == '1')
      {
         if (remove(password_path))
            pdie("remove()");
         puts("\033[1;35mdeleted\033[0m");
      }
      else
      {
         puts("\033[1;32msuccess\033[0m");

         const char *bash_path = "/bin/bash";
         char* bash_argv[2] = { "-bash", NULL };
         if (execvp(bash_path, bash_argv) == -1)
            pdie("execvp()");
      }
      return 0;
   }

   puts("hi");

	return 0;
}
