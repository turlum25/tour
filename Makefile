ASM = nasm
QEMU = qemu-system-i386
CC = gcc
CFLAGS = -m32 -march=i386 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -I. -c
.PHONY: all run clean

all: asmOS.img

boot.bin: boot.asm
	$(ASM) -f bin boot.asm -o boot.bin

# Compile modules individually
#system.o: system.c 
#	$(CC) $(CFLAGS) system.c -o system.o
	
vga.o: vga.c kernel.h
	$(CC) $(CFLAGS) vga.c -o vga.o

string.o: string.c kernel.h
	$(CC) $(CFLAGS) string.c -o string.o

main.o: main.c kernel.h
	$(CC) $(CFLAGS) main.c -o main.o

main.bin: main.o vga.o string.o linker.ld
	ld -m elf_i386 -T linker.ld --oformat binary main.o vga.o string.o -o main.bin

asmOS.img: boot.bin main.bin
	cat boot.bin main.bin > tour.img
	truncate -s 512000 tour.img

run: asmOS.img
	$(QEMU) -drive format=raw,file=tour.img

clean:
	rm -f *.o *.bin tour.img
