char *const SYS_BLOCK_PATH = "/sys/block/";


int show_partitions()
{
    DIR *sys_block_dir = opendir(SYS_BLOCK_PATH);
    struct dirent *disk;

    if (sys_block_dir)
    {
        while ((disk = readdir(sys_block_dir)) != NULL)
        {
            if (strcmp(disk->d_name, ".") && strcmp(disk->d_name, ".."))
            {
                printf("/dev/%s\n", disk->d_name);

                char disk_path[MAX_PATH_LENGTH] = {0};
                strcat(strcat(disk_path, SYS_BLOCK_PATH), disk->d_name);
                DIR *sys_block_subdir = opendir(disk_path);
                struct dirent *partition;

                if (sys_block_subdir)
                {
                    while ((partition = readdir(sys_block_subdir)) != NULL)
                    {
                        if (!memcmp(partition->d_name, disk->d_name, strlen(disk->d_name)))
                        {
                            printf("- /dev/%s\n", partition->d_name);
                        }
                    }
                }

                closedir(sys_block_subdir);
            }
        }

        closedir(sys_block_dir);
    }

    return 0;
}
