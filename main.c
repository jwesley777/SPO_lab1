#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "consts.c"
#include "partitions.c"
#include "fs_explorer.c"

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        return show_partitions();
    }
    else if (argc == 2)
    {
        return explore_fs(argv[1]);
    }
    else
    {
        printf("To many arguments\n");
    }

    return 0;
}
