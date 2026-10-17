#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH        80u
#define VGA_HEIGHT       25u
#define VGA_CELLS        (VGA_WIDTH * VGA_HEIGHT)
#define VGA_COLOR_WHITE  0x0Fu
#define VGA_COLOR_RED    0x0Cu
#define VGA_COLOR_GREEN  0x0Au
#define VGA_COLOR_YELLOW 0x0Eu

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

void printcol(const char *str, uint8_t color, ...);
void put_char(char ch, uint8_t color);
int strings_match(const char *s1, const char *s2);
int StringPrefixMatch(const char *s, const char *prefix);
void printcolvar(uint32_t data, uint8_t mode, uint8_t color);
//void DetectCPU();
//void checkprintram();
extern char CPUType[];
extern uint64_t TakenRAM;
extern uint64_t TotalRAM;


#endif /* KERNEL_H */
