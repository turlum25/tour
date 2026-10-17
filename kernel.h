#include "kernel.h"
#include "system/memory.h"
#include "system/detect.h"

volatile uint16_t *const vga = (volatile uint16_t *)0xB8000;
uint32_t cursor_pos = 0; 
char CPUType[64] = "NULL"; 
uint64_t TakenRAM = 0;
uint64_t TotalRAM = 0;

 

static void scroll_if_needed(void) {
    if (cursor_pos < VGA_CELLS * 2u) return;

    uint32_t row;
    for (row = 0; row < VGA_CELLS - VGA_WIDTH; ++row) {
        vga[row] = vga[row + VGA_WIDTH];
    }
    for (; row < VGA_CELLS; ++row) {
        vga[row] = (uint16_t)((VGA_COLOR_WHITE << 8) | ' ');
    }
    cursor_pos = (VGA_CELLS - VGA_WIDTH) * 2u;
}

void newline_32(void) {
    cursor_pos = ((cursor_pos / 2u / VGA_WIDTH) + 1u) * VGA_WIDTH * 2u;
    scroll_if_needed();
}

void put_char(char ch, uint8_t color) {
    if (ch == '\n') {
        newline_32();
        return;
    }
    scroll_if_needed();
    vga[cursor_pos / 2u] = (uint16_t)(((uint16_t)color << 8) | (uint8_t)ch);
    cursor_pos += 2u;
}

void printcol(const char *str, uint8_t color, ...) {
    if (str == NULL) return;

    uint32_t *args = (uint32_t *)&color + 1;
    int arg_idx = 0;

    while (*str != '\0') {
        if (*str == '%' && *(str + 1) == 's') {
            const char *str_var = (const char *)args[arg_idx++];
            if (str_var != NULL) {
                while (*str_var != '\0') {
                    put_char(*str_var++, color);
                }
            }
            str += 2;
        } 
      
        else if (*str == '\n') {
            newline_32();
            str++;
        }
        else {
            
            put_char(*str++, color);
        }
    }
}


static void print_uint32_helper(uint32_t num, uint8_t color) {
    char num_buf[32]; 
    int idx = 0;
    
    if (num == 0) {
        num_buf[idx++] = '0';
    } else {
        while (num > 0) {
            num_buf[idx++] = (char)('0' + (num % 10));
            num /= 10;                                
        }
    }
    
    int start = 0;
    int end = idx - 1;
    while (start < end) {
        char temp = num_buf[start];
        num_buf[start] = num_buf[end];
        num_buf[end] = temp;
        start++;
        end--;
    }
    
    for (int i = 0; i < idx; i++) {
        put_char(num_buf[i], color);
    }
}

void printcolvar(uint32_t data, uint8_t mode, uint8_t color) {
    if (mode == 0) {
        const char *str_ptr = (const char*)data;
        while (*str_ptr != '\0') {
            put_char(*str_ptr, color);
            str_ptr++;
        }
    } 
    else if (mode == 2) {
        put_char((char)data, color);
    } 
    else {
        print_uint32_helper(data, color);
    }
}

void print_pure_string(const char *str, uint8_t color) {
    if (str == NULL) return;
    while (*str != '\0') {
        put_char(*str, color);
        str++;
    }
}
