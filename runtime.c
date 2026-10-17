#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "kernel.h"

#define HEAP_BYTES (128u * 1024u)
#define HEAP_MAGIC 0x48454150u
#define HEAP_ALIGNMENT 16u

typedef struct heap_block {
    uint32_t magic;
    uint32_t is_free;
    size_t size;
    struct heap_block *next;
} heap_block_t;

static uint8_t runtime_heap[HEAP_BYTES] __attribute__((aligned(HEAP_ALIGNMENT)));
static char *strtok_cursor;

void *memcpy(void *destination, const void *source, size_t count) {
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    for (size_t i = 0; i < count; ++i) out[i] = in[i];
    return destination;
}

void *memset(void *destination, int value, size_t count) {
    uint8_t *out = (uint8_t *)destination;
    for (size_t i = 0; i < count; ++i) out[i] = (uint8_t)value;
    return destination;
}

size_t strlen(const char *text) {
    size_t length = 0;
    while (text[length] != '\0') ++length;
    return length;
}

int strcmp(const char *left, const char *right) {
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return (uint8_t)*left - (uint8_t)*right;
}

char *strncpy(char *destination, const char *source, size_t count) {
    size_t i = 0;
    while (i < count && source[i] != '\0') {
        destination[i] = source[i];
        ++i;
    }
    while (i < count) destination[i++] = '\0';
    return destination;
}

char *strtok(char *text, const char *delimiters) {
    if (text != NULL) strtok_cursor = text;
    if (strtok_cursor == NULL) return NULL;

    while (*strtok_cursor != '\0') {
        int is_delimiter = 0;
        for (const char *delimiter = delimiters; *delimiter != '\0'; ++delimiter) {
            if (*strtok_cursor == *delimiter) {
                is_delimiter = 1;
                break;
            }
        }
        if (!is_delimiter) break;
        ++strtok_cursor;
    }

    if (*strtok_cursor == '\0') {
        strtok_cursor = NULL;
        return NULL;
    }

    char *token = strtok_cursor;
    while (*strtok_cursor != '\0') {
        for (const char *delimiter = delimiters; *delimiter != '\0'; ++delimiter) {
            if (*strtok_cursor == *delimiter) {
                *strtok_cursor++ = '\0';
                return token;
            }
        }
        ++strtok_cursor;
    }
    strtok_cursor = NULL;
    return token;
}

static heap_block_t *heap_initialize(void) {
    heap_block_t *first = (heap_block_t *)runtime_heap;
    if (first->magic != HEAP_MAGIC || first->size > HEAP_BYTES - sizeof(*first)) {
        first->magic = HEAP_MAGIC;
        first->is_free = 1;
        first->size = HEAP_BYTES - sizeof(*first);
        first->next = NULL;
    }
    return first;
}

void *malloc(size_t size) {
    if (size == 0 || size > HEAP_BYTES - sizeof(heap_block_t)) return NULL;
    if (size > SIZE_MAX - (HEAP_ALIGNMENT - 1u)) return NULL;
    size = (size + HEAP_ALIGNMENT - 1u) & ~(HEAP_ALIGNMENT - 1u);

    heap_block_t *block = heap_initialize();
    while (block != NULL) {
        if (block->magic != HEAP_MAGIC) return NULL;
        if (block->is_free && block->size >= size) {
            if (block->size >= size + sizeof(*block) + HEAP_ALIGNMENT) {
                heap_block_t *remainder = (heap_block_t *)((uint8_t *)(block + 1) + size);
                remainder->magic = HEAP_MAGIC;
                remainder->is_free = 1;
                remainder->size = block->size - size - sizeof(*block);
                remainder->next = block->next;
                block->next = remainder;
                block->size = size;
            }
            block->is_free = 0;
            return block + 1;
        }
        block = block->next;
    }
    return NULL;
}

void free(void *pointer) {
    if (pointer == NULL) return;
    uintptr_t address = (uintptr_t)pointer;
    uintptr_t heap_start = (uintptr_t)runtime_heap;
    uintptr_t heap_end = heap_start + HEAP_BYTES;
    if (address < heap_start + sizeof(heap_block_t) || address >= heap_end ||
        (address & (HEAP_ALIGNMENT - 1u)) != 0) {
        return;
    }

    heap_block_t *block = (heap_block_t *)pointer - 1;
    if (block->magic != HEAP_MAGIC || block->is_free) return;
    block->is_free = 1;

    heap_block_t *current = heap_initialize();
    while (current != NULL && current->next != NULL) {
        heap_block_t *next = current->next;
        if (current->is_free && next->is_free &&
            (uint8_t *)(current + 1) + current->size == (uint8_t *)next) {
            current->size += sizeof(*next) + next->size;
            current->next = next->next;
        } else {
            current = next;
        }
    }
}

uint32_t getuid(void) { return 0; }
uint32_t getgid(void) { return 0; }

static uint64_t divide_u64(uint64_t numerator, uint64_t denominator, uint64_t *remainder) {
    if (denominator == 0) {
        if (remainder != NULL) *remainder = numerator;
        return UINT64_MAX;
    }

    uint64_t quotient = 0;
    uint64_t partial = 0;
    for (int bit = 63; bit >= 0; --bit) {
        uint64_t carry = partial >> 63;
        partial = (partial << 1) | ((numerator >> bit) & 1u);
        if (carry || partial >= denominator) {
            partial -= denominator;
            quotient |= (uint64_t)1 << bit;
        }
    }
    if (remainder != NULL) *remainder = partial;
    return quotient;
}

uint64_t __udivdi3(uint64_t numerator, uint64_t denominator) {
    return divide_u64(numerator, denominator, NULL);
}

uint64_t __umoddi3(uint64_t numerator, uint64_t denominator) {
    uint64_t remainder;
    divide_u64(numerator, denominator, &remainder);
    return remainder;
}

int64_t __divdi3(int64_t numerator, int64_t denominator) {
    int negative = (numerator < 0) != (denominator < 0);
    uint64_t left = numerator < 0 ? 0u - (uint64_t)numerator : (uint64_t)numerator;
    uint64_t right = denominator < 0 ? 0u - (uint64_t)denominator : (uint64_t)denominator;
    uint64_t quotient = divide_u64(left, right, NULL);
    return negative ? (int64_t)(0u - quotient) : (int64_t)quotient;
}

int64_t __moddi3(int64_t numerator, int64_t denominator) {
    uint64_t left = numerator < 0 ? 0u - (uint64_t)numerator : (uint64_t)numerator;
    uint64_t right = denominator < 0 ? 0u - (uint64_t)denominator : (uint64_t)denominator;
    uint64_t remainder;
    divide_u64(left, right, &remainder);
    return numerator < 0 ? (int64_t)(0u - remainder) : (int64_t)remainder;
}

static int emit_unsigned(uint64_t value, uint32_t base, int uppercase, int width, char padding) {
    char digits[32];
    int length = 0;
    int emitted = 0;
    const char *alphabet = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    do {
        digits[length++] = alphabet[(uint32_t)(value % base)];
        value /= base;
    } while (value != 0);
    while (width > length) {
        put_char(padding, VGA_COLOR_WHITE);
        ++emitted;
        --width;
    }
    while (length != 0) {
        put_char(digits[--length], VGA_COLOR_WHITE);
        ++emitted;
    }
    return emitted;
}

int printf(const char *format, ...) {
    va_list arguments;
    int emitted = 0;
    va_start(arguments, format);

    while (*format != '\0') {
        if (*format != '%') {
            put_char(*format++, VGA_COLOR_WHITE);
            ++emitted;
            continue;
        }
        ++format;
        if (*format == '%') {
            put_char(*format++, VGA_COLOR_WHITE);
            ++emitted;
            continue;
        }

        char padding = ' ';
        if (*format == '0') {
            padding = '0';
            ++format;
        }
        int width = 0;
        while (*format >= '0' && *format <= '9') {
            if (width < 100) width = width * 10 + (*format - '0');
            ++format;
        }

        int long_count = 0;
        int size_modifier = 0;
        while (*format == 'l') {
            ++long_count;
            ++format;
        }
        if (*format == 'z' || *format == 't' || *format == 'j') {
            size_modifier = 1;
            ++format;
        } else if (*format == 'h') {
            ++format;
            if (*format == 'h') ++format;
        }

        char conversion = *format;
        if (conversion == '\0') break;
        ++format;

        if (conversion == 's') {
            const char *text = va_arg(arguments, const char *);
            if (text == NULL) text = "(null)";
            while (*text != '\0') {
                put_char(*text++, VGA_COLOR_WHITE);
                ++emitted;
            }
        } else if (conversion == 'c') {
            put_char((char)va_arg(arguments, int), VGA_COLOR_WHITE);
            ++emitted;
        } else if (conversion == 'd' || conversion == 'i') {
            int64_t signed_value = long_count > 1
                ? va_arg(arguments, int64_t) : va_arg(arguments, int);
            uint64_t value;
            if (signed_value < 0) {
                put_char('-', VGA_COLOR_WHITE);
                ++emitted;
                value = 0u - (uint64_t)signed_value;
            } else {
                value = (uint64_t)signed_value;
            }
            emitted += emit_unsigned(value, 10, 0, width, padding);
        } else if (conversion == 'u' || conversion == 'x' || conversion == 'X' ||
                   conversion == 'o' || conversion == 'p') {
            uint64_t value;
            if (conversion == 'p') {
                value = (uintptr_t)va_arg(arguments, void *);
            } else if (long_count > 1) {
                value = va_arg(arguments, uint64_t);
            } else if (size_modifier) {
                value = va_arg(arguments, size_t);
            } else {
                value = va_arg(arguments, unsigned int);
            }
            uint32_t base = conversion == 'x' || conversion == 'X' || conversion == 'p'
                ? 16u : conversion == 'o' ? 8u : 10u;
            emitted += emit_unsigned(value, base, conversion == 'X', width, padding);
        } else {
            put_char('%', VGA_COLOR_WHITE);
            put_char(conversion, VGA_COLOR_WHITE);
            emitted += 2;
        }
    }

    va_end(arguments);
    return emitted;
}