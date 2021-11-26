#include "fat32_structs.h"

struct fs_structures
{
    unsigned int file_descriptor;
    unsigned int current_cluster;
    char *current_path;
    struct boot_sector *boot_sector_structure;
    struct fsinfo *fsinfo_structure;
};

struct file
{
    char *file_name;
    unsigned char is_directory;
    unsigned int first_cluster;
    struct file *next_file;
    unsigned int size;
};

struct fs_structures *open_fs(char *target_path);

void close_fs(struct fs_structures *structures);

unsigned int read_table_address(struct fs_structures *structures, unsigned int current_cluster);

void free_file(struct file *file);

struct file *read_directory(struct fs_structures *structures, unsigned int cluster);

struct file *find_directory(struct fs_structures *structures, char *target_path, struct file *current_directory);

unsigned int move_to_directory(struct fs_structures *fs_structures, char *path);

void read_cluster(struct fs_structures *structures, unsigned int current_cluster, char *cluster_buffer);
