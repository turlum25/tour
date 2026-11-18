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

void printcol(const char *str, uint8_t color);
void put_char(char ch, uint8_t color);
int strings_match(const char *s1, const char *s2);
int StringPrefixMatch(const char *s, const char *prefix);
void printcolvar(uint64_t data, uint8_t mode, uint8_t color);
//void DetectCPU();
//void checkprintram();
extern char CPUType[];
extern uint64_t TakenRAM;
extern uint64_t TotalRAM;
void print(const char *str);

void *memcpy(void *destination, const void *source, size_t count);
void *memset(void *destination, int value, size_t count);
void *memmove(void *dst, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *text);
int strcmp(const char *left, const char *right);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *destination, const char *source, size_t count);
char *strtok(char *text, const char *delimiters);

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *pointer);

uint32_t getuid(void);
uint32_t getgid(void);
int printf(const char *format, ...); // How the fuck is this here i do NOT remember making this



#endif /* KERNEL_H */
