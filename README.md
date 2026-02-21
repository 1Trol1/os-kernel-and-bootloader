# OS kernel and bootloader

## Here I describe my kernel: what it is supposed to do, what it currently does, and what it might be doing — although I’m not 100% sure about all details.

### I also describe my hardware specification of the project.

The kernel is tailored for:

- Laptop HP 15-dw1001nw

- AMD x86_64 architecture

- NVMe support (not sure if fully implemented)

- iGPU support (not sure if it works correctly)

- VGA text mode (NOT GOP)

- built‑in PS/2 keyboard

The kernel is not meant to support anything beyond this.
It is not universal — it only works on this specific hardware configuration.



### The kernel is written entirely with the help of Copilot.
It may contain mistakes, because AI can lose context and generate fragments
that later need to be corrected.

All libraries are in a single folder because it makes compilation with Makefile easier.



### Things that may be unclear — kernel libraries:

kheap — a library that has NOTHING to do with loading the kernel.
It is meant as a template for loading other binaries from the kernel later,
basically for running user‑mode applications. For now it’s not important.

It provides simple dynamic memory allocation (a bump allocator),
used for example when reading files.

loader — a library that has NOTHING to do with loading the kernel itself.
It is meant as a template for loading other binaries from the kernel later,
basically for running user‑mode applications. For now it’s not important.

It loads a file as raw machine code (no sections, no ELF),
and then jumps to the entry() function of the loaded binary.
It does not support complex formats or relocations.



### There is no guarantee that everything here is implemented correctly.
Some parts may be inconsistent.

I think the GDT and TSS might not be fully implemented in the kernel,
but I’m not completely sure.

The PS/2 driver is probably not implemented at all.
Do not confuse the “PS/2” driver with the “keyboard” driver.



### Problems:

After compiling and trying to run the bootloader, I either get a "Reboot system" and the machine restarts, which I assume is because my bootloader code is incorrect
or not properly adapted to my hardware.

The bootloader probably loads the kernel into memory, but does not jump to kernel's entry point,
even though entry.asm is present in the folder.
The Makefile does not link entry.asm into the kernel in any special way.

I also don’t know whether the jump from the bootloader to entry.asm is correct or even happening.

I would appreciate help fixing these issues or explaining what I am doing wrong.
