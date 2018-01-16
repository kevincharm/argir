CONFIG_DIR=./config
ISO_DIR=./iso

# Compilers
AS=i386-elf-as
CC=i386-elf-gcc
CFLAGS=-std=gnu11 -ffreestanding -nostdlib -Wall -Wextra

# Build info
GIT_COMMIT=$(shell git log -1 --pretty=format:"%H")
KERNEL_DEFINES=__ARGIR_BUILD_COMMIT__=\"$(GIT_COMMIT)\"

# Sources
SRC_DIR=./src
KERNEL_INCLUDE=$(SRC_DIR)/include
KERNEL_OBJS=\
	$(SRC_DIR)/kernel/irq.o \
	$(SRC_DIR)/kernel/keyboard.o \
	$(SRC_DIR)/kernel/terminal.o \
	$(SRC_DIR)/boot.o \
	$(SRC_DIR)/kernel.o

KLIB_DIR=$(SRC_DIR)/klib
KLIB_INCLUDE=$(KLIB_DIR)/include
KLIB_OBJS=\
	$(KLIB_DIR)/stdio/putchar.o \
	$(KLIB_DIR)/stdio/printf.o \
	$(KLIB_DIR)/string/strlen.o

default: clean all

all: argir

argir: $(KLIB_OBJS) $(KERNEL_OBJS) $(SRC_DIR)/boot.o
	$(CC) -T kernel.ld \
	-o argir.bin -ffreestanding -nostdlib -lgcc \
	$(KLIB_OBJS) \
	$(KERNEL_OBJS)

boot.o: boot.s
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

run: grubiso
	qemu-system-i386 -cdrom argir.iso

.PHONY: clean

clean:
	rm -f *.bin
	rm -f *.iso
	rm -rf $(ISO_DIR)
	find $(SRC_DIR) -type f -name '*.o' -delete
