CONFIG_DIR=./config
ISO_DIR=./iso

# Docker
DOCKER_IMAGE=kevincharm/i686-elf-gcc-toolchain:5.5.0
DOCKER_SH=docker run -it --rm \
	-v `pwd`:/work \
	-w /work \
	--security-opt seccomp=unconfined \
	$(DOCKER_IMAGE) /bin/bash -c

# Compilers
AS=i686-elf-as
CC=i686-elf-gcc
CFLAGS=-std=gnu11 -ffreestanding -nostdlib -Wall -Wextra

# Build info
GIT_COMMIT=$(shell git log -1 --pretty=format:"%H")
KERNEL_DEFINES=__ARGIR_BUILD_COMMIT__=\"$(GIT_COMMIT)\"

# Sources
SRC_DIR=./src
KERNEL_INCLUDE=$(SRC_DIR)/include
KERNEL_OBJS=\
	$(SRC_DIR)/kernel/gdt.o \
	$(SRC_DIR)/kernel/gdt_rst.o \
	$(SRC_DIR)/kernel/pic.o \
	$(SRC_DIR)/kernel/idt.o \
	$(SRC_DIR)/kernel/interrupts.o \
	$(SRC_DIR)/kernel/isr.o \
	$(SRC_DIR)/kernel/keyboard.o \
	$(SRC_DIR)/kernel/terminal.o \
	$(SRC_DIR)/kernel/pci.o \
	$(SRC_DIR)/boot.o \
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

_all: argir

argir: $(KLIB_OBJS) $(KERNEL_OBJS) $(SRC_DIR)/boot.o
	$(CC) -T kernel.ld \
	-o argir.bin -ffreestanding -nostdlib -lgcc \
	$(KLIB_OBJS) \
	$(KERNEL_OBJS)

%.o: %.s
	$(AS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ \
	-I$(KLIB_INCLUDE) -I$(KERNEL_INCLUDE) \
	-D$(KERNEL_DEFINES)

# Disk image & Qemu
grubiso: argir
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	mv argir.bin $(ISO_DIR)/boot/argir.bin
	cp $(CONFIG_DIR)/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o argir.iso iso

QEMU=qemu-system-i386 -cdrom argir.iso -no-reboot -netdev user,id=eth0 -device ne2k_pci,netdev=eth0

run: grubiso
	$(QEMU)

debug: grubiso
	$(QEMU) -d int,cpu_reset

clean:
	rm -f *.bin
	rm -f *.iso
	rm -rf $(ISO_DIR)
	find $(SRC_DIR) -type f -name '*.o' -delete
