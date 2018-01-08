CONFIG_DIR=./config
ISO_DIR=./iso

# Compilers
AS=i386-elf-as
CC=i386-elf-gcc
CFLAGS=-std=gnu11 -ffreestanding -Wall -Wextra

# Build info
GIT_COMMIT=$(shell git log -1 --pretty=format:"%H")
KERNEL_DEFINES=-D__ARGIR_BUILD_COMMIT__=\"$(GIT_COMMIT)\"

# Sources
SRC_DIR=./src
KERNEL_OBJS=\
	$(SRC_DIR)/boot.o \
	$(SRC_DIR)/kernel.o

LIB_SRC_DIR=$(SRC_DIR)/kernel
LIB_SRC_FILES=$(wildcard $(LIB_SRC_DIR)/*.c)
LIB_OBJS=$(patsubst %.c,%.o,$(LIB_SRC_FILES))

KLIB_DIR=$(SRC_DIR)/klib
KLIB_INCLUDE=$(KLIB_DIR)/include
KLIB_OBJS=\
	$(KLIB_DIR)/stdio/putchar.o \
	$(KLIB_DIR)/string/strlen.o

default: clean all

all: argir

argir: $(KLIB_OBJS) $(LIB_OBJS) $(KERNEL_OBJS) $(SRC_DIR)/boot.o
	$(CC) -T kernel.ld \
	-o argir.bin -ffreestanding -nostdlib -lgcc \
	$(KLIB_OBJS) \
	$(LIB_OBJS) \
	$(SRC_DIR)/kernel.o \
	$(SRC_DIR)/boot.o

$(SRC_DIR)/boot.o: $(SRC_DIR)/boot.s
	$(AS) $< -o $@

$(SRC_DIR)/kernel.o: $(SRC_DIR)/kernel.c
	$(CC) $(CFLAGS) -c $< -o $@ \
	-I$(LIB_SRC_DIR) \
	$(KERNEL_DEFINES)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(KLIB_INCLUDE)

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
