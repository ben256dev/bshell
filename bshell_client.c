#include <string.h>
#include <butil/butil.h>

#include "ssh_exec.h"

int main(int argc, char *argv[])
{
    argc--;
    argv++;

    if (strcmp("break", argv[0]) == 0)
    {
        if (argv[1][0] == '-' && (argv[1][1] == 'p' || argv[1][1] == 'd'))
        {
            size_t arglen = strlen(argv[1]) + 7;
            char* combined_arg = xmalloc(arglen);
            snprintf(combined_arg, arglen, "break %s", argv[1]);

            shl_client_exec("ben256.com", "datab", "/home/ben256/.ssh/id_rsa.pub", "/home/ben256/.ssh/id_rsa", combined_arg);
        }
        else
        {
            printf("unknown arguments\n");
        }
    }

    return 0;
}
