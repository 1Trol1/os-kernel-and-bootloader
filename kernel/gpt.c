#include "string.h"
#include "gpt.h"
#include "nvme.h"
#include "vga.h"

// GPT entries potrafią być duże: 128 wpisów * 128 bajtów = 16 KB
// więc bufor 4 KB to za mało.
static uint8_t gpt_buf[16 * 1024] __attribute__((aligned(4096)));

int gpt_read_header(uint32_t nsid, uint32_t block_size, struct gpt_header* out) {
    (void)block_size; // nie jest nam tu potrzebny

    // LBA1 – GPT header
    if (!nvme_read_blocks(nsid, 1, 1, gpt_buf, sizeof(gpt_buf))) {
        vga_print("GPT: failed to read LBA1\n");
        return 0;
    }

    struct gpt_header* hdr = (struct gpt_header*)gpt_buf;

    if (memcmp(hdr->signature, "EFI PART", 8) != 0) {
        vga_print("GPT: invalid signature\n");
        return 0;
    }

    if (hdr->part_entry_size != sizeof(struct gpt_entry)) {
        vga_print("GPT: unexpected entry size\n");
        return 0;
    }

    *out = *hdr;

    vga_print("GPT: header OK\n");
    return 1;
}

int gpt_read_entries(uint32_t nsid, uint32_t block_size,
                     const struct gpt_header* hdr, struct gpt_entry* entries) {
    uint32_t total_size = hdr->num_part_entries * hdr->part_entry_size;
    uint32_t blocks = (total_size + block_size - 1) / block_size;

    if (total_size > sizeof(gpt_buf)) {
        vga_print("GPT: entries too large for buffer\n");
        return 0;
    }

    if (!nvme_read_blocks(nsid, hdr->part_entries_lba, blocks,
                          gpt_buf, sizeof(gpt_buf))) {
        vga_print("GPT: failed to read partition entries\n");
        return 0;
    }

    memcpy(entries, gpt_buf, total_size);

    vga_print("GPT: entries OK\n");
    return 1;
}

int gpt_find_partition(const struct gpt_header* hdr, const struct gpt_entry* entries,
                       uint64_t* out_lba_start, uint64_t* out_lba_end) {

    for (uint32_t i = 0; i < hdr->num_part_entries; i++) {
        const struct gpt_entry* e = &entries[i];

        if (e->first_lba == 0)
            continue;

        // Microsoft Basic Data (typowe FAT32/NTFS):
        // EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
        static const uint8_t fat32_guid[16] = {
            0xA2,0xA0,0xD0,0xEB,0xE5,0xB9,0x33,0x44,
            0x87,0xC0,0x68,0xB6,0xB7,0x26,0x99,0xC7
        };

        // Linux data GUID: 0FC63DAF-8483-4772-8E79-3D69D8477DE4
        static const uint8_t linux_guid[16] = {
            0xAF,0x3D,0xC6,0x0F,0x83,0x84,0x72,0x47,
            0x8E,0x79,0x3D,0x69,0xD8,0x47,0x7D,0xE4
        };

        if (memcmp(e->type_guid, fat32_guid, 16) == 0 ||
            memcmp(e->type_guid, linux_guid, 16) == 0) {

            *out_lba_start = e->first_lba;
            *out_lba_end   = e->last_lba;

            vga_print("GPT: partition found\n");
            return 1;
        }
    }

    vga_print("GPT: no usable partition found\n");
    return 0;
}

