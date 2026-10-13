#include <stdint.h>
#include <stdarg.h>

#ifndef CPU_H
#define CPU_H

extern char CPUType[];

static inline void DetectCPU(void) {
    unsigned int eax, ebx, ecx, edx;

    eax = 0;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(eax));

    char vendor[13];
    *(unsigned int*)&vendor[0] = ebx;
    *(unsigned int*)&vendor[4] = edx;
    *(unsigned int*)&vendor[8] = ecx;
    vendor[12] = '\0';

    int is_intel = 1;
    const char* intel_sig = "GenuineIntel";
    for (int i = 0; i < 12; i++) {
        if (vendor[i] != intel_sig[i]) {
            is_intel = 0;
            break;
        }
    }

    if (!is_intel) {
        int is_amd = 1;
        const char* amd_sig = "AuthenticAMD";
        for (int i = 0; i < 12; i++) {
            if (vendor[i] != amd_sig[i]) {
                is_amd = 0;
                break;
            }
        }
        if (is_amd) {
            const char* amd_str = "AMD Processor";
            int idx = 0;
            while (amd_str[idx] != '\0') {
                CPUType[idx] = amd_str[idx];
                idx++;
            }
            CPUType[idx] = '\0';
            return;
        }
        
        const char* unk_str = "Unknown x86 CPU";
        int idx = 0;
        while (unk_str[idx] != '\0') {
            CPUType[idx] = unk_str[idx];
            idx++;
        }
        CPUType[idx] = '\0';
        return;
    }

    eax = 1;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(eax));

    unsigned int model = (eax >> 4) & 0xF;
    unsigned int family = (eax >> 8) & 0xF;
    unsigned int extended_model = (eax >> 16) & 0xF;

    if (family == 6 || family == 15) {
        model += (extended_model << 4);
    }

    const char* model_name = "Intel Core Processor";

    if (family == 6) {
        switch (model) {
            case 0x1C: case 0x26: case 0x36:
                model_name = "Intel Atom";
                break;
            case 0x2A: case 0x2D:
                model_name = "Intel Sandy Bridge";
                break;
            case 0x3A: case 0x3E:
                model_name = "Intel Ivy Bridge";
                break;
            case 0x3C: case 0x3F: case 0x45: case 0x46:
                model_name = "Intel Haswell";
                break;
            case 0x3D: case 0x47: case 0x4F: case 0x56:
                model_name = "Intel Broadwell";
                break;
            case 0x4E: case 0x5E:
                model_name = "Intel Skylake";
                break;
            default:
                model_name = "Intel P6 / Modern Processor";
                break;
        }
    } else if (family == 15) {
        model_name = "Intel Pentium 4";
    } else if (family == 5) {
        model_name = "Intel Pentium Classic";
    } else if (family == 4) {
        model_name = "Intel i486";
    }

    int i = 0;
    while (model_name[i] != '\0' && i < 63) {
        CPUType[i] = model_name[i];
        i++;
    }
    CPUType[i] = '\0';
}

#endif
