# ==================================================================================== #
# |                           DEFINITIONS                                  | #
# ==================================================================================== #
CC      = gcc
ASM     = nasm
LIN     = ld

SOURCE_FOLDER   = src
OUTPUT_FOLDER   = bin
DISK_NAME = storage


# Variabel untuk semua file objek yang akan di-link
# (load_gdt.o dihapus untuk menghindari konflik)
OBJECT_FILES = $(OUTPUT_FOLDER)/kernel-entrypoint.o \
$(OUTPUT_FOLDER)/kernel.o \
$(OUTPUT_FOLDER)/gdt.o \
$(OUTPUT_FOLDER)/framebuffer.o \
$(OUTPUT_FOLDER)/portio.o \
$(OUTPUT_FOLDER)/keyboard.o \
$(OUTPUT_FOLDER)/idt.o\
$(OUTPUT_FOLDER)/disk.o\
$(OUTPUT_FOLDER)/interrupt.o \
$(OUTPUT_FOLDER)/interrupt-asm.o \
$(OUTPUT_FOLDER)/string.o\
$(OUTPUT_FOLDER)/ext2.o \
$(OUTPUT_FOLDER)/paging.o
                

# Compiler flags
CFLAGS  = -ffreestanding -fshort-wchar -g -nostdlib -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -m32 -c -Isrc

# Assembler flags
AFLAGS  = -f elf32 -g -F dwarf

# Linker flags
LFLAGS  = -T src/linker.ld -melf_i386


# ==================================================================================== #
# |                                     BUILD                                        | #
# ==================================================================================== #

# Default target - build dan run
all: run

# Hanya build
build: iso

#Disk
disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 4M


# Build dan run
# Menambahkan 'insert-shell' sebagai dependensi agar shell selalu terbaru
run: iso insert-shell
	@echo "Running OS in QEMU..."
	@qemu-system-i386 -s -drive file=bin/storage.bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/OS2025.iso

kernel:
	@echo "Compiling assembly and C files..."
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel-entrypoint.s -o $(OUTPUT_FOLDER)/kernel-entrypoint.o
	# PERBAIKAN: Menggunakan intsetup.s dari folder interrupt
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/interrupt/intsetup.s -o $(OUTPUT_FOLDER)/interrupt-asm.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o $(OUTPUT_FOLDER)/kernel.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/keyboard.c -o $(OUTPUT_FOLDER)/keyboard.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/cpu/gdt.c -o $(OUTPUT_FOLDER)/gdt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/interrupt/idt.c -o $(OUTPUT_FOLDER)/idt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/framebuffer/framebuffer.c -o $(OUTPUT_FOLDER)/framebuffer.o
	# PERBAIKAN: Path portio.c sekarang ada di dalam framebuffer
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/framebuffer/portio.c -o $(OUTPUT_FOLDER)/portio.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/disk/disk.c -o $(OUTPUT_FOLDER)/disk.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/interrupt/interrupt.c -o $(OUTPUT_FOLDER)/interrupt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/stdlib/string.c -o $(OUTPUT_FOLDER)/string.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/filesystem/ext2.c -o $(OUTPUT_FOLDER)/ext2.o 
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/memory/paging.c -o $(OUTPUT_FOLDER)/paging.o
	@echo "Linking object files and generating kernel ELF..."
	@$(LIN) $(LFLAGS) $(OBJECT_FILES) -o $(OUTPUT_FOLDER)/kernel
	@rm -f $(OUTPUT_FOLDER)/*.o


iso: kernel
	@echo Creating ISO image...
	@mkdir -p bin/iso/boot/grub
	@cp bin/kernel bin/iso/boot/
	@cp other/grub1 bin/iso/boot/grub/
	@cp src/menu.lst bin/iso/boot/grub/

	@genisoimage -R \
	-b boot/grub/grub1 \
	-no-emul-boot \
	-boot-load-size 4 \
	-A os \
	-input-charset utf8 \
	-boot-info-table \
	-o bin/OS2025.iso \
	bin/iso

inserter:
	@echo "Compiling host program 'inserter'..."
	# PERBAIKAN: Menambahkan ext2.c ke kompilasi inserter
	@$(CC) -Wno-builtin-declaration-mismatch -g -I$(SOURCE_FOLDER) \
		$(SOURCE_FOLDER)/stdlib/string.c \
		$(SOURCE_FOLDER)/filesystem/ext2.c \
		$(SOURCE_FOLDER)/external/external-inserter.c \
		-o $(OUTPUT_FOLDER)/inserter

user-shell:
	@echo "Compiling user program 'shell'..."
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/crt0.s -o crt0.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/user-shell.c -o user-shell.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/stdlib/string.c -o string.o
	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 --oformat=binary \
		crt0.o user-shell.o string.o -o $(OUTPUT_FOLDER)/shell
	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 --oformat=elf32-i386 \
		crt0.o user-shell.o string.o -o $(OUTPUT_FOLDER)/shell_elf

	@echo Linking object shell object files and generate flat binary...
	@size --target=binary $(OUTPUT_FOLDER)/shell
	@rm -f *.o

insert-shell: inserter user-shell
	@echo Inserting shell into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter shell 2 $(DISK_NAME).bin

clean:
	@echo Cleaning up...
	@rm -rf $(OUTPUT_FOLDER)/*