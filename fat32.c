#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "fat32.h"
#include "fat32_consts.c"
#include "fat32_utils.c"
#include <stdio.h>

struct fs_structures *open_fs(char *target_path)
{
    int target = open(target_path, O_RDONLY, 00444);
    struct fs_structures *structures;

    if (target != -1)
    {
        struct boot_sector *boot_sector_structure = malloc(sizeof(struct boot_sector));
        pread(target, boot_sector_structure, sizeof(struct boot_sector), 0);

        struct fsinfo *fsinfo_structure = malloc(sizeof(struct fsinfo));
        pread(target, fsinfo_structure, sizeof(struct fsinfo), boot_sector_structure->fs_info_sector_number * boot_sector_structure->bytes_per_sector);

        structures = malloc(sizeof(struct fs_structures));
        structures->file_descriptor = target;
        structures->current_cluster = boot_sector_structure->root_cluster_number;
        structures->current_path = calloc(1, MAX_PATH_LENGTH);
        strcat(structures->current_path, "/");
        structures->boot_sector_structure = boot_sector_structure;
        structures->fsinfo_structure = fsinfo_structure;
    }
    else
    {
    	return NULL;
    }

    if (structures->fsinfo_structure->start_signature == 0x41615252 && structures->fsinfo_structure->middle_signature == 0x61417272 
        && structures->fsinfo_structure->end_signature == 0xAA550000)
    {
        return structures;
    }
    else
    {
        close(target);
        return NULL;
    }
}

void close_fs(struct fs_structures *structures)
{
    close(structures->file_descriptor);
    free(structures->current_path);
    free(structures->boot_sector_structure);
    free(structures->fsinfo_structure);
    free(structures);
}

unsigned int find_first_sector(unsigned int cluster, struct boot_sector *boot_sector_structure)
{
    // first data sector = relerved sectors + num of fats * sectors per fat32
    unsigned int first_data_sector = boot_sector_structure->reserved_sectors + (boot_sector_structure->number_of_fats * boot_sector_structure->sectors_per_fat32);

    return (cluster - boot_sector_structure->root_cluster_number) * boot_sector_structure->sectors_per_cluster + first_data_sector;
}

unsigned int read_table_address(struct fs_structures *structures, unsigned int current_cluster)
{
    unsigned short sector_size = structures->boot_sector_structure->bytes_per_sector;

    unsigned int table_offset = current_cluster * FAT32_CLUSTER_ADDRESS_LENGTH;
    unsigned int address;
    pread(structures->file_descriptor, &address, sizeof(address), structures->boot_sector_structure->reserved_sectors * sector_size + table_offset);

    return address;
}

struct file *create_file(struct directory_entry *entry, char *file_name, struct boot_sector *boot_sector_structure)
{
    struct file *file = calloc(1, sizeof(struct file));
    file->file_name = file_name;
    file->is_directory = (entry->attributes & 0x20) != 0x20;
    if ((file->first_cluster = (entry->high_two_bytes << 16) + entry->low_two_bytes) == 0)
    {
        file->first_cluster = 2;
    }
    file->size = entry->file_size;

    return file;
}

void read_long_file_name(struct long_file_name_entry *long_file_name_entry, char *file_name)
{
    int offset = 0;
    for (int i = 0; i < sizeof(long_file_name_entry->name_characters0); i += 2)
    {
        file_name[offset++] = (char)long_file_name_entry->name_characters0[i];
    }
    for (int i = 0; i < sizeof(long_file_name_entry->name_characters1); i += 2)
    {
        file_name[offset++] = (char)long_file_name_entry->name_characters1[i];
    }
    for (int i = 0; i < sizeof(long_file_name_entry->name_characters2); i += 2)
    {
        file_name[offset++] = (char)long_file_name_entry->name_characters2[i];
    }
}

void build_long_file_name(char *file_name, char **long_file_name_parts, char long_file_name_entries_count)
{
    for (int i = long_file_name_entries_count - 1; i >= 0 ; i--)
    {
        strcat((char*) file_name, long_file_name_parts[i]);
        free(long_file_name_parts[i]);
    }
}

void build_file_name(char *file_name, struct directory_entry *entry)
{
    strcpy(file_name, entry->file_name);
    for (int i = strlen(file_name) - 1; i >= 0; i--)
    {
        if (file_name[i] <= 32)
        {
            file_name[i] = 0;
        }
        else
        {
            break;
        }
    }

    int is_extension_empty = 1;

    for (int i = strlen(entry->extension) - 1; i >= 0; i--)
    {
        if (entry->extension[i] > 32)
        {
            is_extension_empty = 0;
            break;
        }
    }

    if (!is_extension_empty)
    {
        strcat(file_name, ".");
        strcat(file_name, entry->file_name);
    }

    for (int i = sizeof(strlen(file_name)) - 1; i >= 0; i--)
    {
        if (file_name[i] <= 32)
        {
            file_name[i] = 0;
        }
        else
        {
            break;
        }
    }
}

void free_file(struct file *file)
{
    struct file *current_file;
    while (file != NULL)
    {
        free(file->file_name);
        current_file = file;
        file = current_file->next_file;
        free(current_file);
    }
}

struct file *read_directory(struct fs_structures *structures, unsigned int cluster)
{
    unsigned int sector_size = structures->boot_sector_structure->bytes_per_sector;
    unsigned int current_sector_address = find_first_sector(cluster, structures->boot_sector_structure) * sector_size;
    int entries_in_cluster = structures->boot_sector_structure->bytes_per_sector * structures->boot_sector_structure->sectors_per_cluster / sizeof(struct directory_entry);

    struct directory_entry *entries = calloc(entries_in_cluster, sizeof(struct directory_entry));;

    char *long_file_name_parts[LONG_FILE_NAME_MAX_ENTRIES];
    char long_file_name_entries_count = 0;

    char has_entries = 1;
    int entries_count = -1;

    struct file *first_file = NULL;
    struct file *last_file = NULL;

    do
    {
        if (entries_count == -1 || entries_count >= entries_in_cluster)
        {
            if (cluster >= 0x0FFFFFF8)
            {
                has_entries = 0;
            }
            else
            {
                current_sector_address = find_first_sector(cluster, structures->boot_sector_structure) * sector_size;
                pread(structures->file_descriptor, entries, entries_in_cluster * sizeof(struct directory_entry), current_sector_address);
                entries_count = 0;
                cluster = read_table_address(structures, cluster);
            }
        }

        struct directory_entry *entry = &entries[entries_count++];

        if (entry->file_name[0] == 0x00)
        {
            has_entries = 0;
        }
        else if (entry->file_name[0] == 0xE5)
        {
            continue;
        }
        else
        {
            if (entry->attributes & 0x10 || entry->attributes & 0x20)
            {
                char *file_name;

                if (long_file_name_entries_count)
                {
                    file_name = calloc(1, long_file_name_entries_count * 13);
                    build_long_file_name(file_name, long_file_name_parts, long_file_name_entries_count);
                    long_file_name_entries_count = 0;
                }
                else
                {
                    file_name = calloc(1, sizeof(entry->file_name) + sizeof(entry->extension) + 1);
                    build_file_name(file_name, entry);
                }

                struct file *file = create_file(entry, file_name, structures->boot_sector_structure);

                if (first_file == NULL)
                {
                    first_file = file;
                }
                else
                {
                    last_file->next_file = file;
                }
                last_file = file;
            }
            else if (entry->attributes == 0x0F)
            {
                // long file name entry
                long_file_name_parts[long_file_name_entries_count] = calloc(1, 13);
                read_long_file_name((struct long_file_name_entry*) entry, long_file_name_parts[long_file_name_entries_count]);
                long_file_name_entries_count++;
            }
        }
    } while(has_entries);

    free(entries);
    return first_file;
}

struct file *find_directory(struct fs_structures *structures, char *target_path, struct file *current_directory)
{
    if (target_path[0] == '/') {
        return NULL;
    }

    char normalized_path[MAX_PATH_LENGTH];
    strcpy(normalized_path, target_path);
    normalize_path(normalized_path);

    if (!strcmp("", normalized_path))
    {
        return current_directory;
    }

    char target_path_part[MAX_PATH_LENGTH] = {0};
    unsigned int target_path_pointer = 0;
    unsigned int target_path_length = strlen(normalized_path);

    struct file *current_file = current_directory;

    for (int i = 0; i <= target_path_length; i++)
    {
        if (i < target_path_length && normalized_path[i] != '/')
        {
            target_path_part[target_path_pointer++] = normalized_path[i];
        }
        else
        {

            while (current_file != NULL)
            {
                if (!strcmp(current_file->file_name, target_path_part))
                {
                    if (current_file->is_directory)
                    {
                        if (i == target_path_length || ((i + 1 == target_path_length) && normalized_path[i] == '/'))
                        {
                            // last part in path. Target directory was reached
                            return read_directory(structures, current_file->first_cluster);
                        }
                        else
                        {
                            char new_path[MAX_PATH_LENGTH] = {0};
                            strncpy(new_path, normalized_path + target_path_pointer + 1, strlen(normalized_path) - target_path_pointer);

                            struct file *next_directory = read_directory(structures, current_file->first_cluster);
                            struct file *target_directory = find_directory(structures, new_path, next_directory);
                            if (target_directory != next_directory)
                            {
                                free(next_directory);
                            }
                            
                            return target_directory;
                        }
                    }
                    break;
                }
                else
                {
                    current_file = current_file->next_file;
                }
            }

            return NULL;
        }
    }

    return NULL;
}

unsigned int move_to_directory(struct fs_structures *fs_structures, char *path)
{
    struct file *file;
    if (path[0] == '/')
    {
        struct file *first_directory = read_directory(fs_structures, fs_structures->boot_sector_structure->root_cluster_number);  
        file = find_directory(fs_structures, path + 1, first_directory);
        if (file != first_directory)
        {
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
        if (!strcmp(".", file->file_name))
        {
            fs_structures->current_cluster = file->first_cluster;
        }
        else
        {
            fs_structures->current_cluster = fs_structures->boot_sector_structure->root_cluster_number;
        }
        free(file);

        char *new_path = calloc(1, MAX_PATH_LENGTH);
        if (path[0] == '/')
        {
            strcat(new_path, path);
        }
        else
        {
            strcat(new_path, fs_structures->current_path);
            if (strlen(fs_structures->current_path) != 1) {
                strcat(new_path, "/");
            }
            strcat(new_path, path);
        }
        normalize_path(new_path);
        remove_back_directories(new_path);
        free(fs_structures->current_path);
        fs_structures->current_path = new_path;

        return 0;
    }
    else
    {
        return 1;
    }
}

void read_cluster(struct fs_structures *structures, unsigned int current_cluster, char *cluster_buffer)
{
    unsigned int cluster_size = structures->boot_sector_structure->bytes_per_sector * structures->boot_sector_structure->sectors_per_cluster;
    unsigned int offset = find_first_sector(current_cluster, structures->boot_sector_structure) * structures->boot_sector_structure->bytes_per_sector;
    pread(structures->file_descriptor, cluster_buffer, cluster_size, offset);
}
