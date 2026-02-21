# ============================
#  CONFIG
# ============================
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
NASM = nasm

CFLAGS = -ffreestanding -O2 -Wall -Wextra \
		-mno-red-zone -fshort-wchar \
		-I. -I/usr/include/efi -I/usr/include/efi/x86_64

BOOT_CFLAGS = $(CFLAGS) -fPIC

LDFLAGS = -nostdlib

# ============================
#  UEFI (gnu-efi)
# ============================

EFI_DIR = /usr/lib
EFI_CRT = $(EFI_DIR)/crt0-efi-x86_64.o
EFI_LDS = $(EFI_DIR)/elf_x86_64_efi.lds
EFI_LIBS = -L$(EFI_DIR) -lgnuefi -lefi

# ============================
#  KERNEL SOURCES
# ============================

# NIE WYCINAMY entry.asm !!!
KERNEL_SRC = $(filter-out bootloader.c, $(wildcard *.c))

ASM_SRC = irq.asm isr.asm gdt_flush.asm entry.asm
ASM_OBJ = irq_asm.o isr_asm.o gdt_flush.o entry.o

KERNEL_OBJ = $(KERNEL_SRC:.c=.o) $(ASM_OBJ)

# ============================
#  BOOTLOADER SOURCES (UEFI)
# ============================

BOOT_C = bootloader.c
BOOT_OBJ = bootloader.o

# ============================
#  TARGETS
# ============================

all: kernel.elf bootloader.efi

# ============================
#  BUILD KERNEL (ELF)
# ============================

kernel.elf: $(KERNEL_OBJ)
	$(LD) -T kernel.ld -o $@ $(KERNEL_OBJ) $(LDFLAGS)

# ============================
#  BUILD BOOTLOADER (UEFI PE/COFF)
# ============================

bootloader.efi: $(BOOT_OBJ)
	$(LD) -nostdlib -znocombreloc -shared -Bsymbolic \
		-T $(EFI_LDS) \
		$(EFI_CRT) $(BOOT_OBJ) \
		-o $@ $(EFI_LIBS)

bootloader.o: bootloader.c
	$(CC) $(BOOT_CFLAGS) -c $< -o $@

# ============================
#  RULES
# ============================

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

irq_asm.o: irq.asm
	$(NASM) -f elf64 $< -o $@

isr_asm.o: isr.asm
	$(NASM) -f elf64 $< -o $@

gdt_flush.o: gdt_flush.asm
	$(NASM) -f elf64 $< -o $@

entry.o: entry.asm
	$(NASM) -f elf64 $< -o $@

clean:
	rm -f $(KERNEL_OBJ) $(BOOT_OBJ) kernel.elf bootloader.efi

