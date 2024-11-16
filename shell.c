#include <stdio.h>
#include <string.h>
#include <ctype.h>

void die()
{
   exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
   argc--;
   argv++;

   for (int i = 0; i < argc; i++)
   {
      char c;
      for (int j = 0; (c = argv[j]) != '\0'; j++)
      {
         if (!isalnum(c))
            die();
      }
   }

   if (argc == 2)
   {
      if (strcmp(argv[0], "unlock-shell") == 0)
      {
         setenv("BPASSPHRASERAW", argv[1], 1);
         system("unlock-shell-check-passphrase");
         perror("execvp failed");
      }
   }
   else
      printf("sup\n");

	return 0;
}
