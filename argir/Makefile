CONFIG_DIR=./config

SRC_DIR=./src
MAIN_SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
MAIN_OBJS=$(patsubst %.c,%.o,$(MAIN_SRC_FILES))

ISO_DIR=./iso

AS=i386-elf-as
CC=i386-elf-gcc

GIT_COMMIT=$(shell git log -1 --pretty=format:"%H")
KERNEL_DEFINES=-D__ARGIR_BUILD_COMMIT__=\"$(GIT_COMMIT)\"

LIB_SRC_DIR=$(SRC_DIR)/kernel
LIB_SRC_FILES=$(wildcard $(LIB_SRC_DIR)/*.c)
LIB_OBJS=$(patsubst %.c,%.o,$(LIB_SRC_FILES))

KLIB_DIR=$(SRC_DIR)/klib
KLIB_INCLUDE=$(KLIB_DIR)/include
KLIB_OBJS=\
	$(KLIB_DIR)/stdio/putchar.o \
	$(KLIB_DIR)/string/strlen.o

# Kernel bootstrap & entry
default: clean all

all: argir

argir: $(KLIB_OBJS) $(LIB_OBJS) $(MAIN_OBJS) $(SRC_DIR)/boot.o
	$(CC) -T kernel.ld \
	-o argir.bin -ffreestanding -nostdlib -lgcc \
	$(KLIB_OBJS) \
	$(LIB_OBJS) \
	$(SRC_DIR)/kernel.o \
	$(SRC_DIR)/boot.o

$(SRC_DIR)/boot.o: $(SRC_DIR)/boot.s
	$(AS) $(SRC_DIR)/boot.s -o $(SRC_DIR)/boot.o

$(SRC_DIR)/kernel.o: $(SRC_DIR)/kernel.c
	$(CC) -c $(SRC_DIR)/kernel.c \
	-o $(SRC_DIR)/kernel.o -I$(LIB_SRC_DIR) \
	-std=gnu99 -ffreestanding -Wall -Wextra \
	$(KERNEL_DEFINES)

$(LIB_SRC_DIR)/%.o: $(LIB_SRC_DIR)/%.c
	$(CC) -std=gnu99 -ffreestanding -Wall -Wextra -c $< -o $@ -I$(KLIB_INCLUDE)

%.o: %.c
	$(CC) -std=gnu99 -ffreestanding -Wall -Wextra -c $< -o $@ -I$(KLIB_INCLUDE)

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
	rm -f $(SRC_DIR)/*.o
