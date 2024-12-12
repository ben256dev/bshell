#include <string.h>
#include <dirent.h>

#include <butil/butil.h>

#include "ssh_exec.h"

int main(int argc, char *argv[])
{
    argc--;
    argv++;
    
    const char* break_comm = "break";
    size_t break_comm_len = sizeof(break_comm);

    if (argc > 0 && strcmp("break", argv[0]) == 0)
    {
        if (argc == 2 && argv[1][0] == '-' && (argv[1][1] == 'p' || argv[1][1] == 'd'))
        {
            size_t arglen = strlen(argv[1]) + 1 + break_comm_len;
            char* combined_arg = xmalloc(arglen);
            snprintf(combined_arg, arglen, "%s %s", break_comm, argv[1]);

            shl_client_exec("ben256.com", "datab", "/home/ben256/.ssh/id_rsa.pub", "/home/ben256/.ssh/id_rsa", combined_arg);
        }
        else
            printf("\033[1;3mbreak:\033[0m break <-p | -d>\n");

        return 0;
    }
    else
    {
        struct stat dir_stat;
        if (stat("exec", &dir_stat) == -1)
        {
            puts("missing \033[1;33m\"exec\"\033[0m directory");
            return 0;
        }

        const char* comm_dir = "exec";
        size_t comm_dir_len = sizeof(comm_dir);

        DIR* dir = opendir(comm_dir);
        if (dir == NULL)
            pdie("opendir()");

        for (struct dirent* ent; (ent = readdir(dir)) != NULL; )
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            size_t name_len = comm_dir_len + 1 + strlen(ent->d_name);
            char* combined_name = xmalloc(name_len);
            snprintf(combined_name, name_len, "%s/%s", comm_dir, ent->d_name);

            if (execvp(combined_name, &argv[0]) == -1)
                pdie("execvp()");
        }

        closedir(dir);
        printf("unknown command: \"%s\"\n", argv[0]);
        return -1;
    }
}
