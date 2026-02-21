#include "fat32.h"
#include "nvme.h"
#include "vga.h"
#include <stdint.h>

#define FAT32_NSID 1

static struct fat32_bpb g_bpb;
static uint64_t g_part_start_lba;   // w LBA NVMe (z GPT)
static uint32_t g_block_size;       // rozmiar LBA NVMe (z NVMe)

static uint64_t g_first_fat_sector;   // w sektorach FAT (512B)
static uint64_t g_data_region_sector; // w sektorach FAT
static uint32_t g_root_cluster;
static uint16_t g_bytes_per_sector;
static uint8_t  g_sectors_per_cluster;

static uint8_t  fat32_sector_buf[4096] __attribute__((aligned(4096)));
static uint8_t  fat32_cluster_buf[4096 * 8] __attribute__((aligned(4096)));
static uint8_t  fat32_fat_buf[4096 * 8] __attribute__((aligned(4096))); // 32 KB

static uint32_t* g_fat_table = 0;
static uint32_t  g_fat_entries = 0;

// ile sektorów FAT32 mieści się w jednym bloku NVMe
static inline uint32_t sectors_per_block(void) {
    return g_block_size / g_bytes_per_sector;
}

// czytanie sektorów FAT32 (512 B) przez NVMe (block_size B)
static int fat32_read_sectors(uint64_t lba_sector, uint32_t sector_count, void* buffer) {
    uint32_t spb = sectors_per_block();
    if (spb == 0) return 0;

    uint8_t* dst = (uint8_t*)buffer;

    while (sector_count > 0) {
        uint64_t first_block    = lba_sector / spb;
        uint32_t offset_sectors = lba_sector % spb;
        uint32_t sectors_here   = spb - offset_sectors;

        if (sectors_here > sector_count)
            sectors_here = sector_count;

        static uint8_t tmp_block[4096] __attribute__((aligned(4096)));

        if (!nvme_read_blocks(FAT32_NSID, first_block, 1, tmp_block, g_block_size)) {
            vga_print("FAT32: nvme_read_blocks failed\n");
            return 0;
        }

        uint32_t byte_offset   = offset_sectors * g_bytes_per_sector;
        uint32_t bytes_to_copy = sectors_here * g_bytes_per_sector;

        uint8_t* src = tmp_block + byte_offset;
        for (uint32_t i = 0; i < bytes_to_copy; i++) {
            dst[i] = src[i];
        }

        dst         += bytes_to_copy;
        lba_sector  += sectors_here;
        sector_count -= sectors_here;
    }

    return 1;
}

static uint64_t fat32_cluster_to_sector(uint32_t cluster) {
    uint64_t rel = (uint64_t)(cluster - 2) * g_sectors_per_cluster;
    return g_data_region_sector + rel; // w sektorach FAT
}

static int fat32_read_cluster(uint32_t cluster, void* buffer) {
    uint64_t first_sector = fat32_cluster_to_sector(cluster);
    return fat32_read_sectors(first_sector, g_sectors_per_cluster, buffer);
}

int fat32_mount(uint64_t part_start_lba, uint32_t block_size) {
    g_part_start_lba = part_start_lba; // w LBA NVMe
    g_block_size     = block_size;

    // 1) Czytamy pierwszy blok NVMe i bierzemy pierwsze 512 B jako BPB
    if (!nvme_read_blocks(FAT32_NSID, g_part_start_lba, 1, fat32_sector_buf, g_block_size)) {
        vga_print("FAT32: failed to read boot block\n");
        return 0;
    }

    struct fat32_bpb* bpb = (struct fat32_bpb*)fat32_sector_buf;
    g_bpb = *bpb;

    g_bytes_per_sector    = bpb->bytes_per_sector;
    g_sectors_per_cluster = bpb->sectors_per_cluster;
    g_root_cluster        = bpb->root_cluster;

    if (g_bytes_per_sector == 0 || (g_block_size % g_bytes_per_sector) != 0) {
        vga_print("FAT32: invalid bytes_per_sector vs block_size\n");
        return 0;
    }

    uint32_t spb = sectors_per_block();
    uint64_t part_start_sector = g_part_start_lba * spb;

    uint32_t fat_size = bpb->fat_size_32 ? bpb->fat_size_32 : bpb->fat_size_16;

    g_first_fat_sector   = part_start_sector + bpb->reserved_sectors;
    g_data_region_sector = g_first_fat_sector + (uint64_t)bpb->num_fats * fat_size;

    vga_print("FAT32: mounted\n");
    vga_print("  bytes/sector: ");
    vga_print_hex(g_bytes_per_sector);
    vga_print("\n");

    vga_print("  sectors/cluster: ");
    vga_print_hex(g_sectors_per_cluster);
    vga_print("\n");

    vga_print("  root_cluster: ");
    vga_print_hex(g_root_cluster);
    vga_print("\n");

    vga_print("  first_fat_sector: ");
    vga_print_hex64(g_first_fat_sector);
    vga_print("\n");

    vga_print("  data_region_sector: ");
    vga_print_hex64(g_data_region_sector);
    vga_print("\n");

    // --- wczytanie FAT do bufora (demo: max 32 KB) ---
    uint32_t fat_sectors = fat_size;
    uint32_t fat_bytes   = fat_sectors * g_bytes_per_sector;

    if (fat_bytes > sizeof(fat32_fat_buf)) {
        vga_print("FAT32: FAT too big for static buffer (demo limit)\n");
        return 1; // montaż OK, ale bez FAT chain
    }

    if (!fat32_read_sectors(g_first_fat_sector, fat_sectors, fat32_fat_buf)) {
        vga_print("FAT32: failed to read FAT\n");
        return 0;
    }

    g_fat_table  = (uint32_t*)fat32_fat_buf;
    g_fat_entries = fat_bytes / 4;

    vga_print("FAT32: FAT loaded\n");

    return 1;
}

static uint32_t fat32_next_cluster(uint32_t cluster) {
    if (!g_fat_table) return 0x0FFFFFFF;
    if (cluster >= g_fat_entries) return 0x0FFFFFFF;

    uint32_t val = g_fat_table[cluster] & 0x0FFFFFFF;
    return val;
}

void fat32_list_root(void) {
    if (!g_root_cluster) {
        vga_print("FAT32: not mounted\n");
        return;
    }

    if (!fat32_read_cluster(g_root_cluster, fat32_cluster_buf)) {
        vga_print("FAT32: failed to read root cluster\n");
        return;
    }

    uint32_t cluster_bytes = (uint32_t)g_sectors_per_cluster * g_bytes_per_sector;
    uint32_t entries = cluster_bytes / sizeof(struct fat32_dir_entry);

    vga_print("FAT32: root directory:\n");

    for (uint32_t i = 0; i < entries; i++) {
        struct fat32_dir_entry* e =
            (struct fat32_dir_entry*)(fat32_cluster_buf + i * sizeof(struct fat32_dir_entry));

        if (e->name[0] == 0x00)
            break;
        if (e->name[0] == (char)0xE5)
            continue;
        if ((e->attr & 0x0F) == 0x0F)
            continue;

        char name[13];
        int p = 0;

        for (int j = 0; j < 8; j++) {
            if (e->name[j] == ' ')
                break;
            name[p++] = e->name[j];
        }

        if (e->name[8] != ' ') {
            name[p++] = '.';
            for (int j = 8; j < 11; j++) {
                if (e->name[j] == ' ')
                    break;
                name[p++] = e->name[j];
            }
        }

        name[p] = 0;

        vga_print("  ");
        vga_print(name);
        vga_print("  size=");
        vga_print_hex64(e->file_size);
        vga_print("\n");
    }
}

static int fat32_names_equal_8_3(const char* a11, const char* name) {
    char temp[12];
    int p = 0;

    for (int j = 0; j < 8; j++) {
        if (a11[j] == ' ')
            break;
        temp[p++] = a11[j];
    }

    if (a11[8] != ' ') {
        temp[p++] = '.';
        for (int j = 8; j < 11; j++) {
            if (a11[j] == ' ')
                break;
            temp[p++] = a11[j];
        }
    }

    temp[p] = 0;

    const char* s = name;
    const char* t = temp;
    while (*s && *t) {
        if (*s != *t) return 0;
        s++; t++;
    }
    return (*s == 0 && *t == 0);
}

int fat32_read_file_from_root(const char* name, void* buffer,
                              uint32_t buffer_size, uint32_t* out_size) {
    if (!g_root_cluster) {
        vga_print("FAT32: not mounted\n");
        return 0;
    }

    if (!fat32_read_cluster(g_root_cluster, fat32_cluster_buf)) {
        vga_print("FAT32: failed to read root cluster\n");
        return 0;
    }

    uint32_t cluster_bytes = (uint32_t)g_sectors_per_cluster * g_bytes_per_sector;
    uint32_t entries = cluster_bytes / sizeof(struct fat32_dir_entry);

    struct fat32_dir_entry* found = 0;

    for (uint32_t i = 0; i < entries; i++) {
        struct fat32_dir_entry* e =
            (struct fat32_dir_entry*)(fat32_cluster_buf + i * sizeof(struct fat32_dir_entry));

        if (e->name[0] == 0x00)
            break;
        if (e->name[0] == (char)0xE5)
            continue;
        if ((e->attr & 0x0F) == 0x0F)
            continue;
        if (e->attr & 0x10)
            continue;

        if (fat32_names_equal_8_3(e->name, name)) {
            found = e;
            break;
        }
    }

    if (!found) {
        vga_print("FAT32: file not found in root\n");
        return 0;
    }

    uint32_t first_cluster =
        ((uint32_t)found->first_cluster_high << 16) | found->first_cluster_low;
    uint32_t file_size = found->file_size;

    if (out_size) *out_size = file_size;

    uint8_t* out = (uint8_t*)buffer;
    uint32_t remaining = file_size;
    uint32_t cluster = first_cluster;

    while (cluster >= 2 && cluster < 0x0FFFFFF8 && remaining > 0 && buffer_size > 0) {
        if (!fat32_read_cluster(cluster, fat32_cluster_buf)) {
            vga_print("FAT32: failed to read file cluster\n");
            return 0;
        }

        uint32_t to_copy = cluster_bytes;
        if (to_copy > remaining)   to_copy = remaining;
        if (to_copy > buffer_size) to_copy = buffer_size;

        for (uint32_t i = 0; i < to_copy; i++) {
            out[i] = fat32_cluster_buf[i];
        }

        out         += to_copy;
        buffer_size -= to_copy;
        remaining   -= to_copy;

        if (remaining == 0 || buffer_size == 0)
            break;

        uint32_t next = fat32_next_cluster(cluster);
        if (next == 0x0FFFFFFF || next == 0) break;
        cluster = next;
    }

    vga_print("FAT32: file read\n");
    return 1;
}

int fat32_open(const char* path, struct fat32_file* out) {
    uint32_t size = 0;
    // używamy tymczasowego bufora tylko po to, żeby poznać rozmiar
    if (!fat32_read_file_from_root(path, fat32_cluster_buf,
                                   sizeof(fat32_cluster_buf), &size)) {
        return 0;
    }

    // znajdź wpis jeszcze raz, żeby wyciągnąć first_cluster
    if (!fat32_read_cluster(g_root_cluster, fat32_cluster_buf)) {
        return 0;
    }

    uint32_t cluster_bytes = (uint32_t)g_sectors_per_cluster * g_bytes_per_sector;
    uint32_t entries = cluster_bytes / sizeof(struct fat32_dir_entry);

    for (uint32_t i = 0; i < entries; i++) {
        struct fat32_dir_entry* e =
            (struct fat32_dir_entry*)(fat32_cluster_buf + i * sizeof(struct fat32_dir_entry));

        if (e->name[0] == 0x00)
            break;
        if (e->name[0] == (char)0xE5)
            continue;
        if ((e->attr & 0x0F) == 0x0F)
            continue;
        if (e->attr & 0x10)
            continue;

        if (fat32_names_equal_8_3(e->name, path)) {
            uint32_t first_cluster =
                ((uint32_t)e->first_cluster_high << 16) | e->first_cluster_low;
            out->first_cluster = first_cluster;
            out->size          = e->file_size;
            return 1;
        }
    }

    return 0;
}

int fat32_read_all(struct fat32_file* f, void* buf, uint32_t size) {
    (void)f;
    uint32_t dummy = 0;
    (void)dummy;
    // na razie po prostu użyj istniejącej funkcji
    return fat32_read_file_from_root("KERNEL  BIN", buf, size, &dummy);
}

