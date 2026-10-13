#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "kernel.h"

static const char scancode_table[64] = {
    0, 0, '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', 0, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', 0, 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0
};

static const char shift_scancode_table[64] = {
    0, 0, '!', '@', '#', '$', '%', '^',
    '&', '*', '(', ')', '_', '+', 0, 0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', 0, 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0
};

static inline void read_keyboard_input_32(char *start, char *end) {
    char *current = start;
    int extended = 0;
    static int shift_pressed = 0; 
    static int caps_locked = 0;   
    volatile uint16_t *const local_vga = (volatile uint16_t *)0xB8000;
    extern uint32_t cursor_pos; 

    if (start == NULL || end == NULL || end <= start) return;
    *start = '\0';

    for (;;) {
        if ((inb(0x64) & 1u) == 0) continue; 
        uint8_t scancode = inb(0x60);         

        if (scancode == 0xE0) { extended = 1; continue; }
        if (extended) { extended = 0; continue; }

        /* --- CATCH SHIFT PRESS EVENTS --- */
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
            continue;
        }

        if (scancode == 0xAA || scancode == 0xB6) {
            shift_pressed = 0;
            continue;
        }

        if (scancode == 0x3A) {
            caps_locked = !caps_locked;
            continue;
        }

        if (scancode & 0x80u) continue; 
        if (scancode == 0x1C) { 
            *current = '\0';
            void newline_32(void); 
            newline_32();
            return;
        }

        if (scancode == 0x0E) { 
            if (current > start) {
                --current;
                *current = '\0';
                if (cursor_pos >= 2u) cursor_pos -= 2u;
                local_vga[cursor_pos / 2u] = (uint16_t)((VGA_COLOR_WHITE << 8) | ' ');
            }
            continue;
        }

        if (scancode < 64 && current < end - 1) {
            char ch = scancode_table[scancode];
            
            int is_alpha = (ch >= 'a' && ch <= 'z');
            
            int use_shift_table = is_alpha ? (shift_pressed ^ caps_locked) : shift_pressed;
            
            char final_ch = use_shift_table ? shift_scancode_table[scancode] : scancode_table[scancode];
            
            if (final_ch != '\0') {
                *current++ = final_ch;
                *current = '\0';
                put_char(final_ch, VGA_COLOR_WHITE);
            }
        }
    }
}

static inline void input(const char *prompt, char *buffer, char *buffer_end) {
    while (*prompt != '\0') {
        put_char(*prompt++, VGA_COLOR_WHITE);
    }
    read_keyboard_input_32(buffer, buffer_end);
}

#define str(str1, str2, output) (output = strings_match(str1, str2))

#endif /* KEYBOARD_H */
