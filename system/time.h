#ifndef TIME_H
#define TIME_H

#include "../kernel.h"

extern uint32_t CurrentTime;

static inline uint8_t readCMOSRegister(uint8_t reg) {
    uint8_t previous_index = inb(0x70);
    outb(0x70, (previous_index & 0x80) | (reg & 0x7F));
    uint8_t value = inb(0x71);
    outb(0x70, previous_index);
    return value;
}

static inline void checkCMOSTime(void) {
    static uint8_t last_tracked_second = 0xFF;
    uint8_t current_second;
    uint8_t status_b;
    uint8_t raw_second;

    do {
        while (readCMOSRegister(0x0A) & 0x80) {
        }

        raw_second = readCMOSRegister(0x00);
    } while (readCMOSRegister(0x0A) & 0x80);

    status_b = readCMOSRegister(0x0B);

    if (status_b & 0x04) {
        current_second = raw_second;
    } else {
        current_second = (uint8_t)(((raw_second >> 4) * 10) + (raw_second & 0x0F));
    }

    if (current_second >= 60) {
        return;
    }

    if (last_tracked_second == 0xFF) {
        last_tracked_second = current_second;
        return;
    }

    CurrentTime += (current_second + 60 - last_tracked_second) % 60;
    last_tracked_second = current_second;
}

static inline void sleep(uint32_t seconds) {
    if (seconds == 0) return;

    uint32_t start_tick = CurrentTime;

    while ((CurrentTime - start_tick) < seconds) {
        checkCMOSTime();
    }
}

#endif /* TIME_H */