struct boot_sector
{
    unsigned char jump_instruction[3];
    unsigned char oem_id[8];
    unsigned short bytes_per_sector;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char number_of_fats;
    unsigned short root_entries;
    unsigned short small_sectors;
    unsigned char media_descriptor;
    unsigned short sectors_per_fat16;
    unsigned short sectors_per_track;
    unsigned short number_of_heads;
    unsigned int hidden_sectors;
    unsigned int large_sectors;
    unsigned int sectors_per_fat32;
    unsigned short unused;
    unsigned short file_system_version;
    unsigned int root_cluster_number;
    unsigned short fs_info_sector_number;
    unsigned short backup_boot_sector;
    unsigned char reserved_fat32[12];
    unsigned char physical_drive_number;
    unsigned char reserved;
    unsigned char extended_boot_signature;
    unsigned int volume_serial_number;
    unsigned char volume_label[11];
    unsigned char system_id[8];
}__attribute__((packed));

struct fsinfo
{
    unsigned int start_signature;
    unsigned char reserved0[480];
    unsigned int middle_signature;
    unsigned int last_known_free_cluster;
    unsigned int last_known_allocated_cluster;
    unsigned char reserved1[12];
    unsigned int end_signature;
}__attribute__((packed));

struct directory_entry
{
    unsigned char file_name[8];
    unsigned char extension[3];
    unsigned char attributes;
    unsigned char mark;
    unsigned char creation_time_ms;
    unsigned short creation_time;
    unsigned short creation_date;
    unsigned short last_access_date;
    unsigned short high_two_bytes;
    unsigned short last_modification_time;
    unsigned short last_modification_date;
    unsigned short low_two_bytes;
    unsigned int file_size;
}__attribute__((packed));

struct long_file_name_entry
{
    unsigned char sequence_number;
    unsigned char name_characters0[10];
    unsigned char attributes;
    unsigned char type;
    unsigned char checksum;
    unsigned char name_characters1[12];
    unsigned short first_cluster;
    unsigned char name_characters2[4];
}__attribute__((packed));