/*
main.c
By Turlum25 , Added YNB beta

Sometimes code needs to be written via hand

Oct 16 2026: I WILL FUCKING KILL YOU DLS()
             Re: I removed it. Peace at last

Note from dev: 





















            Gay.
*/

#include "kernel.h"
#include "keyboard.h"

#include "system/detect.h"
#include "system/memory.h"
#include "system/time.h"

#define DISK_IMPLEMENTATION
#define SATA_IMPLEMENTATION
#define EXT2_IMPLEMENTATION

#include "disk/tmpfs.h"
#include "disk/sata.h"

// Vowed myself to not add this. Had to add this too.
#include <stdint.h>
#include <stddef.h>


// Dear forkers:
// Functions on top. Main cool stuff bottom. Don't leave early. Thanks

/*
#define VGA_COLOR_WHITE 0x0Fu
#define VGA_COLOR_RED   0x0Cu
#define VGA_COLOR_GREEN 0x0Au
#define VGA_COLOR_YELLOW 0x0Eu
*/

static uint8_t str_sector_buffer[SATA_SECTOR_SIZE];

// If debug = 1, then debug. Else, no debug. Fried eggs
static int debug = 1;
extern void bios_probe(void);
volatile uint16_t bios_probe_flags;
volatile uint16_t bios_probe_bx;


char User[1024] = "NULL"; // Username. Default NULL
char Host[1024] = "NULL"; // PC hostname. Default NULL

// I SWEAR TO GOD I WILL SCREAM TOUR CAN PRINT ANY VARIABLE BUT THIS FOR SOME FUCKING REASON
// static const char VERSION[8] = "0.03"; // TOUR VERSION. Don't change variable naem. We don't gotta be dumbasses, yk
// I commented it. God i want some peace, and blame fucking God for making me rewrite.. printcol. MY BABY!! WAAAH
// New version below.

int VERSION(int COLOR) { // This will never \n
    // Notes: This int just prints the version. Mod however you like, forkers 
    //        COLOR defines the color you wanna put. 
    printcol("0.04", COLOR);
}


// Signifies whether disk is mounted or not.
// 0 = not mounted
// You can find it being used first in initproc().
static uint8_t mounted = 0;

uint32_t CurrentTime = 0;


char SeeData[1024]; // This is where strsee() returns its data to.



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



int strsee(uint32_t sector_num) {
    if (mounted == 0) { 
        printcol("Disk is not mounted.\n", VGA_COLOR_RED); 
        return 0; 
    }

    for (uint32_t i = 0; i < 1024; i++) {
        SeeData[i] = '\0';
    }

    uint8_t raw_buffer[SATA_SECTOR_SIZE];
    for (uint32_t i = 0; i < SATA_SECTOR_SIZE; i++) {
        raw_buffer[i] = '\0';
    }

    if (!sata_read_sector(&primary_sata_master, sector_num, raw_buffer)) {
        return 0;
    }

    uint8_t is_empty = 1;
    for (uint32_t i = 0; i < SATA_SECTOR_SIZE; i++) {
        if (raw_buffer[i] != '\0') {
            is_empty = 0;
            break;
        }
    }

    if (is_empty) {
        SeeData[0] = '\0'; 
        return 1;
    }

    uint32_t out_idx = 0;
    for (uint32_t i = 0; i < SATA_SECTOR_SIZE; i++) {
        char c = (char)raw_buffer[i];
        if (c >= 32 && c <= 126) {
            SeeData[out_idx++] = c;
        }
    }
    
    SeeData[out_idx] = '\0';
    return 1;
}

int strwrite(uint32_t sector_num, const char *data) {
    if (mounted == 0) { 
        printcol("Disk is not mounted.\n", VGA_COLOR_RED); 
        return 0; 
    }
    if (!data || *data == '\0') {
        return 0;
    }

    uint8_t sector_buff[SATA_SECTOR_SIZE];
    for (uint32_t i = 0; i < SATA_SECTOR_SIZE; i++) {
        sector_buff[i] = '\0';
    }

    uint32_t i = 0;
    while (data[i] != '\0' && i < (SATA_SECTOR_SIZE - 1)) {
        sector_buff[i] = (uint8_t)data[i];
        i++;
    }

    if (!sata_write_sector(&primary_sata_master, sector_num, sector_buff)) {
        return 0;
    }

    return strsee(sector_num);
}





// Commands.
void ver() {
    printcol("TOUR (TOUR Offends UNIX Rules)\nBy Turlum25\n=========================\n", VGA_COLOR_GREEN);
    printcolvar((uint32_t)"Version : ", 0, VGA_COLOR_RED);
    VERSION(VGA_COLOR_YELLOW);
    printcol("\n", VGA_COLOR_YELLOW);
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
    printcol("help    - show this message\n", VGA_COLOR_WHITE);
    printcol("ver     - show version\n", VGA_COLOR_WHITE); // Practically useless after i made fetch.
    printcol("clear   - clear screen\n", VGA_COLOR_WHITE);
    printcol("echo    - repeat text \n", VGA_COLOR_WHITE);
    printcol("peek    - read a byte from RAM address\n", VGA_COLOR_WHITE);
    printcol("poke    - modify a byte at RAM address\n", VGA_COLOR_WHITE);
    printcol("fetch   - get system and RAM info\n", VGA_COLOR_WHITE);
    printcol("time    - get current time in seconds since boot\n", VGA_COLOR_WHITE);
    printcol("sleep   - wait specific time\n", VGA_COLOR_WHITE);
    printcol("strpeek - read specific sector in SSD\n", VGA_COLOR_WHITE);
    printcol("strpoke - write specific sector in SSD\n", VGA_COLOR_WHITE);
    printcol("reboot  - reboot computer\n", VGA_COLOR_WHITE);
}
void echo(const char *buffer) {
    if (strings_match(buffer, "echo")) {
        printcol("\n", VGA_COLOR_WHITE);
        return;
    }
    if (StringPrefixMatch(buffer, "echo ")) {
        printcol(buffer + 5, VGA_COLOR_WHITE);
    } else {
        printcol("Incorrect echo formatting\nUsage: echo <text>\n", VGA_COLOR_RED);
    }
}

void peek(const char *buffer) {
    uint32_t address = parse_hex(buffer + 5);
    volatile uint8_t *ptr = (volatile uint8_t *)address;
    uint8_t value = *ptr;

    printcol("Reading memory\n", VGA_COLOR_WHITE);
    if (value >= 32 && value <= 126) {
        char out_str[2] = {(char)value, '\0'};
        printcol(out_str, VGA_COLOR_YELLOW);
    } else {
        printcol("Error (Cannot read bytes)\n", VGA_COLOR_RED);
    }
}

void poke(const char *buffer) {
    int i = 5;
    while (buffer[i] != ' ' && buffer[i] != '\0') {
        i++;
    }
    if (buffer[i] == '\0') {
        printcol("Error (Invalid target value. Did you enter right value?)\n", VGA_COLOR_RED);
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
        printcol("Critical error (User tried modifying kernel space)\n", VGA_COLOR_RED);
        return;
    }
    
    volatile uint8_t *ptr = (volatile uint8_t *)address;
    *ptr = value;
    
    printcol("Done", VGA_COLOR_GREEN);
}


// Dear me: ...
//          Presenting.. Just fetch.
void fetch() {
    DetectCPU();
    checkprintram();
    
    printcol("\n", VGA_COLOR_WHITE);
    printcolvar((uint32_t)User, 0, VGA_COLOR_GREEN);
    printcol(" * ", VGA_COLOR_GREEN);
    printcolvar((uint32_t)Host, 0, VGA_COLOR_GREEN);
    printcol("\n==============\n", VGA_COLOR_WHITE);
    printcolvar((uint32_t)"TOUR Version : ", 0, VGA_COLOR_GREEN);
    VERSION(VGA_COLOR_YELLOW);
    printcol("\n", VGA_COLOR_WHITE);
    printcolvar((uint32_t)"CPU          : ", 0, VGA_COLOR_WHITE);
    printcolvar((uint32_t)CPUType, 0, VGA_COLOR_YELLOW);
    printcol("\n", VGA_COLOR_WHITE);
    
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
    printcolvar((uint32_t)"Seconds from boot: ", 0, VGA_COLOR_WHITE);
    printcolvar(CurrentTime, 1, VGA_COLOR_YELLOW);
}


// TODO: Fix this shitty command. Doesnt work properly
//       TLDR: Disgrace, fix.
// Re: TODO done, at least i think 
//     Might still be one of, at least, the shittiest commands of this kernel ever. 
//     I admit it: I have not tested this. I am full of shit just like roman bellic 
//                 Niko my cousin! Let's go for some bowling
void snore() {
    // debug = 1; // EMABLE DEBUG FOR SLEEP ET CETERA
    checkCMOSTime();
    char buff1[1024];
    if (debug == 1) { // Originally added debug just to see buff1. How lazy am I?
        printcolvar((uint32_t)"[debug] buffer : ", 0, VGA_COLOR_YELLOW);
        printcolvar((uint32_t)buff1, 0, VGA_COLOR_YELLOW);
        printcol("\n", VGA_COLOR_WHITE);
    }
    input("Enter seconds to sleep : ", buff1, buff1 + 1024);
    int preseconds = parse_hex(buff1);
    int seconds = preseconds + 1; // Add 1 second to sleep. THen it'll be accurate to user input
                                  // "Your honor i added an extra second to align with *fucking reality*"
    sleep(seconds);
    if (debug == 1) {
        printcolvar((uint32_t)"[debug] buffer : ", 0, VGA_COLOR_YELLOW);
        printcolvar((uint32_t)buff1, 0, VGA_COLOR_YELLOW);
        printcol("\n", VGA_COLOR_WHITE);
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
        printcol("\n", VGA_COLOR_WHITE);
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


/*
INITPROC
This is initproc
Veri good
Helthi
Importan
*/


// usernane()
// Checks for username, etc etc.
// Dev note: Dear me, this might worsen your posture
//           And a neck beard
//           Because handwritten code is always shit.
int username() {
    printcol("[disk] Searching for user in disk\n", VGA_COLOR_WHITE);
    sleep(1);
    char YNcache[10];
    if (mounted == 0) {
        printcol("[warning] Cannot search for username, SATA disk not mounted.", VGA_COLOR_RED);
        sleep(1);
        return 2;
    }
    // This checks whether username is in sector 10.
    // If not, it asks for username. Then it writes to disk. Repeat
    strsee(10); // Check for username
    if(SeeData[0] == '\0' || SeeData[0] == '0') { // If strsee returns 0, that means nothing is in sector 10.
        printcol("[warning] Username not found in disk (sector 10)\n", VGA_COLOR_RED);
        input("Skip username? (Y/n) : ", YNcache, YNcache + 10);
        if (YNcache[0] == 'Y' || YNcache[0] == 'y') { return 2; } // Exit if user wanna skip the username
        else {
            input("Enter username: ", User, User + 1024);
            if (User[0] == '0') {
                printcol("[error] You cannot set first character as 0.", VGA_COLOR_RED);
                reboot();
            }
            strwrite(10, User);
            strsee(10);
        }
    }
    else {
        printcol("Found username: ", VGA_COLOR_WHITE);
        printcolvar((uint32_t)SeeData, 0, VGA_COLOR_YELLOW);
        charis(User, SeeData);
        printcol("\n", VGA_COLOR_WHITE);
        return 1;
    }
}

// hostname()
// Just same thing as username, BUT checks for data in sector 9.
// Practically useless'nt

int hostname() {
    printcol("[disk] Searching for hostname in disk\n", VGA_COLOR_WHITE);
    sleep(1);
    char yncache[10];
        if (mounted == 0) {
        printcol("[warning] Cannot search for hostname, SATA disk not mounted\n          Hostname will temporarily be set to NULL and not be saved.", VGA_COLOR_RED);
        sleep(1);
        return 2;
    }
    strsee(9); // Check for username in sector 9
    if (SeeData[0] == '\0' || SeeData[0] == '0') { // Check if sector is empty or value is 0
        printcol("[warning] Cannot find hostname in disk (sector 9)\n", VGA_COLOR_RED);
        input("Skip hostname? (Y/n): ", yncache, yncache + 10);
        if (yncache[0] == 'Y' || yncache[0] == 'y') {
            printcol("[note] You are skipping your hostname, so your hostname will temporarily be set to NULL.\n      Unless you reboot, you will not be able to re-set your hostname", VGA_COLOR_YELLOW);
        } else {
            input("Enter hostname: ", Host, Host + 1024); 
            if (Host[0] == '0') {
                printcol("[error] You cannot set first character as 0.", VGA_COLOR_RED);
                reboot();
            }
            strwrite(9, Host);
            strsee(9);
        }
    }
    else {
        printcol("[disk] Found hostname: ", VGA_COLOR_GREEN);
        printcolvar((uint32_t)SeeData, 0, VGA_COLOR_YELLOW);
        charis(Host, SeeData);
        printcol("\n", VGA_COLOR_WHITE);
        return 1;
    }
}


// Starts up username, disk drivers.

int initproc() {
    printcol("\n\nTOUR, by Turlum25, September, October 2026\n", VGA_COLOR_GREEN);
    printcolvar((uint32_t)"Version detected: ", 0, VGA_COLOR_GREEN);
    VERSION(VGA_COLOR_YELLOW);
    printcol("\n[warning] You may not be booting off of disk.\n          Your data may be lost after rebooting.\n", VGA_COLOR_RED);

    init_tmpfs(); // Starts temporary file system. Needed for disk mounting

    printcol("\n[note] This disk driver is still in development. Please use in caution. \n", VGA_COLOR_YELLOW);
    sleep(0.5);
    
    if (sata_detect_device(&primary_sata_master, 0x1F0, 0x3F6, 0)) {
        printcol("[disk] Found SATA storage link at primary master channel\n", VGA_COLOR_GREEN);
        mounted = 1;
    } else {
        printcol("[warning] Storage not available or sig bytes mismatched\n", VGA_COLOR_RED);
    }

    if (!mounted) {
        printcol("[warning] No mountable disks found.\n\n", VGA_COLOR_RED);
    }

    username();
    /*
    Yeah no need. username() at its duty since 1 am at fucking 20 october
    // Practically telling user to look for their names in the large bytes of ram. HAHAHAHAHAHA IM SO EVIL
    printcol("\n[warning] Username is currently stored in RAM as char 'User'.", VGA_COLOR_RED); 
    // Sir, I was late for work because I was looking for my username in TOUR's RAM
    */
    hostname();


    return 0;
}


// End of initproc
// Ok

// Just too damn lazy to explain these two functions, read it, earn some IQ points (you probably have none)
// In case you're a really big dumbass who doesn't know how to read C and use AI to use it:
// strpeek absolutely sudo rm -fr /* your PC. 
// strpoke totally haxxors the mainframe of your pentagon opsec levels and accesses the memory buffer of each FBI agent's terminal
// Trust (/s)

#define STR_SECTOR_SIZE 512u

__attribute__((aligned(4096))) char ReadStorage[512];


static int parse_lba_silent(const char *buff, const char *cmd, uint32_t *lba, const char **args) {
    if (!buff || !cmd || !lba) return 0;
    while (*cmd && *buff == *cmd) { cmd++; buff++; }
    if (*cmd == '\0' && *buff == '\0') {
        *lba = 0u; // Fallback to safe baseline sector
        if (args) *args = buff;
        return 1;  
    }
    if (*cmd != '\0' || (*buff != ' ' && *buff != '\t')) return 0;
    while (*buff == ' ' || *buff == '\t') buff++;

    uint32_t base = 10u;
    if (buff[0] == '0' && (buff[1] == 'x' || buff[1] == 'X')) {
        base = 16u;
        buff += 2;
    }

    uint32_t val = 0;
    int digits = 0;
    while (*buff && *buff != ' ' && *buff != '\t') {
        uint32_t d = 0xFFFFFFFFu;
        if (*buff >= '0' && *buff <= '9') d = (uint32_t)(*buff - '0');
        else if (base == 16u) {
            if (*buff >= 'a' && *buff <= 'f') d = (uint32_t)(*buff - 'a' + 10);
            else if (*buff >= 'A' && *buff <= 'F') d = (uint32_t)(*buff - 'A' + 10);
        }
        if (d >= base || val > (0xFFFFFFFFu - d) / base) return 0;
        val = val * base + d;
        digits++;
        buff++;
    }
    if (!digits) return 0;
    *lba = val;
    while (*buff == ' ' || *buff == '\t') buff++;
    if (args) *args = buff;
    return 1;
}

int strpeek(const char *buff) {
    if (!mounted) { 
        printcol("Disk is not mounted.\n", VGA_COLOR_RED); 
        return 1; 
    }
    uint32_t lba;
    if (!parse_lba_silent(buff, "strpeek", &lba, NULL)) return 0;

    for (uint32_t i = 0; i < STR_SECTOR_SIZE; i++) ReadStorage[i] = '\0';
    if (!sata_read_sector(&primary_sata_master, lba, (uint8_t *)ReadStorage)) return 0;
    char out_str[2] = { '\0', '\0' };
    
    for (uint32_t i = 0; i < STR_SECTOR_SIZE; i++) {
        char c = ReadStorage[i];
        
        if (c >= 32 && c <= 126) {
            out_str[0] = c;
            printcol(out_str, VGA_COLOR_WHITE); 
        } else {
            printcol(".", VGA_COLOR_WHITE);
        }
    }
    
    printcol("\n", VGA_COLOR_WHITE);
    return 1;
}


int strpoke(const char *buff) {
    if (!mounted) { printcol("Disk is not mounted.\n", VGA_COLOR_RED); 
        return 1; }
    uint32_t lba;
    const char *payload = NULL;
    uint8_t sector_buff[STR_SECTOR_SIZE];

    if (!parse_lba_silent(buff, "strpoke", &lba, &payload) || !payload || *payload == '\0') return 0;

    for (uint32_t i = 0; i < STR_SECTOR_SIZE; i++) sector_buff[i] = '\0';

    uint32_t i = 0;
    while (payload[i] != '\0' && i < (STR_SECTOR_SIZE - 1u)) {
        sector_buff[i] = (uint8_t)payload[i];
        i++;
    }

    if (!sata_write_sector(&primary_sata_master, lba, sector_buff)) return 0;
    strpeek(buff);
    return 1;
}

// Why do I not comment at all in super fucking duper complicated functions? 


// Dear spookinate from Discord: 
// Prepare to be scared. I added a shell to the fucking kernel
// HAHAHAHA I BROKE THE LAWS OF FUCKING KERNEL DEVELOPMENT
// MIJAJAJAAFBFUIHJGFUIBFDUYFEUIYHFGFUYTEGFBIHDGFUE

void kernel_main(void) __attribute__((noreturn, used));

void kernel_main(void) {
    
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
    static uint8_t STRPOKE = 0;
    static uint8_t STRPEEK = 0;
    static uint8_t FETCH = 0;
    static uint8_t TIME = 0;
    static uint8_t SLEEP = 0;
    static uint8_t ENABLEDEBUG = 0;
    static uint8_t DISABLEDEBUG = 0;
    static uint8_t REBOOT = 0;

    
    initproc();
    printcol("\nType 'help' for a list of commands.", VGA_COLOR_GREEN);
    printcol("\n", VGA_COLOR_WHITE);

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
        STRPEEK = StringPrefixMatch(buff, "strpeek ");
        STRPOKE = StringPrefixMatch(buff, "strpoke ");

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
        else if (STRPEEK != 0) { strpeek(buff); }
        else if (STRPOKE != 0) { strpoke(buff); }
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
        STRPEEK = 0;
        STRPOKE = 0;
        // REBOOT = 0; Not needed. This value resets back to 0 after reboot.
        
        
        if (cursor_pos != 0) {
            printcol("\n", VGA_COLOR_WHITE);
        }
        printcol("\n", VGA_COLOR_WHITE);
    }

   // End of main kernel stuff.
}
