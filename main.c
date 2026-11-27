/*
main.c
By Turlum25 , Added YNB beta

Sometimes code needs to be written via hand

Oct 16 2026: I WILL FUCKING KILL YOU DLS()
             Re: I removed it. Peace at last

Note from dev: 





















            Gay.
*/

// Dear GOD, why is main.c so simple yet 1,000 lines?


#include "kernel.h"
#include "keyboard.h"

#include "system/detect.h"
#include "system/memory.h"
#include "system/time.h"
#include "system/control.h"

#include "system/version.h"

#define DISK_IMPLEMENTATION
#define SATA_IMPLEMENTATION

#include "disk/tmpfs.h"
#include "disk/sata.h"
// #include "assembler.h"
#include "disk/ext4/ext4.h" // 0.06!
#include "fb.h" // 0.06!
#include "colors.h" // 0x06..

#include "elf/elfext.h" // YEAAAHHH! 0.06!! why tf am i doing this in exams

// Vowed myself to not add this. Had to add this thoguh.
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

char current_directory[256] = "/";



int atoi(const char *str) {
    int result = 0;
    int sign = 1;

    while (*str == ' ')
        str++;

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}




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


int VGA_COLOR_WHITE = 0x0F;


char User[1024] = "root"; // Username. Default NULL

char Host[1024] = "tour"; // PC hostname. Default NULL






// I SWEAR TO GOD I WILL SCREAM TOUR CAN PRINT ANY VARIABLE BUT THIS FOR SOME FUCKING REASON
// static const char VERSION[8] = "0.03"; // TOUR VERSION. Don't change variable naem. We don't gotta be dumbasses, yk
// I commented it. God i want some peace, and blame fucking God for making me rewrite.. printcol. MY BABY!! WAAAH
// New version below.

/* Nov 22 2026: Moved this to system/version.h.
int VERSION(int COLOR) { // This will never \n
    // Notes: This int just prints the version. Mod however you like, forkers 
    //        COLOR defines the color you wanna put. 
    printcol("0.06b2", COLOR);
}
*/


// Signifies whether disk is mounted or not.
// 0 = not mounted
// You can find it being used first in initproc().
static uint8_t mounted = 0;

uint32_t CurrentTime = 0;


char SeeData[1024]; // This is where strsee() returns its data to.


char buffclearer[1024];


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

void charis(char *dest, const char *src) {
    while ((*dest++ = *src++));
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
    clear_screen(0x00);
    cursor_pos = 0;
}

static int shell_next_token(const char **cursor, char *out, size_t max_len);

void run(const char *command) {
    if (!mounted || !ext4_mounted) {
        printcol("[!] Filesystem not mounted, cannot continue\n", VGA_COLOR_RED); // Who tf will use elf without a disk and filesystem?!!!! Wtf!!
        return;
    }
    const char *cursor = command + 4; // The obvious. Then run 
                                      // will start counting from:
                                      // 0[r] 1[u] 2[n] 3[ ] 4[HERE]
                                      // peak!
    char filename[512];
    int token = shell_next_token(&cursor, filename, sizeof(filename));

    if (token <= 0) {
        printcol("Usage: run <file>\n", VGA_LIGHT_CYAN);
        return;
    }

    telf_t program;

    if (!elf_open(&program, filename)) {
        printcol("[elf] [error] Failed to open ELF file\n", VGA_COLOR_RED);
        return;
    }

    printf("[elf] Valid ELF file!\n");

    el_status status = el_load(&program.ctx, elf_allocate);

    printf("[elf] load status: %d\n", (int)status);

    if (el_load(&program.ctx, elf_allocate) != EL_OK) {
        printcol("[elf] [error] Loading failed\n", VGA_COLOR_RED);
        elf_close(&program);
        return;
    }

    printf("[elf] Loaded successfully!\n");

    void (*entry)(void) =
        (void (*)(void))(uintptr_t)program.ctx.ehdr.e_entry;

    elf_close(&program);

    entry();
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
    printcol("read    - print a file from filesystem\n", VGA_COLOR_WHITE);
    printcol("list    - list folders/files, list [-a] [-l] [path]\n", VGA_COLOR_WHITE);
    printcol("chdir   - change directory, cd [path]\n", VGA_COLOR_WHITE);
    printcol("panic   - debug - panic host with return 123\n", VGA_COLOR_WHITE);
    printcol("regdump - dump registers of host\n", VGA_COLOR_WHITE);
    printcol("bcol    - change background of console\n", WHITE);
    printcol("tcol    - black instead of white text\n", WHITE);
    printcol("knad    - information about kernel\n", WHITE);
    printcol("pxl     - mess around with framebuffer", WHITE);
    printcol("reboot  - reboot computer [WARNING: triple faulter]\n", VGA_COLOR_WHITE);
}
void echo(const char *buffer) {
    if (strings_match(buffer, "echo")) {
        print("\n");
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
    
    print("\n");
    printcolvar((uint32_t)User, 0, VGA_COLOR_GREEN);
    printcol(" * ", VGA_COLOR_GREEN);
    printcolvar((uint32_t)Host, 0, VGA_COLOR_GREEN);
    printcol("\n==============\n", VGA_COLOR_WHITE);
    printcolvar((uint32_t)"TOUR Version : ", 0, VGA_COLOR_GREEN);
    VERSION(VGA_COLOR_YELLOW);
    print("\n");
    printcolvar((uint32_t)"CPU          : ", 0, VGA_COLOR_WHITE);
    printcolvar((uint32_t)CPUType, 0, VGA_COLOR_YELLOW);
    print("\n");
    
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
        print("\n");
    }
    input("Enter seconds to sleep : ", buff1, buff1 + 1024);
    int preseconds = parse_hex(buff1);
    int seconds = preseconds + 1; // Add 1 second to sleep. THen it'll be accurate to user input
                                  // "Your honor i added an extra second to align with *fucking reality*"\
                                     Re: 10/31/2026 It somehow works, i don't know why but it somehow does.\
                                         Alongside with the rest of the entire kernel
                                  
    sleep(seconds);
    if (debug == 1) {
        printcolvar((uint32_t)"[debug] buffer : ", 0, VGA_COLOR_YELLOW);
        printcolvar((uint32_t)buff1, 0, VGA_COLOR_YELLOW);
        print("\n");
    }
    charis(buff1, buffclearer);
    if (debug == 1) {
        printcolvar((uint32_t)"[debug] buffer : ", 0, VGA_COLOR_YELLOW);
        printcolvar((uint32_t)buff1, 0, VGA_COLOR_YELLOW);
        print("\n");
    }
    seconds = 0; // Clear seconds for next run
    preseconds = 0; // Also this. We shall not forget.
    // debug = 0; // DISABLE DEBUG FOR OTHER SHIT
}

/*  10/26/2026
    Removed reboot() from main.c to system/control.h
    Added panic, DumpRegister too.
*/

int enableDebug() {
    debug = 1;
}
int disableDebug() {
    debug = 0;
}


/*
INITPROC
This is initproc section
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
        printcol("[warning] Cannot search for username, SATA disk not mounted.\n", VGA_COLOR_RED);
        sleep(1);
        return 2;
    }
    // This checks whether username is in sector 10.
    // If not, it asks for username. Then it writes to disk. Repeat
    strsee(10); // Check for username
    if(SeeData[0] == '\0' || SeeData[0] == '0') { // If strsee returns 0, that means nothing is in sector 10.
        printcol("[warning] Username not found in disk (sector 10)\n", VGA_COLOR_RED);
        input("Skip username? (Y/n) : ", YNcache, YNcache + 10);
        if (YNcache[0] == 'Y' || YNcache[0] == 'y') { printcol("[note] Skipping user will temporarily be set to 'root'.\n       Unless you reboot, you will not be able to re-set your username\n", VGA_COLOR_YELLOW);
            return 2; } // Exit if user wanna skip the username
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
        printcol("[disk] Found username: ", VGA_COLOR_GREEN);
        printcolvar((uint32_t)SeeData, 0, VGA_COLOR_YELLOW);
        charis(User, SeeData);
        print("\n");
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
            printcol("[note] Skipping hostname will temporarily be set to 'tour'.\n       Unless you reboot, you will not be able to re-set your hostname", VGA_COLOR_YELLOW);
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
        print("\n");
        return 1;
    }
}

int fileinit() {
    if (!mounted) {
        return 2;
    }
    printcol("\n[ext4] Initialising filesystem\n", VGA_COLOR_WHITE);
    if (fs_mount()) {
        printcol("[ext4] Filesystem mounted at /\n", VGA_COLOR_GREEN);
    } else {
        printcol("[ext4] [error] ext4 mount failed\n", VGA_COLOR_RED);
        return 0;
    }

    debug = 0;
    if (debug == 1) {
        const char *msg = "Doohickey\n";
        fs_write("/welcome.txt", msg, 27, 0);
    }

    debug = 0;
    return 1;
}




// Starts up username, disk drivers.

int initproc() {
    DetectCPU();
    checkprintram();
    autopanic();
    printcol("\nTOUR, by Turlum25, September, October, November 2026\n", VGA_COLOR_GREEN);
    printcolvar((uint32_t)"Version detected: ", 0, VGA_COLOR_GREEN);
    VERSION(VGA_COLOR_YELLOW);
    printcol("\n[warning] You may not be booting off of disk.\n          Your data may be lost after rebooting.\n", VGA_COLOR_RED);

    init_tmpfs(); // Starts temporary file system. Needed for disk mounting
                  // Re: Nov 22 2026: Not needed as of now. Microkernels are superior btw

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
    fileinit();

    printcol("\nCPU Generation: ", VGA_COLOR_WHITE);
    printcol(CPUType, VGA_COLOR_GREEN);
    printcol("\nTotal memory (KB): ", VGA_COLOR_WHITE);
    printcolvar(TotalRAM, 1, VGA_COLOR_GREEN);
    printcol("\nTaken memory (KB): ", VGA_COLOR_WHITE);
    printcolvar(TakenRAM, 1, VGA_COLOR_GREEN);
    print("\n");


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
    
    print("\n");
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



// list command!
void list(const char *command) {
    const char *arg = command + 4;
    char path[256];

    while (arg && (*arg == ' ' || *arg == '\t')) arg++;

    if (!arg || arg[0] == '\0') {
        strcpy(path, current_directory);
    } else if (arg[0] == '/') {
        strcpy(path, arg);
    } else {
        strcpy(path, current_directory);
        size_t len = strlen(path);
        if (len > 1 && path[len - 1] != '/') {
            path[len++] = '/';
            path[len] = '\0';
        }
        strcpy(path + len, arg);
    }

    size_t plen = strlen(path);
    if (plen > 1 && path[plen - 1] == '/') {
        path[plen - 1] = '\0';
    }

    ext4_dir d;
    if (ext4_dir_open(&d, path) != EOK) {
        printcol("Directory not found\n", VGA_COLOR_RED);
        return;
    }

    const ext4_direntry *de;
    while ((de = ext4_dir_entry_next(&d)) != NULL) {
        const char *name = (const char *)de->name;
        if (name[0] == '.') {
            continue;
        }

        char full_item[512];
        strcpy(full_item, path);
        size_t flen = strlen(full_item);
        if (flen > 1 && full_item[flen - 1] != '/') {
            full_item[flen++] = '/';
            full_item[flen] = '\0';
        }
        strcpy(full_item + flen, name);

        if (de->inode_type == EXT4_DE_DIR) {
            printcol("(dir) ", VGA_LIGHT_CYAN);
            printcol(name, VGA_LIGHT_CYAN);
            printcol(" (4096B)\n", VGA_LIGHT_GREY);
        } else {
            uint64_t fsz = 0;
            ext4_file f;
            if (ext4_fopen(&f, full_item, "rb") == EOK) {
                fsz = ext4_fsize(&f);
                ext4_fclose(&f);
            }
            printcol("(file) ", VGA_COLOR_WHITE);
            printcol(name, VGA_COLOR_GREEN);
            printcol(" (", VGA_LIGHT_GREY);
            printcolvar(fsz, 1, VGA_LIGHT_GREY);
            printcol("B)\n", VGA_LIGHT_GREY);
        }
    }

    ext4_dir_close(&d);
}

static int shell_next_token(const char **cursor, char *out, size_t max_len) {
    const char *p = *cursor;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0') {
        *cursor = p;
        return 0;
    }
    size_t i = 0;
    while (*p && *p != ' ' && *p != '\t') {
        if (i + 1 >= max_len) return -1;
        out[i++] = *p++;
    }
    out[i] = '\0';
    *cursor = p;
    return 1;
}


// chdir.
void chdir(const char *command) {
    const char *path = command + 5;

    while (*path == ' ' || *path == '\t') {
        ++path;
    }

    char token[256];
    char resolved[256];

    if (!*path) {
        path = "/";
    }

    const char *cursor = path;

    if (shell_next_token(&cursor, token, sizeof(token)) != 1) {
        printcol("Usage: chdir [directory]\n", VGA_COLOR_YELLOW);
        return;
    }

    while (*cursor == ' ' || *cursor == '\t') {
        ++cursor;
    }

    if (*cursor != '\0') {
        printcol("Usage: chdir [directory] (Too many arguments)\n",
                 VGA_COLOR_YELLOW);
        return;
    }

    if (token[0] == '/') {
        strcpy(resolved, token);
    } else {
        strcpy(resolved, current_directory);
        size_t len = strlen(resolved);
        if (len > 1 && resolved[len - 1] != '/') {
            resolved[len++] = '/';
            resolved[len] = '\0';
        }
        strcpy(resolved + len, token);
    }

    size_t rlen = strlen(resolved);
    if (rlen > 1 && resolved[rlen - 1] == '/') {
        resolved[rlen - 1] = '\0';
    }

    ext4_dir d;
    if (!mounted || !ext4_mounted || ext4_dir_open(&d, resolved) != EOK) {
        printcol("Directory not found\n", VGA_COLOR_RED);
        return;
    }
    ext4_dir_close(&d);

    uint32_t i = 0;
    while (resolved[i] && i < sizeof(current_directory) - 1) {
        current_directory[i] = resolved[i];
        ++i;
    }
    current_directory[i] = '\0';
}


void readFile(const char *command) {
    const char *cursor = command;
    if (strncmp(cursor, "read", 4) == 0) {
        cursor += 4;
    }

    char filename[256];
    char extra[256];

    if (shell_next_token(&cursor, filename, sizeof(filename)) != 1 ||
        shell_next_token(&cursor, extra, sizeof(extra)) != 0) {
        printcol("Usage: read <filename>\n", VGA_COLOR_YELLOW);
        return;
    }

    if (!mounted || !ext4_mounted) {
        printcol("Disk/filesystem is not ready\n", VGA_COLOR_RED);
        return;
    }

    char resolved[256];
    if (filename[0] == '/') {
        strcpy(resolved, filename);
    } else {
        strcpy(resolved, current_directory);
        size_t len = strlen(resolved);
        if (len > 1 && resolved[len - 1] != '/') {
            resolved[len++] = '/';
            resolved[len] = '\0';
        }
        strcpy(resolved + len, filename);
    }

    ext4_file f;
    int rc = ext4_fopen(&f, resolved, "rb");
    if (rc != EOK) {
        printf("ext4_fopen failed (%d) for: %s\n", rc, resolved);
        return;
    }

    char buf[512];
    size_t br = 0;
    while ((rc = ext4_fread(&f, buf, sizeof(buf) - 1, &br)) == EOK && br > 0) {
        buf[br] = '\0';
        print(buf);
    }

    if (rc != EOK) {
        printf("ext4_fread error: %d\n", rc);
    }

    ext4_fclose(&f);
    print("\n");
}

void tcol() {
    if (VGA_COLOR_WHITE != 0x0F) {
        VGA_COLOR_WHITE = 0x0F;
    } else {VGA_COLOR_WHITE = 0x00;} 
}






void bcol(const char *character) { // Changes background color
    const char *pointer = character + 5; // "bcol "

    int black = 0;
    int blue = 0;
    int green = 0;
    int cyan = 0;
    int red = 0;
    int magenta = 0;
    int brown = 0;
    int orange = 0;
    int light_grey = 0;
    int dark_grey = 0;
    int light_blue = 0;
    int light_green = 0;
    int light_cyan = 0;
    int light_red = 0;
    int light_magenta = 0;
    int yellow = 0;
    int white = 0;

    str(pointer, "black", black);
    str(pointer, "blue", blue);
    str(pointer, "green", green);
    str(pointer, "cyan", cyan);
    str(pointer, "red", red);
    str(pointer, "magenta", magenta);
    str(pointer, "brown", brown);
    str(pointer, "orange", orange);
    str(pointer, "light grey", light_grey);
    str(pointer, "dark grey", dark_grey);
    str(pointer, "light blue", light_blue);
    str(pointer, "light green", light_green);
    str(pointer, "light cyan", light_cyan);
    str(pointer, "light red", light_red);
    str(pointer, "light magenta", light_magenta);
    str(pointer, "yellow", yellow);
    str(pointer, "white", white);
    
    if (black != 0) {clear_screen(VGA_BLACK);}
    else if (blue != 0) {clear_screen(VGA_BLUE);}
    else if (green != 0) {clear_screen(VGA_GREEN);}
    else if (cyan != 0) {clear_screen(VGA_CYAN);}
    else if (red != 0) {clear_screen(VGA_RED);}
    else if (magenta != 0) {clear_screen(VGA_MAGENTA);}
    else if (brown != 0) {clear_screen(VGA_BROWN);}
    else if (orange != 0) {clear_screen(VGA_BROWN);}
    else if (light_grey != 0) {clear_screen(VGA_LIGHT_GREY);}
    else if (dark_grey != 0) {clear_screen(VGA_DARK_GREY);}
    else if (light_blue != 0) {clear_screen(VGA_LIGHT_BLUE);}
    else if (light_green != 0) {clear_screen(VGA_LIGHT_GREEN);}
    else if (light_cyan != 0) {clear_screen(VGA_LIGHT_CYAN);}
    else if (light_red != 0) {clear_screen(VGA_LIGHT_RED);}
    else if (light_magenta != 0) {clear_screen(VGA_LIGHT_MAGENTA);}
    else if (yellow != 0) {clear_screen(VGA_YELLOW);}
    else if (white != 0) {clear_screen(VGA_WHITE); tcol();}
    else if (pointer[0] == '\0') {clear_screen(VGA_BLACK);
        printcol("Usage: bcol <color>\n", VGA_YELLOW);
        printcol("Colors: black, blue, green, cyan,\nred, magenta, brown, orange, \nlight grey, dark grey, light blue, light green, \nlight cyan, light red, light magenta, yellow, white\n", VGA_LIGHT_CYAN);
    }
}

void coffee() {
    printcol("[err] I'm a teapot\n      Returned code 418\n", VGA_LIGHT_CYAN);
    return;
}

void bob() {printcol("bob\n", VGA_LIGHT_CYAN);}

void knad(const char *character) { // Know About (The) Details
    const char *pointer = character + 5; // "knad "

    if (pointer[0] == '-') {
        if (pointer[1] == 'a') { // knad -a (-all)
            print(sysname);
            printf(" %s", branch);
            print(" ");
            VERSION(VGA_WHITE);
            printf(" %s", build);
        }
        else if (pointer[1] == 'b') { // Build, branch
            print(build);
            printf(" %s", branch);
        }
        else if (pointer[1] == 'n') { // Base name print
            print(sysname);
        }
        else if (pointer[1] == 'v') { // Base version print
            print(vernum);
        }

        else if(pointer[1] == 'h') { // Help
            printcol("knad [-a][-b][-n][-v]\n", VGA_LIGHT_CYAN);
        }
    }
    else {print(vernum);}
}



void pxl() {
    char x[32];
    char y[32];
    char c[32];
    char sx[999];
    char sy[999];

    input("x: ", x, x + sizeof(x));
    input("size x :", sx, sx + sizeof(sx));

    input("y: ", y, y + sizeof(y));
    input("size y:", sy, sy + sizeof(sy));
    input("color: ", c, c + sizeof(c));

    int sizex = atoi(sx);
    int sizey = atoi(sy);
    int xint = atoi(x);
    int yint = atoi(y);
    uint32_t color = RGB_WHITE; // Default obv

    if (strings_match(c, "black")) color = RGB_BLACK;
    else if (strings_match(c, "blue")) color = RGB_BLUE;
    else if (strings_match(c, "green")) color = RGB_GREEN;
    else if (strings_match(c, "cyan")) color = RGB_CYAN;
    else if (strings_match(c, "red")) color = RGB_RED;
    else if (strings_match(c, "magenta")) color = RGB_MAGENTA;
    else if (strings_match(c, "brown")) color = RGB_BROWN;
    else if (strings_match(c, "lightgrey")) color = RGB_LIGHT_GREY;
    else if (strings_match(c, "darkgrey")) color = RGB_DARK_GREY;
    else if (strings_match(c, "lightblue")) color = RGB_LIGHT_BLUE;
    else if (strings_match(c, "lightgreen")) color = RGB_LIGHT_GREEN;
    else if (strings_match(c, "lightcyan")) color = RGB_LIGHT_CYAN;
    else if (strings_match(c, "lightred")) color = RGB_LIGHT_RED;
    else if (strings_match(c, "lightmagenta")) color = RGB_LIGHT_MAGENTA;
    else if (strings_match(c, "yellow")) color = RGB_YELLOW;
    else if (strings_match(c, "white")) color = RGB_WHITE;

    for (int py = 0; py < sizey; py++) {
        for (int px = 0; px < sizex; px++) {
            put_pixel(xint + px, yint + py, color);
        }
    }
}



// Why do I not comment at all in super fucking duper complicated functions? 


// Dear spookinate from Discord: 
// Prepare to be scared. I added a shell to the fucking kernel
// HAHAHAHA I BROKE THE LAWS OF FUCKING KERNEL DEVELOPMENT
// MIJAJAJAAFBFUIHJGFUIBFDUYFEUIYHFGFUYTEGFBIHDGFUE
void kernel_main(void) __attribute__((noreturn, used));

void kernel_main(void) {
    clear_screen(VGA_BLACK);
    extern uint32_t cursor_pos;

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
    static uint8_t TSTPNC = 0;
    static uint8_t REGDUMP = 0;
    static uint8_t LIST = 0;
    static uint8_t READ = 0;
    static uint8_t CHDIR = 0;
    static uint8_t RUN = 0;
    static uint8_t BCOL = 0; 
    static uint8_t TCOL = 0;
    static uint8_t BOB = 0;
    static uint8_t CFE = 0;
    static uint8_t KNAD = 0;
    static uint8_t PXL = 0;

    initproc();
    printcol("\nType 'help' for a list of commands.", VGA_COLOR_GREEN);
    print("\n");
    while (1) {

        printcol("{ ", VGA_COLOR_WHITE);
        printcol(User, VGA_COLOR_WHITE);
        printcol(" * ", VGA_COLOR_WHITE); 
        printcol(Host, VGA_COLOR_WHITE);
        print(" ");
        printcol(current_directory, VGA_COLOR_WHITE);
        printcol(" }", VGA_COLOR_WHITE);
        

        input(" % ", buff, buff + 1024);

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
        str(buff, "panic", TSTPNC);
        str(buff, "regdump", REGDUMP);
        LIST = strings_match(buff, "list") || StringPrefixMatch(buff, "list ");        
        READ = StringPrefixMatch(buff, "read ");
        CHDIR = strings_match(buff, "chdir") || StringPrefixMatch(buff, "chdir ");
        RUN = strings_match(buff, "run") || StringPrefixMatch(buff, "run ");
        RUN = strings_match(buff, "run") || StringPrefixMatch(buff, "run ");
        BCOL = strings_match(buff, "bcol") || StringPrefixMatch(buff, "bcol ");
        str(buff, "tcol", TCOL);
        str(buff, "bob", BOB);
        str(buff, "coffee", CFE);
        KNAD = strings_match(buff, "knad") || StringPrefixMatch(buff, "knad ");
        str(buff, "pxl", PXL);

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
        else if (TSTPNC != 0) { panic("Debug panic", 123); }
        else if (REGDUMP != 0) { DumpRegisters(); }
        else if (LIST != 0) { list(buff); }
        else if (strcmp(buff, "read") == 0 || strncmp(buff, "read ", 5) == 0) {readFile(buff);}
        else if (CHDIR != 0) { chdir(buff); }
        else if (RUN != 0) { run(buff); }
        else if (BCOL != 0) {bcol(buff);}
        else if (TCOL != 0) {tcol();}
        else if (BOB != 0) {bob();}
        else if (CFE != 0) {coffee();}
        else if (KNAD != 0) {knad(buff);}
        else if (PXL != 0) {pxl();}

        else if (buff[0] == '\0') {print("\n");}
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
        READ = 0;
        LIST = 0;
        CHDIR = 0;
        RUN = 0;
        PXL = 0; // Added this later
        BOB = 0; // This
        CFE = 0; // Also this
        KNAD = 0; // This too. Idk how it worked without these flag resetters :sob:
        // REBOOT = 0; Not needed. This value resets back to 0 after reboot.
        // TSTPNC not needed.
        
        if (cursor_pos != 0) {
            print("\n");
        }
        print("\n");
    }

   // End of main kernel stuff.
}
