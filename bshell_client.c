#include "ssh_exec.h"

int main(int argc, char *argv[])
{
    shl_client_exec("ben256.com", "datab", "/home/ben256/.ssh/id_rsa.pub", "/home/ben256/.ssh/id_rsa", "break -p");

    return 0;
}
