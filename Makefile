OBJ_DIR = objects
OBJ = $(OBJ_DIR)/main.o $(OBJ_DIR)/vga.o $(OBJ_DIR)/string.o \
	$(OBJ_DIR)/runtime.o $(OBJ_DIR)/multiboot.o $(OBJ_DIR)/multiboot_c.o

QEMU = qemu-system-i386
CFLAGS = -m32 -ffreestanding -O0 -fno-pic -fno-pie -fno-stack-protector -I. -Idisk/fs/ext2/src -c

.DEFAULT_GOAL := all
.PHONY: all legacy-iso grub clean makedisk rundisk run

all: legacy-iso

legacy-iso: tour.img
	mkdir -p iso_legacy
	cp tour.img iso_legacy/tour.img
	xorriso -as mkisofs -o tour.iso -volid TOUR \
		-b tour.img -c boot.catalog -boot-load-size 4 iso_legacy
	rm -rf iso_legacy

grub: tour.img kernel.elf boot/grub/grub.cfg
	mkdir -p iso_grub/boot/grub
	cp tour.img iso_grub/
	cp kernel.elf iso_grub/kernel.elf
	cp boot/grub/grub.cfg iso_grub/boot/grub/grub.cfg
	grub-mkrescue -volid "TOUR (GRUB)" -o tour.iso iso_grub \
		-eltorito-alt-boot -eltorito-platform x86 \
		-b /tour.img -boot-load-size 4
	rm -rf iso_grub

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/main.o: main.c disk/disk.h disk/sata.h | $(OBJ_DIR)
	gcc $(CFLAGS) main.c -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/vga.o: vga.c | $(OBJ_DIR)
	gcc $(CFLAGS) vga.c -o $(OBJ_DIR)/vga.o

$(OBJ_DIR)/string.o: string.c | $(OBJ_DIR)
	gcc $(CFLAGS) string.c -o $(OBJ_DIR)/string.o

$(OBJ_DIR)/runtime.o: runtime.c | $(OBJ_DIR)
	gcc $(CFLAGS) -fno-builtin runtime.c -o $(OBJ_DIR)/runtime.o

$(OBJ_DIR)/multiboot.o: multiboot.asm | $(OBJ_DIR)
	nasm -f elf32 multiboot.asm -o $(OBJ_DIR)/multiboot.o

$(OBJ_DIR)/multiboot_c.o: multiboot.c | $(OBJ_DIR)
	gcc $(CFLAGS) -fno-builtin multiboot.c -o $(OBJ_DIR)/multiboot_c.o

$(OBJ_DIR)/bios.o: disk/bios.asm | $(OBJ_DIR) # Forgot why this is needed but i'll keep it
	nasm -f elf32 disk/bios.asm -o $(OBJ_DIR)/bios.o

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel.elf: $(OBJ) linker.ld
	ld -m elf_i386 -T linker.ld --build-id=none $(OBJ) -o kernel.elf

main.bin: kernel.elf
	objcopy -O binary kernel.elf main.bin

tour.img: boot.bin main.bin
	cat boot.bin main.bin > tour.img
	truncate -s 1474560 tour.img

clean:
	rm -rf $(OBJ_DIR) *.bin *.elf *.img tour.iso iso_legacy/ iso_grub/

makedisk:
	dd if=/dev/zero of=disk.img bs=1M count=512 && sfdisk disk.img <<< "label: dos" && sfdisk disk.img <<< ",,L,*" && mkfs.ext2 -F -E offset=1048576 disk.img

rundisk: grub
	# Note: Please run after using make makedisk.
	$(QEMU) -cpu 486 -m 2096K -cdrom tour.iso -device ahci,id=ahci -drive format=raw,file=disk.img,if=none,id=sata_disk -device ide-hd,bus=ahci.0,drive=sata_disk -boot d

run: grub
	# Running without disk... Depressing
	$(QEMU) -cpu 486 -m 2096K -cdrom tour.iso
