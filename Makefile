CONFIG_DIR=./config
ISO_DIR=./iso

# Docker
DOCKER_IMAGE=kevincharm/x86_64-elf-gcc-toolchain:latest
DOCKER_SH=docker run -it --rm \
	-v `pwd`:/work \
	-w /work \
	--security-opt seccomp=unconfined \
	$(DOCKER_IMAGE) /bin/bash -c

# Compilers
AS=x86_64-elf-as
CC=x86_64-elf-gcc
LD=x86_64-elf-ld
CFLAGS=-m64 -std=gnu11 -ffreestanding -O2 -nostdlib -Wall -Wextra -mcmodel=large -mno-red-zone \
	-mgeneral-regs-only -mno-mmx -mno-sse -mno-sse2 -mno-avx

# Build info
GIT_COMMIT=$(shell git log -1 --pretty=format:"%H")
KERNEL_DEFINES=__ARGIR_BUILD_COMMIT__=\"$(GIT_COMMIT)\"

# Sources
SRC_DIR=./src
KERNEL_INCLUDE=$(SRC_DIR)/include
KERNEL_OBJS=\
	$(SRC_DIR)/boot.o \
	$(SRC_DIR)/kernel/mb2.o \
	$(SRC_DIR)/kernel/font_vga.o \
	$(SRC_DIR)/kernel/gdt.o \
	$(SRC_DIR)/kernel/pic.o \
	$(SRC_DIR)/kernel/idt.o \
	$(SRC_DIR)/kernel/interrupts.o \
	$(SRC_DIR)/kernel/isr.o \
	$(SRC_DIR)/kernel/keyboard.o \
	$(SRC_DIR)/kernel/terminal.o \
	$(SRC_DIR)/kernel/pci.o \
	$(SRC_DIR)/kernel.o

KLIB_DIR=$(SRC_DIR)/klib
KLIB_INCLUDE=$(KLIB_DIR)/include
KLIB_OBJS=\
	$(KLIB_DIR)/memory/memset.o \
	$(KLIB_DIR)/ringbuf/ringbuf.o \
	$(KLIB_DIR)/stdio/putchar.o \
	$(KLIB_DIR)/stdio/printf.o \
	$(KLIB_DIR)/string/strlen.o

default: clean all

.PHONY: clean

all:
	$(DOCKER_SH) "make _all"

_all: argir.iso

argir.bin: $(KERNEL_OBJS) $(KLIB_OBJS)
	$(CC) -z max-page-size=0x1000 $(CFLAGS) -T kernel.ld -lgcc -o $@ $(KERNEL_OBJS) $(KLIB_OBJS)

%.o: %.s
	$(AS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ \
	-I$(KLIB_INCLUDE) -I$(KERNEL_INCLUDE) -D$(KERNEL_DEFINES)

# Disk image & Qemu
argir.iso: argir.bin
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	mv argir.bin $(ISO_DIR)/boot/argir.bin
	cp $(CONFIG_DIR)/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o argir.iso iso

QEMU=qemu-system-x86_64 -cdrom argir.iso -netdev user,id=eth0 -device ne2k_pci,netdev=eth0 -monitor stdio -d int,cpu_reset -no-reboot

run: all
	$(QEMU)

debug: all
	$(QEMU) -d int,cpu_reset

clean:
	rm -f *.bin
	rm -f *.iso
	rm -rf $(ISO_DIR)
	find $(SRC_DIR) -type f -name '*.o' -delete

print_toolchain:
	$(DOCKER_SH) "make _print_toolchain"
_print_toolchain:
	$(CC) --version

sections:
	greadelf -S $(ISO_DIR)/boot/argir.bin

objdump:
	objdump -x $(ISO_DIR)/boot/argir.bin
