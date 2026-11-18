#include "kernel.h"
#include "system/memory.h"
#include "system/detect.h"
#include "fb.h"

unsigned char* fb_addr = 0;
unsigned int fb_pitch  = 0;
unsigned int fb_width  = 0;
unsigned int fb_height = 0;
unsigned int fb_bpp    = 0;

unsigned int cols = 0;
unsigned int rows = 0;

unsigned int cursor_index = 0;
unsigned int vga_text_offset = 0;

unsigned int current_fg = 0x00FFFFFF; // White text
unsigned int current_bg = 0x00000000; // Black background
unsigned char current_vga_attr = 0x0F; // Black bg (0), White fg (F)


volatile uint16_t *const vga = (volatile uint16_t *)0xB8000;
uint32_t cursor_pos = 0; 
char CPUType[64] = "NULL"; 
uint64_t TakenRAM = 0;
uint64_t TotalRAM = 0;

static void sync_cursor_pos(void) {
    if (!fb_addr) {
        cursor_pos = vga_text_offset * 2u;
    } else {
        cursor_pos = cursor_index * 2u;
    }
}

void newline_32(void) {
    print_char('\n');
    sync_cursor_pos();
}

void put_char(char ch, uint8_t color) {
    if (ch == '\n') {
        newline_32();
        return;
    }
    current_fg = color_to_rgb(color & 0x0F);
    current_vga_attr = (current_vga_attr & 0xF0) | (color & 0x0F);
    print_char(ch);
    sync_cursor_pos();
}

void printcol(const char *str, uint8_t color) {
    while (*str != '\0') {
        put_char(*str++, color);
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

void printcolvar(uint64_t data, uint8_t mode, uint8_t color) {
    if (mode == 0) {
        const char *str_ptr = (const char*)(uint32_t)data;
        while (*str_ptr != '\0') {
            put_char(*str_ptr, color);
            str_ptr++;
        }
    } 
    else if (mode == 2) {
        put_char((char)(uint32_t)data, color);
    } 
    else {
        print_uint32_helper((uint32_t)data, color);
    }
}

void print(const char *str) {
    printcol(str, VGA_COLOR_WHITE);
}
