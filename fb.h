#ifndef FB_H
#define FB_H

#include <stdint.h>
#include "font.h"

#define GLYPH_W 8
#define GLYPH_H 16
#define VGA_TEXT_ADDR 0xB8000
#define VGA_COLS 80
#define VGA_ROWS 25

extern unsigned char* fb_addr;
extern unsigned int fb_pitch;
extern unsigned int fb_width;
extern unsigned int fb_height;
extern unsigned int fb_bpp;

extern unsigned int cols;
extern unsigned int rows;

extern unsigned int cursor_index;
extern unsigned int vga_text_offset;

extern unsigned int current_fg;
extern unsigned int current_bg;
extern unsigned char current_vga_attr;

static inline void clear_screen_reset_cursor(void);

static unsigned int color_to_rgb(unsigned char color)
{
    switch (color & 0x0F) {
        case 0:  return 0x00000000;
        case 1:  return 0x000000AA;
        case 2:  return 0x0000AA00;
        case 3:  return 0x0000AAAA;
        case 4:  return 0x00AA0000;
        case 5:  return 0x00AA00AA;
        case 6:  return 0x00AA5500;
        case 7:  return 0x00AAAAAA;
        case 8:  return 0x00555555;
        case 9:  return 0x005555FF;
        case 10: return 0x0055FF55;
        case 11: return 0x0055FFFF;
        case 12: return 0x00FF5555;
        case 13: return 0x00FF55FF;
        case 14: return 0x00FFFF55;
        case 15: return 0x00FFFFFF;
        default: return 0x00FFFFFF;
    }
}

static unsigned char rgb_to_vga_attr(unsigned int rgb)
{
    unsigned char r = (rgb >> 16) & 0xFF;
    unsigned char g = (rgb >> 8) & 0xFF;
    unsigned char b = rgb & 0xFF;
    unsigned char attr = 0;
    if (r > 128) attr |= 4;
    if (g > 128) attr |= 2;
    if (b > 128) attr |= 1;
    if (r > 200 || g > 200 || b > 200) attr |= 8;
    return attr;
}

static void put_pixel(unsigned int x, unsigned int y, unsigned int rgb)
{
    if (!fb_addr || x >= fb_width || y >= fb_height) {
        return;
    }

    unsigned char* p = fb_addr + y * fb_pitch + x * (fb_bpp / 8);

    if (fb_bpp == 32) {
        p[0] = (unsigned char)(rgb & 0xFF);
        p[1] = (unsigned char)((rgb >> 8) & 0xFF);
        p[2] = (unsigned char)((rgb >> 16) & 0xFF);
        p[3] = (unsigned char)((rgb >> 24) & 0xFF);
    } else if (fb_bpp == 24) {
        p[0] = (unsigned char)(rgb & 0xFF);
        p[1] = (unsigned char)((rgb >> 8) & 0xFF);
        p[2] = (unsigned char)((rgb >> 16) & 0xFF);
    } else if (fb_bpp == 16) {
        unsigned char r = (unsigned char)((rgb >> 16) & 0xFF);
        unsigned char g = (unsigned char)((rgb >> 8) & 0xFF);
        unsigned char b = rgb & 0xFF;
        unsigned short packed = (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        p[0] = (unsigned char)(packed & 0xFF);
        p[1] = (unsigned char)((packed >> 8) & 0xFF);
    }
}

static void draw_glyph(unsigned int col, unsigned int row, char c, unsigned int fg, unsigned int bg)
{
    const unsigned char* glyph = &iso_font[(unsigned char)c * GLYPH_H];

    unsigned int base_x = col * GLYPH_W;
    unsigned int base_y = row * GLYPH_H;

    for (unsigned int gy = 0; gy < GLYPH_H; gy++) {
        unsigned char bits = glyph[gy];
        for (unsigned int gx = 0; gx < GLYPH_W; gx++) {
            unsigned int rgb = (bits & (1 << gx)) ? fg : bg;
            put_pixel(base_x + gx, base_y + gy, rgb);
        }
    }
}

static void draw_cell_blank(unsigned int col, unsigned int row, unsigned int bg)
{
    draw_glyph(col, row, ' ', bg, bg);
}

static inline void screen_init(unsigned long long addr, unsigned int pitch,
                  unsigned int width, unsigned int height, unsigned int bpp)
{
    if (addr == 0 || (bpp != 32 && bpp != 24 && bpp != 16)) {
        fb_addr = 0;
        cols = VGA_COLS;
        rows = VGA_ROWS;
        vga_text_offset = 0;
        return;
    }

    fb_addr   = (unsigned char*)(unsigned int)addr;
    fb_pitch  = pitch;
    fb_width  = width;
    fb_height = height;
    fb_bpp    = bpp;

    cols = fb_width / GLYPH_W;
    rows = fb_height / GLYPH_H;
    cursor_index = 0;
}

static void scroll_screen(void)
{
    if (!fb_addr) {
        unsigned short* vga_mem = (unsigned short*)VGA_TEXT_ADDR;
        for (int i = 0; i < VGA_COLS * (VGA_ROWS - 1); i++) {
            vga_mem[i] = vga_mem[i + VGA_COLS];
        }
        for (int i = VGA_COLS * (VGA_ROWS - 1); i < VGA_COLS * VGA_ROWS; i++) {
            vga_mem[i] = (current_vga_attr << 8) | ' ';
        }
        vga_text_offset -= VGA_COLS;
        return;
    }

    unsigned int row_bytes = GLYPH_H * fb_pitch;
    unsigned char* dst = fb_addr;
    unsigned char* src = fb_addr + row_bytes;
    unsigned int bytes_to_move = row_bytes * (rows - 1);

    for (unsigned int i = 0; i < bytes_to_move; i++) {
        dst[i] = src[i];
    }

    for (unsigned int c = 0; c < cols; c++) {
        draw_cell_blank(c, rows - 1, current_bg);
    }

    cursor_index -= cols;
}

static void advance_cursor(void)
{
    if (!fb_addr) {
        while (vga_text_offset >= VGA_COLS * VGA_ROWS) {
            scroll_screen();
        }
        return;
    }
    while (cursor_index >= cols * rows) {
        scroll_screen();
    }
}

static unsigned int parse_ansi_num(const char* str, int* idx)
{
    unsigned int val = 0;
    while (str[*idx] >= '0' && str[*idx] <= '9') {
        val = (val * 10) + (str[*idx] - '0');
        (*idx)++;
    }
    return val;
}

static inline void fb_print(const char* str, uint8_t col)
{
    current_fg = color_to_rgb(col & 0x0F);
    current_vga_attr = (current_vga_attr & 0xF0) | (col & 0x0F);

    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == '\033' && str[i+1] == '[') {
            int orig_i = i;
            i += 2;
            unsigned int p1 = parse_ansi_num(str, &i);
            
            if (p1 == 38 && str[i] == ';') {
                i++;
                unsigned int p2 = parse_ansi_num(str, &i);
                if (p2 == 2 && str[i] == ';') {
                    i++;
                    unsigned int r = parse_ansi_num(str, &i);
                    if (str[i] == ';') i++;
                    unsigned int g = parse_ansi_num(str, &i);
                    if (str[i] == ';') i++;
                    unsigned int b = parse_ansi_num(str, &i);
                    
                    if (str[i] == 'm') {
                        i++;
                        current_fg = (r << 16) | (g << 8) | b;
                        current_vga_attr = (current_vga_attr & 0xF0) | rgb_to_vga_attr(current_fg);
                        continue;
                    }
                }
            } else if (p1 == 0 && str[i] == 'm') {
                i++;
                current_fg = color_to_rgb(col & 0x0F);
                current_bg = 0x00000000;
                current_vga_attr = (current_vga_attr & 0xF0) | (col & 0x0F);
                continue;
            }

            i = orig_i;
        }

        if (str[i] == '\n') {
            if (!fb_addr) {
                vga_text_offset = ((vga_text_offset / VGA_COLS) + 1) * VGA_COLS;
            } else {
                cursor_index = ((cursor_index / cols) + 1) * cols;
            }
            advance_cursor();
            i++;
            continue;
        }

        if (!fb_addr) {
            unsigned short* vga_mem = (unsigned short*)VGA_TEXT_ADDR;
            vga_mem[vga_text_offset++] = (current_vga_attr << 8) | str[i];
        } else {
            draw_glyph(cursor_index % cols, cursor_index / cols, str[i], current_fg, current_bg);
            cursor_index++;
        }
        advance_cursor();
        i++;
    }
}

static inline void print_char(char c)
{
    if (c == '\n') {
        if (!fb_addr) {
            vga_text_offset = ((vga_text_offset / VGA_COLS) + 1) * VGA_COLS;
        } else {
            cursor_index = ((cursor_index / cols) + 1) * cols;
        }
        advance_cursor();
        return;
    }

    if (c == '\b') {
        if (!fb_addr) {
            if (vga_text_offset > 0) {
                vga_text_offset--;
                unsigned short* vga_mem = (unsigned short*)VGA_TEXT_ADDR;
                vga_mem[vga_text_offset] = (current_vga_attr << 8) | ' ';
            }
        } else {
            if (cursor_index > 0) {
                cursor_index--;
                draw_cell_blank(cursor_index % cols, cursor_index / cols, current_bg);
            }
        }
        return;
    }

    if (!fb_addr) {
        unsigned short* vga_mem = (unsigned short*)VGA_TEXT_ADDR;
        vga_mem[vga_text_offset++] = (current_vga_attr << 8) | c;
    } else {
        draw_glyph(cursor_index % cols, cursor_index / cols, c, current_fg, current_bg);
        cursor_index++;
    }
    advance_cursor();
}

static inline void fb_print_uint(unsigned int n)
{
    char buf[11];
    int i = 10;
    buf[10] = '\0';

    if (n == 0) {
        print_char('0');
        return;
    }

    while (n > 0 && i > 0) {
        i--;
        buf[i] = '0' + (n % 10);
        n /= 10;
    }

    fb_print(&buf[i], 0x0F);
}

static inline void fb_print_hex(unsigned int n)
{
    const char* hex_digits = "0123456789ABCDEF";
    char buf[9];
    buf[8] = '\0';

    for (int i = 7; i >= 0; i--) {
        buf[i] = hex_digits[n & 0xF];
        n >>= 4;
    }

    fb_print("0x", 0x0F);
    fb_print(buf, 0x0F);
}

static inline void clear_screen(unsigned char color)
{
    current_bg = color_to_rgb(color);
    current_vga_attr = (current_vga_attr & 0xF0) | ((color & 0x0F) << 4);

    if (!fb_addr) {
        unsigned short* vga_mem = (unsigned short*)VGA_TEXT_ADDR;
        for (int i = 0; i < VGA_COLS * VGA_ROWS; i++) {
            vga_mem[i] = (current_vga_attr << 8) | ' ';
        }
    } else {
        for (unsigned int r = 0; r < rows; r++) {
            for (unsigned int c = 0; c < cols; c++) {
                draw_cell_blank(c, r, current_bg);
            }
        }
    }

    clear_screen_reset_cursor();
}

static inline void clear_screen_reset_cursor(void)
{
    cursor_index = 0;
    vga_text_offset = 0;
}

#endif
