#pragma once
#include <stdint.h>

struct fat32_bpb {
    uint8_t  jmp[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
} __attribute__((packed));

struct fat32_dir_entry {
    char     name[11];
    uint8_t  attr;
    uint8_t  ntres;
    uint8_t  crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed));

//
// *** BRAKOWAŁO TEGO ***
//
struct fat32_file {
    uint32_t first_cluster;
    uint32_t size;
    uint32_t current_cluster;
    uint32_t current_offset;
    // dopisz pola, jeśli masz więcej w fat32.c
};

int fat32_mount(uint64_t part_start_lba, uint32_t block_size);
int fat32_open(const char* path, struct fat32_file* out);
int fat32_read_all(struct fat32_file* f, void* buf, uint32_t size);
void fat32_list_root(void);
int fat32_read_file_from_root(const char* name,
                              void* buffer,
                              uint32_t buffer_size,
                              uint32_t* out_size);

