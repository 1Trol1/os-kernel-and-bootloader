#define EFIAPI __attribute__((ms_abi))
#include <efi.h>
#include <efilib.h>

typedef void (*kernel_entry_t)(void);

typedef struct {
    unsigned char e_ident[16];
    UINT16  e_type;
    UINT16  e_machine;
    UINT32  e_version;
    UINT64  e_entry;
    UINT64  e_phoff;
    UINT64  e_shoff;
    UINT32  e_flags;
    UINT16  e_ehsize;
    UINT16  e_phentsize;
    UINT16  e_phnum;
    UINT16  e_shentsize;
    UINT16  e_shnum;
    UINT16  e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    UINT32  p_type;
    UINT32  p_flags;
    UINT64  p_offset;
    UINT64  p_vaddr;
    UINT64  p_paddr;
    UINT64  p_filesz;
    UINT64  p_memsz;
    UINT64  p_align;
} Elf64_Phdr;

#define PT_LOAD 1

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"KonradOS ELF UEFI loader startuje...\n");

    EFI_STATUS status;
    EFI_LOADED_IMAGE *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_HANDLE RootDir;
    EFI_FILE_HANDLE KernelFile;

    status = uefi_call_wrapper(
        SystemTable->BootServices->HandleProtocol,
        3,
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (void**)&LoadedImage
    );
    if (EFI_ERROR(status)) return status;

    status = uefi_call_wrapper(
        SystemTable->BootServices->HandleProtocol,
        3,
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void**)&FileSystem
    );
    if (EFI_ERROR(status)) return status;

    status = uefi_call_wrapper(
        FileSystem->OpenVolume,
        2,
        FileSystem,
        &RootDir
    );
    if (EFI_ERROR(status)) return status;

    // Ładujemy ELF: \KERNEL.ELF
    status = uefi_call_wrapper(
        RootDir->Open,
        5,
        RootDir,
        &KernelFile,
        L"\\KERNEL.ELF",
        EFI_FILE_MODE_READ,
        0
    );
    if (EFI_ERROR(status)) {
        Print(L"Blad: nie moge otworzyc \\KERNEL.ELF: %r\n", status);
        return status;
    }

    // Nagłówek ELF
    Elf64_Ehdr ehdr;
    UINTN size = sizeof(ehdr);
    status = uefi_call_wrapper(
        KernelFile->Read,
        3,
        KernelFile,
        &size,
        &ehdr
    );
    if (EFI_ERROR(status) || size != sizeof(ehdr)) {
        Print(L"Blad: nie moge odczytac naglowka ELF\n");
        KernelFile->Close(KernelFile);
        return EFI_LOAD_ERROR;
    }

    // Sprawdzenie magic
    if (ehdr.e_ident[0] != 0x7F || ehdr.e_ident[1] != 'E' ||
        ehdr.e_ident[2] != 'L'  || ehdr.e_ident[3] != 'F') {
        Print(L"Blad: to nie jest ELF\n");
        KernelFile->Close(KernelFile);
        return EFI_LOAD_ERROR;
    }

    // Wczytaj tablicę program headers
    UINTN ph_size = ehdr.e_phentsize * ehdr.e_phnum;
    Elf64_Phdr *phdrs = AllocatePool(ph_size);
    if (!phdrs) {
        KernelFile->Close(KernelFile);
        return EFI_OUT_OF_RESOURCES;
    }

    // Ustaw pozycję na e_phoff
    status = uefi_call_wrapper(
        KernelFile->SetPosition,
        2,
        KernelFile,
        ehdr.e_phoff
    );
    if (EFI_ERROR(status)) {
        KernelFile->Close(KernelFile);
        FreePool(phdrs);
        return status;
    }

    size = ph_size;
    status = uefi_call_wrapper(
        KernelFile->Read,
        3,
        KernelFile,
        &size,
        phdrs
    );
    if (EFI_ERROR(status) || size != ph_size) {
        KernelFile->Close(KernelFile);
        FreePool(phdrs);
        return EFI_LOAD_ERROR;
    }

    // Ładuj segmenty PT_LOAD
    for (UINT16 i = 0; i < ehdr.e_phnum; i++) {
        Elf64_Phdr *ph = &phdrs[i];
        if (ph->p_type != PT_LOAD) continue;

        UINTN seg_pages = (ph->p_memsz + 0xFFF) / 0x1000;
        EFI_PHYSICAL_ADDRESS seg_addr = ph->p_paddr ? ph->p_paddr : ph->p_vaddr;

        status = uefi_call_wrapper(
            SystemTable->BootServices->AllocatePages,
            4,
            AllocateAddress,
            EfiLoaderData,
            seg_pages,
            &seg_addr
        );
        if (EFI_ERROR(status)) {
            Print(L"Blad: AllocatePages dla segmentu: %r\n", status);
            KernelFile->Close(KernelFile);
            FreePool(phdrs);
            return status;
        }

        // Wczytaj dane z pliku
        status = uefi_call_wrapper(
            KernelFile->SetPosition,
            2,
            KernelFile,
            ph->p_offset
        );
        if (EFI_ERROR(status)) {
            KernelFile->Close(KernelFile);
            FreePool(phdrs);
            return status;
        }

        UINTN to_read = ph->p_filesz;
        status = uefi_call_wrapper(
            KernelFile->Read,
            3,
            KernelFile,
            &to_read,
            (void*)(UINTN)seg_addr
        );
        if (EFI_ERROR(status) || to_read != ph->p_filesz) {
            KernelFile->Close(KernelFile);
            FreePool(phdrs);
            return EFI_LOAD_ERROR;
        }

        // Wyzeruj resztę (BSS)
        if (ph->p_memsz > ph->p_filesz) {
            UINTN diff = ph->p_memsz - ph->p_filesz;
            SetMem((void*)(UINTN)(seg_addr + ph->p_filesz), diff, 0);
        }
    }

    KernelFile->Close(KernelFile);
    FreePool(phdrs);

    Print(L"ELF zaladowany, e_entry = 0x%lx\n", (UINTN)ehdr.e_entry);

    // ExitBootServices
    UINTN MapSize = 0, MapKey, DescSize;
    UINT32 DescVersion;

    status = uefi_call_wrapper(
        SystemTable->BootServices->GetMemoryMap,
        5,
        &MapSize,
        NULL,
        &MapKey,
        &DescSize,
        &DescVersion
    );

    MapSize += DescSize * 4;
    EFI_MEMORY_DESCRIPTOR *MemMap = NULL;
    status = uefi_call_wrapper(
        SystemTable->BootServices->AllocatePool,
        3,
        EfiLoaderData,
        MapSize,
        (void**)&MemMap
    );
    if (EFI_ERROR(status)) return status;

    status = uefi_call_wrapper(
        SystemTable->BootServices->GetMemoryMap,
        5,
        &MapSize,
        MemMap,
        &MapKey,
        &DescSize,
        &DescVersion
    );
    if (EFI_ERROR(status)) return status;

    status = uefi_call_wrapper(
        SystemTable->BootServices->ExitBootServices,
        2,
        ImageHandle,
        MapKey
    );
    if (EFI_ERROR(status)) return status;

    // Skok do entry ELF – to jest _start z entry.asm
    kernel_entry_t kernel_entry = (kernel_entry_t)(UINTN)ehdr.e_entry;
    kernel_entry();

    return EFI_SUCCESS;
}

