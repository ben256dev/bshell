#include <stdio.h>

#include <butil/butil.h>

int main(int argc, char* argv[])
{
   const char* passraw = getenv("BPASSRAW");
   if (passraw == NULL)
      die("passraw == NULL");

   puts(passraw);

	return 0;
}
