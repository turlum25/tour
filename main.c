#include "kernel.h"
#include "keyboard.h"
#include "system/detect.h"
#include "system/memory.h"
#include "system/time.h"

// Dear forkers:
// Functions on top. Main cool stuff bottom. Don't leave early. Thanks

/*
#define VGA_COLOR_WHITE 0x0Fu
#define VGA_COLOR_RED   0x0Cu
#define VGA_COLOR_GREEN 0x0Au
#define VGA_COLOR_YELLOW 0x0Eu
*/

char User[1024]; // Username
static char version[] = "0.02"; // TOUR version. Don't change variable number

uint32_t CurrentTime = 0;
#define str(str1, str2, output) (output = strings_match(str1, str2))


uint32_t parse_hex(const char *str) {
    uint32_t val = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }
    while (*str != '\0') {
        char c = *str++;
        val <<= 4;
        if (c >= '0' && c <= '9')      val += (c - '0');
        else if (c >= 'a' && c <= 'f') val += (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val += (c - 'A' + 10);
        else { val >>= 4; break; }
    }
    return val;
}

// Commands.
void ver() {
    printcol("TOUR (TOUR Offends UNIX Rules)\nBy Turlum25\n=========================", VGA_COLOR_GREEN);
    printcolvar((uint32_t)"Version : ", 0, VGA_COLOR_RED);
    printcolvar((uint32_t)version, 0, VGA_COLOR_YELLOW);
}
void clear() {
    extern uint32_t cursor_pos; 
    volatile uint16_t *const local_vga = (volatile uint16_t *)0xB8000;
    
    for (uint32_t i = 0; i < VGA_CELLS; ++i) {
        local_vga[i] = (uint16_t)((VGA_COLOR_WHITE << 8) | ' ');
    }
    
    cursor_pos = 0; 
}
void help() {
    printcol("help   - show this message", VGA_COLOR_WHITE);
    printcol("ver    - show version", VGA_COLOR_WHITE);
    printcol("clear  - clear screen", VGA_COLOR_WHITE);
    printcol("echo   - repeat text ", VGA_COLOR_WHITE);
    printcol("peek   - read a byte from RAM address", VGA_COLOR_WHITE);
    printcol("poke   - modify a byte at RAM address", VGA_COLOR_WHITE);
    printcol("fetch  - get system and RAM info", VGA_COLOR_WHITE);
    printcol("time   - get current time in seconds since boot", VGA_COLOR_WHITE);
    printcol("sleep  - wait specific time", VGA_COLOR_WHITE);
    printcol("reboot - reboot computer", VGA_COLOR_WHITE);
}
void echo(const char *buffer) {
    if (strings_match(buffer, "echo")) {
        printcol("", VGA_COLOR_WHITE);
        return;
    }
    if (StringPrefixMatch(buffer, "echo ")) {
        printcol(buffer + 5, VGA_COLOR_WHITE);
    } else {
        printcol("Incorrect echo formatting\nUsage: echo <text>", VGA_COLOR_RED);
    }
}

void peek(const char *buffer) {
    uint32_t address = parse_hex(buffer + 5);
    volatile uint8_t *ptr = (volatile uint8_t *)address;
    uint8_t value = *ptr;

    printcol("Reading memory...", VGA_COLOR_WHITE);
    if (value >= 32 && value <= 126) {
        char out_str[2] = {(char)value, '\0'};
        printcol(out_str, VGA_COLOR_YELLOW);
    } else {
        printcol("Error (Cannot really read bytes)", VGA_COLOR_RED);
    }
}

void poke(const char *buffer) {
    int i = 5;
    while (buffer[i] != ' ' && buffer[i] != '\0') {
        i++;
    }
    if (buffer[i] == '\0') {
        printcol("Error (Invalid target value. Did you enter right value?)", VGA_COLOR_RED);
        return;
    }
    
    char addr_buf[32];
    int addr_len = i - 5;
    if (addr_len > 31) addr_len = 31;
    for (int j = 0; j < addr_len; j++) {
        addr_buf[j] = buffer[5 + j];
    }
    addr_buf[addr_len] = '\0';
    
    uint32_t address = parse_hex(addr_buf);
    uint8_t value = (uint8_t)parse_hex(buffer + i + 1);
    

    if (address >= 0x08000 && address < 0x20000) {
        printcol("Critical error (User tried modifying kernel space)", VGA_COLOR_RED);
        return;
    }
    
    volatile uint8_t *ptr = (volatile uint8_t *)address;
    *ptr = value;
    
    printcol("Done", VGA_COLOR_GREEN);
}

void fetch() {
    DetectCPU();
    checkprintram();
    
    printcol("", VGA_COLOR_WHITE);
    printcolvar((uint32_t)User, 0, VGA_COLOR_GREEN);
    printcol("\n==============", VGA_COLOR_WHITE);
    printcolvar((uint32_t)"TOUR Version : ", 0, VGA_COLOR_GREEN);
    printcolvar((uint32_t)version, 0, VGA_COLOR_YELLOW);
    printcol("", VGA_COLOR_WHITE);
    printcolvar((uint32_t)"CPU          : ", 0, VGA_COLOR_WHITE);
    printcolvar((uint32_t)CPUType, 0, VGA_COLOR_YELLOW);
    printcol("", VGA_COLOR_WHITE);
    
    uint32_t print_taken = (uint32_t)TakenRAM;
    uint32_t print_total = (uint32_t)TotalRAM; /*TotalRAM always will print 640KB. That's what the fucking BIOS sees apparently.
                                                 Really damn irritating. Hope some smartass fixes it.
                                                 Re: Fixed. Meh.*/                                    

    printcolvar((uintptr_t)"RAM Taken    : ", 0, VGA_COLOR_WHITE);
    printcolvar(print_taken, 1, VGA_COLOR_YELLOW);
    printcolvar((uintptr_t)"KB / ", 0, VGA_COLOR_YELLOW);
    printcolvar(print_total, 1, VGA_COLOR_YELLOW); 
    printcolvar((uintptr_t)"KB\n", 0, VGA_COLOR_YELLOW);
}

void time() {
    checkCMOSTime();
    printcolvar(CurrentTime, 1, VGA_COLOR_YELLOW);
}

// If debug = 1, then debug. Else, no debug. Fried eggs
static int debug = 0;


// TODO: Fix this shitty command. Doesnt work properly
//       TLDR: Disgrace, fix.
void snore() {
    // debug = 1; // EMABLE DEBUG FOR SLEEP ET CETERA
    checkCMOSTime();
    char buff1[1024];
    if (debug == 1) {
        printcolvar((uint32_t)"[debug] buffer : ", 0, VGA_COLOR_YELLOW);
        printcolvar((uint32_t)buff1, 0, VGA_COLOR_YELLOW);
        printcol("", VGA_COLOR_WHITE);
    }
    input("Enter seconds to sleep : ", buff1, buff1 + 1024);
    int preseconds = parse_hex(buff1);
    int seconds = preseconds + 1; // Add 1 second to sleep. THen it'll be accurate to user input
    sleep(seconds);
    if (debug == 1) {
        printcolvar((uint32_t)"[debug] buffer : ", 0, VGA_COLOR_YELLOW);
        printcolvar((uint32_t)buff1, 0, VGA_COLOR_YELLOW);
        printcol("", VGA_COLOR_WHITE);
    }
    buff1[0] = '\0';
    buff1[1] = '\0';
    buff1[2] = '\0';
    buff1[3] = '\0';
    buff1[4] = '\0';
    buff1[5] = '\0';
    buff1[6] = '\0';
    buff1[7] = '\0';
    buff1[8] = '\0';
    buff1[9] = '\0';
    buff1[10] = '\0';
    if (debug == 1) {
        printcolvar((uint32_t)"[debug] buffer : ", 0, VGA_COLOR_YELLOW);
        printcolvar((uint32_t)buff1, 0, VGA_COLOR_YELLOW);
        printcol("", VGA_COLOR_WHITE);
    }
    seconds = 0; // Clear seconds for next run
    preseconds = 0; // Also this. We shall not forget.
    // debug = 0; // DISABLE DEBUG FOR OTHER SHIT
}

int reboot() { // We purposefully triple faulting CPU to reboot. Hehe!
    volatile uint16_t zero_idt[3] = {0, 0, 0};
    __asm__ volatile ("lidt %0" : : "m"(zero_idt));
    __asm__ volatile ("int $3");
    while (1) {
        __asm__ volatile ("hlt");
    }
}

int enableDebug() {
    debug = 1;
}
int disableDebug() {
    debug = 0;
}




int initproc() {
    printcol("\n\nTOUR, by Turlum25, September, October 2026", VGA_COLOR_GREEN);
    printcolvar((uint32_t)"Version detected: ", 0, VGA_COLOR_GREEN);
    printcolvar((uint32_t)version, 0, VGA_COLOR_YELLOW);
    printcol("\n[warning] You may not be booting off of disk.\n          Your data may be lost after rebooting.", VGA_COLOR_RED);
    input("Enter username: ", User, User + 1024);
    printcolvar((uint32_t)"Your username is ", 0, VGA_COLOR_GREEN);
    printcolvar((uint32_t)User, 0, VGA_COLOR_GREEN);
    printcol("\n[warning] Username is currently stored in RAM as char 'User'.", VGA_COLOR_RED);

}


void _start(void) __attribute__((section(".text._start"), noreturn, used));

void _start(void) {
    
    extern uint8_t __bss_start, __bss_end;
    extern uint32_t cursor_pos; 
    volatile uint8_t *p;
    

    for (p = &__bss_start; p < &__bss_end; ++p) {
        *p = 0;
    }

    // Main kernel print, stuff.

    char buff[1024]; // Input buffer

    // Input string match stuff. Will be 1 when matches.
    static uint8_t CLEAR = 0;
    static uint8_t HELP = 0;
    static uint8_t VER = 0;
    static uint8_t ECHO = 0;
    static uint8_t EXIT = 0;
    static uint8_t PEEK = 0;
    static uint8_t POKE = 0;
    static uint8_t FETCH = 0;
    static uint8_t TIME = 0;
    static uint8_t SLEEP = 0;
    static uint8_t ENABLEDEBUG = 0;
    static uint8_t DISABLEDEBUG = 0;
    static uint8_t REBOOT = 0;

    
    initproc();
    printcol("\nType 'help' for a list of commands.", VGA_COLOR_GREEN);
    printcol("", VGA_COLOR_WHITE);

    while (1) {
        input("# ", buff, buff + 1024);

        str(buff, "help", HELP);
        str(buff, "ver", VER);
        str(buff, "clear", CLEAR);
        ECHO = StringPrefixMatch(buff, "echo ") || strings_match(buff, "echo");
        PEEK = StringPrefixMatch(buff, "peek ");
        POKE = StringPrefixMatch(buff, "poke ");
        str(buff, "fetch", FETCH);
        str(buff, "time", TIME);
        str(buff, "sleep", SLEEP);
        str(buff, "enabledebug", ENABLEDEBUG);
        str(buff, "disabledebug", DISABLEDEBUG);
        str(buff, "reboot", REBOOT);

        if (HELP != 0) { help(); }
        else if (VER != 0) { ver(); }
        else if (ECHO != 0) { echo(buff); }
        else if (CLEAR != 0) { clear(); }
        else if (PEEK != 0) { peek(buff); }
        else if (POKE != 0) { poke(buff); }
        else if (FETCH != 0) { fetch(); }
        else if (TIME != 0) { time(); }
        else if (SLEEP != 0) { snore(); }
        else if (ENABLEDEBUG != 0) { enableDebug(); }
        else if (DISABLEDEBUG != 0) { disableDebug(); }
        else if (REBOOT != 0) { reboot(); }
        else { 
            printcolvar((uint32_t)"Incorrect command: ", 0, VGA_COLOR_RED);
            printcolvar((uint32_t)buff, 0, VGA_COLOR_RED);
        }
        
        // This block clears out the variables that enable commands.
        // Specific commands at least. 
        CLEAR = 0;
        HELP = 0;
        VER = 0;
        ECHO = 0;
        EXIT = 0;
        PEEK = 0;
        POKE = 0;
        TIME = 0;
        SLEEP = 0;
        REBOOT = 0;
        
        
        if (cursor_pos != 0) {
            printcol("", VGA_COLOR_WHITE);
        }
        printcol("", VGA_COLOR_WHITE);
    }

   // End of main kernel stuff.
}
