#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <butil/butil.h>

int main(int argc, char* argv[])
{
   argc--;
   argv++;

   for (int i = 0; i < argc; i++)
   {
      char c;
      for (int j = 0; (c = argv[i][j]) != '\0'; j++)
      {
         if (!isalnum(c))
            die("'%c' is non-alphanumeric", c);
      }
   }

   if (argc == 2)
   {
      if (strcmp(argv[0], "break") == 0)
      {
         if (setenv("BPASSRAW", argv[1], 1))
            pdie("setenv(\"BPASSRAW\", argv[1], 1)");

         if (system("checkpass"))
            pdie("system(\"checkpass\")");

         return 0;
      }
   }

   printf("sup\n");

	return 0;
}
