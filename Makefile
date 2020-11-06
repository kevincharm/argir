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
CFLAGS=-fPIC -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args

EFI_INCLUDES=-I/usr/include/efi -I/usr/include/efi/protocol -I/usr/include/efi/x86_64

# Build info
GIT_COMMIT=$(shell git log -1 --pretty=format:"%H")
KERNEL_DEFINES=__ARGIR_BUILD_COMMIT__=\"$(GIT_COMMIT)\"

# Sources
SRC_DIR=./src
KERNEL_INCLUDE=$(SRC_DIR)/include
KERNEL_OBJS=\
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
	$(KLIB_DIR)/ringbuf/ringbuf.o \
	$(KLIB_DIR)/stdio/putchar.o \
	$(KLIB_DIR)/stdio/printf.o \
	$(KLIB_DIR)/string/strlen.o

default: clean all

.PHONY: clean

all:
	$(DOCKER_SH) "make _all"
	make argir.img

_all: $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI

$(ISO_DIR)/EFI/BOOT/BOOTX64.EFI: $(KERNEL_OBJS) $(KLIB_OBJS)
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/EFI/BOOT
	$(LD) -z max-page-size=0x1000 -shared -Bsymbolic -L/usr/lib -Tkernel.ld -o $@.so /usr/lib/crt0-efi-x86_64.o $(KERNEL_OBJS) $(KLIB_OBJS) -lgnuefi -lefi
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc \
		--target efi-app-x86_64 --subsystem=10 $@.so $@
	rm $@.so

%.o: %.s
	$(AS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ \
	$(EFI_INCLUDES) -I$(KLIB_INCLUDE) -I$(KERNEL_INCLUDE) -D$(KERNEL_DEFINES)

# Disk image & Qemu
argir.img: $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI
	hdiutil create -fs fat32 -ov -size 48m -volname ARGIROS -format UDTO -srcfolder $(ISO_DIR) $@

QEMU=qemu-system-x86_64 -cpu qemu64 -bios OVMF.fd -drive file=argir.img.cdr,if=ide \
	-netdev user,id=eth0 -device ne2k_pci,netdev=eth0 -monitor stdio -d cpu_reset -D ./tmp/qemu.log

run: all
	$(QEMU)

debug: all
	$(QEMU) -d int,cpu_reset

clean:
	rm -f *.efi *.so *.cdr
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
