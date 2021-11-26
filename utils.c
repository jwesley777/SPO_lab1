void parse_args(const char *symbol, char *args[], int args_number)
{
    int arg_number = 0;
    int args_parsed = 0;

    while (!args_parsed)
    {
        if (arg_number >= args_number || *symbol == '\0') {
            args_parsed = 1;
        }
        else if (isspace(*symbol))
        {
            symbol++;
        }
        else if (*symbol == '"' || *symbol == '\'')
        {
            int quote_char = *symbol;
            const char* first_char = ++symbol;

            while (*symbol != quote_char && *symbol != '\0')
            {
                symbol++;
            }

            strncpy(args[arg_number++], first_char, symbol++ - first_char);

            if (*symbol == '\0')
            {
                args_parsed = 1;
            }
            else
            {
                symbol++;
            }
        }
        else if (*symbol > 32)
        {
            const char* first_char = symbol;

            while (*symbol > 32)
            {
                symbol++;
            }

            strncpy(args[arg_number++], first_char, symbol - first_char);

            if (*symbol == '\0')
            {
                args_parsed = 1;
            }
            else
            {
                symbol++;
            }
        }
        else
        {
            args_parsed = 1;
        }
    }
}


void copy_file(struct fs_structures *structures, char *target, struct file *file)
{
    int directory_specified = target[strlen(target) - 1] == '/' ? 1 : 0;

    char path[MAX_PATH_LENGTH] = "";
    strcat(path, target);
    if (directory_specified)
    {
        strcat(path, file->file_name);
    }

    int target_file_descriptor = open(path, O_RDWR | O_CREAT, 0777);

    unsigned int current_cluster = file->first_cluster;
    unsigned int cluster_size = structures->boot_sector_structure->bytes_per_sector * structures->boot_sector_structure->sectors_per_cluster;
    char *cluster_buffer = malloc(cluster_size);

    unsigned long bytes_left = file->size;

    while (bytes_left)
    {
        unsigned long int bytes_to_read = bytes_left < cluster_size ? bytes_left : cluster_size;
        bytes_left -= bytes_to_read;
        read_cluster(structures, current_cluster, cluster_buffer);
        current_cluster = read_table_address(structures, current_cluster);
        write(target_file_descriptor, cluster_buffer, bytes_to_read);
    }

    free(cluster_buffer);
    close(target_file_descriptor);
}


void copy_dir(struct fs_structures *fs_structures, char *destination, struct file *directory)
{
    mkdir(destination, 0777);
    struct file *file = read_directory(fs_structures, directory->first_cluster);
    struct file *current_file = file;
    while (current_file != NULL)
    {
        char subdirectory_path[MAX_PATH_LENGTH] = "";
        strcat(subdirectory_path, destination);
        strcat(subdirectory_path, "/");
        strcat(subdirectory_path, current_file->file_name);

        if (current_file->is_directory)
        {
            if (!strcmp(current_file->file_name, ".") || !strcmp(current_file->file_name, ".."))
            {
                current_file = current_file->next_file;
                continue;
            }
            copy_dir(fs_structures, subdirectory_path, current_file);
        }
        else
        {
            copy_file(fs_structures, subdirectory_path, current_file);
        }
        current_file = current_file->next_file;
    }
    free_file(file);
}