#ifndef COLORS_H
#define COLORS_H

// 4 bit stuff
#define VGA_BLACK         0x00
#define VGA_BLUE          0x01
#define VGA_GREEN         0x02
#define VGA_CYAN          0x03
#define VGA_RED           0x04
#define VGA_MAGENTA       0x05
#define VGA_BROWN         0x06
#define VGA_LIGHT_GREY    0x07
#define VGA_DARK_GREY     0x08
#define VGA_LIGHT_BLUE    0x09
#define VGA_LIGHT_GREEN   0x0A
#define VGA_LIGHT_CYAN    0x0B
#define VGA_LIGHT_RED     0x0C
#define VGA_LIGHT_MAGENTA 0x0D
#define VGA_YELLOW        0x0E
#define VGA_WHITE         0x0F

// 24 bit stuff
#define RGB_BLACK         0x000000
#define RGB_BLUE          0x0000AA
#define RGB_GREEN         0x00AA00
#define RGB_CYAN          0x00AAAA
#define RGB_RED           0xAA0000
#define RGB_MAGENTA       0xAA00AA
#define RGB_BROWN         0xAA5500
#define RGB_LIGHT_GREY    0xAAAAAA
#define RGB_DARK_GREY     0x555555
#define RGB_LIGHT_BLUE    0x5555FF
#define RGB_LIGHT_GREEN   0x55FF55
#define RGB_LIGHT_CYAN    0x55FFFF
#define RGB_LIGHT_RED     0xFF5555
#define RGB_LIGHT_MAGENTA 0xFF55FF
#define RGB_YELLOW        0xFFFF55
#define RGB_WHITE         0xFFFFFF

// and whatever this is
#define ANSI_RESET        "\033[0m"
#define ANSI_RED          "\033[38;2;170;0;0m"
#define ANSI_GREEN        "\033[38;2;0;170;0m"
#define ANSI_YELLOW       "\033[38;2;255;255;85m"
#define ANSI_BLUE         "\033[38;2;85;85;255m"

// The OS fails to compile with these things. Eggs. Leftover shit.
#define BLACK         VGA_BLACK
#define BLUE          VGA_BLUE
#define GREEN         VGA_GREEN
#define CYAN          VGA_CYAN
#define RED           VGA_RED
#define MAGENTA       VGA_MAGENTA
#define BROWN         VGA_BROWN
#define LIGHT_GREY    VGA_LIGHT_GREY
#define DARK_GREY     VGA_DARK_GREY
#define LIGHT_BLUE    VGA_LIGHT_BLUE
#define LIGHT_GREEN   VGA_LIGHT_GREEN
#define LIGHT_CYAN    VGA_LIGHT_CYAN
#define LIGHT_RED     0x0C
#define LIGHT_MAGENTA 0x0D
#define YELLOW        VGA_YELLOW
#define WHITE         VGA_WHITE


#endif
