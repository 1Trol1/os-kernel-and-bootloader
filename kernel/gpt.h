#pragma once
#include <stdint.h>

struct gpt_header {
    char     signature[8];      // "EFI PART"
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t part_entries_lba;
    uint32_t num_part_entries;
    uint32_t part_entry_size;
    uint32_t part_entries_crc32;
} __attribute__((packed));

struct gpt_entry {
    uint8_t  type_guid[16];
    uint8_t  unique_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attrs;
    uint16_t name[36]; // UTF-16
} __attribute__((packed));

// --- DODAJ TO ---

int gpt_read_header(uint32_t nsid, uint32_t block_size, struct gpt_header* out);

int gpt_read_entries(uint32_t nsid, uint32_t block_size,
                     const struct gpt_header* hdr, struct gpt_entry* entries);

int gpt_find_partition(const struct gpt_header* hdr, const struct gpt_entry* entries,
                       uint64_t* out_lba_start, uint64_t* out_lba_end);

