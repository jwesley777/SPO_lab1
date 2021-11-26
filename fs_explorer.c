#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "fat32.h"
#include "utils.c"


void execute_ls(struct fs_structures *fs_structures, char *path)
{
    struct file *file;
    if (!strcmp("", path))
    {
        file = read_directory(fs_structures, fs_structures->current_cluster);
    }
    else if (path[0] == '/')
    {
        struct file *first_directory = read_directory(fs_structures, fs_structures->boot_sector_structure->root_cluster_number);  
        file = find_directory(fs_structures, path + 1, first_directory);
        if (file != first_directory) {
            free(first_directory);
        }
    }
    else
    {
        struct file *first_directory = read_directory(fs_structures, fs_structures->current_cluster);
        file = find_directory(fs_structures, path, first_directory);
        if (file != first_directory) {
            free(first_directory);
        }
    }

    if (file != NULL)
    {
        struct file *current_file = file;
        while (current_file != NULL) {
            if (current_file->is_directory) {
                printf("%s/\n", current_file->file_name);
            } else {
                printf("%s\n", current_file->file_name);
            }
            current_file = current_file->next_file;
        }
        free_file(file);
    }
    else
    {
        printf("Could not find specified directory\n");
    }
}


void execute_cp(struct fs_structures *fs_structures, char *source, char *destination)
{
    if (!strcmp("", source) || !strcmp("", destination))
    {
        printf("You must specify source directory and destination directory as parameters of 'cp' command\n");
        return;
    }

    if (destination[0] != '/')
    {
        printf("Specify destination path as an absolute path\n");
        return;
    }

    int target_found = 0;

    unsigned int last_subpath_slash = strlen(source);
    while (source[last_subpath_slash] != '/' && last_subpath_slash != 0)
    {
        last_subpath_slash--;
    }

    char path[MAX_PATH_LENGTH] = {0};
    char file_name[MAX_PATH_LENGTH] = {0};

    strncpy(path, source, last_subpath_slash);
    if (last_subpath_slash == 0 && source[0] != '/')
    {
        strncpy(file_name, source + last_subpath_slash, strlen(source) - last_subpath_slash);
    }
    else
    {
        strncpy(file_name, source + last_subpath_slash + 1, strlen(source) - last_subpath_slash);
    }

    struct file *current_file;
    if (path[0] == '/')
    {
        struct file *first_directory = read_directory(fs_structures, fs_structures->boot_sector_structure->root_cluster_number);  
        current_file = find_directory(fs_structures, path + 1, first_directory);
        if (current_file != first_directory)
        {
            free(first_directory);
        }
    }
    else
    {
        struct file *first_directory = read_directory(fs_structures, fs_structures->current_cluster);
        current_file = find_directory(fs_structures, path, first_directory);
        if (current_file != first_directory)
        {
            free(first_directory);
        }
    }

    while (current_file != NULL)
    {
        if (!strcmp(current_file->file_name, file_name))
        {
            if (current_file->is_directory)
            {
                copy_dir(fs_structures, destination, current_file);
            }
            else
            {
                copy_file(fs_structures, destination, current_file);
            }

            target_found = 1;

            break;
        }

        current_file = current_file->next_file;
    }

    if (!target_found)
    {
        printf("Could not find file specified by source path\n");
    }

    free_file(current_file);
}


void execute_cd(struct fs_structures *fs_structures, char *path)
{
    if (!strcmp("", path))
    {
        printf("You must specify target directory as parameters of 'cd' command\n");
        return;
    }

    unsigned int result = move_to_directory(fs_structures, path);
    if (result == 1) {
        printf("Could not find directory with specified name\n");
    }
}


void execute_help()
{
    printf("Existing commands:\n");
    printf(" ls - print list of files and directories in current directory\n");
    printf(" cp - copy files from target file system\n");
    printf(" cd - change current directory\n");
    printf(" pwd - show path to current directory\n");
    printf(" help - print this information\n");
    printf(" exit - close this program\n");
}


int explore_fs(char *path)
{
    struct fs_structures *fs_structures = open_fs(path);

    if (fs_structures == NULL)
    {
        printf("Could not open specified disk/partition/file as FAT32 file system\n");
        return -1;
    }

    int exit = 0;

    while (!exit)
    {
        printf("# ");
        char command[MAX_COMMAND_LENGTH];
        fgets(command, sizeof(command), stdin);

        for (int i = 0; i < sizeof(command); i++)
        {
            if (command[i] == '\n')
            {
                command[i] = 0;
                break;
            }
            else if (command[i] == EOF)
            {
                return 0;
            }
        }

        char* args[MAX_ARGS];
        for (int i = 0; i < MAX_ARGS; i++)
        {
            args[i] = calloc(1, MAX_ARG_LENGTH);
        }

        parse_args(command, args, MAX_ARGS);

        if (!strcmp("ls", args[0]))
        {
            execute_ls(fs_structures, args[1]);
        }
        else if (!strcmp("cp", args[0]))
        {
            execute_cp(fs_structures, args[1], args[2]);
        }
        else if (!strcmp("cd", args[0]))
        {
            execute_cd(fs_structures, args[1]);
        }
        else if (!strcmp("pwd", args[0]))
        {
            printf("%s\n", fs_structures->current_path);
        }
        else if (!strcmp("help", args[0]))
        {
            execute_help();
        }
        else if (!strcmp("exit", args[0]))
        {
            printf("Exiting...\n");
            exit = 1;
        }
        else if (strlen(args[0]) == 0) {
            // Empty line. Do nothing
        }
        else {
            printf("You put unknown command. Type \"help\" for list all existing commands\n");
        }

        for (int i = 0; i < MAX_ARGS; i++) {
            free(args[i]);
        }
    }

    close_fs(fs_structures);

    return 0;
}
