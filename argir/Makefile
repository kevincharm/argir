CONFIG_DIR=./config

MAIN_SRC_DIR=./src
MAIN_SRC_FILES=$(wildcard $(MAIN_SRC_DIR)/*.c)
MAIN_OBJ_FILES=$(patsubst %.c,%.o,$(MAIN_SRC_FILES))

LIB_SRC_DIR=$(MAIN_SRC_DIR)/lib
LIB_SRC_FILES=$(wildcard $(LIB_SRC_DIR)/*.c)
LIB_OBJ_FILES=$(patsubst %.c,%.o,$(LIB_SRC_FILES))

ISO_DIR=./iso

AS=i386-elf-as
CC=i386-elf-gcc

TARGET=argir

GIT_COMMIT=$(shell git log -1 --pretty=format:"%H")
KERNEL_DEFINES=-D__ARGIR_BUILD_COMMIT__=\"$(GIT_COMMIT)\"

# Kernel bootstrap & entry
default: clean all

all: $(TARGET)

$(TARGET): $(LIB_OBJ_FILES) $(MAIN_OBJ_FILES) $(MAIN_SRC_DIR)/boot.o
	$(CC) -T kernel.ld \
	-o $(TARGET).bin -ffreestanding -nostdlib -lgcc \
	$(LIB_OBJ_FILES) \
	$(MAIN_SRC_DIR)/kernel.o \
	$(MAIN_SRC_DIR)/boot.o

$(MAIN_SRC_DIR)/boot.o: $(MAIN_SRC_DIR)/boot.s
	$(AS) $(MAIN_SRC_DIR)/boot.s -o $(MAIN_SRC_DIR)/boot.o

$(MAIN_SRC_DIR)/kernel.o: $(MAIN_SRC_DIR)/kernel.c
	$(CC) -c $(MAIN_SRC_DIR)/kernel.c \
	-o $(MAIN_SRC_DIR)/kernel.o -I$(LIB_SRC_DIR) \
	-std=gnu99 -ffreestanding -Wall -Wextra \
	$(KERNEL_DEFINES)

$(LIB_SRC_DIR)/%.o: $(LIB_SRC_DIR)/%.c
	$(CC) -std=gnu99 -ffreestanding -Wall -Wextra -c $< -o $@

# Disk image & Qemu
grubiso: $(TARGET)
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	mv $(TARGET).bin $(ISO_DIR)/boot/$(TARGET).bin
	cp $(CONFIG_DIR)/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(TARGET).iso iso

run: grubiso
	qemu-system-i386 -cdrom $(TARGET).iso

.PHONY: clean

clean:
	rm -f *.bin
	rm -f *.iso
	rm -rf $(ISO_DIR)
	rm -f $(MAIN_SRC_DIR)/*.o
