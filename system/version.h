/*
    TOUR:

    system/version.h 
    Turlum25, 22 Nov 2026
    =====================
    Contains version stuff.

    Added: 0.06b2

    yummy code! who would vibecode such
    a simple header, don't be a crybaby
*/

#ifndef VER_H
#define VER_H

#include "../kernel.h"

char sysname[65] = "tour"; // TOUR
char vernum[65] = "0.06b2"; // eg 0.06b2
char branch[65] = "branch:main"; // What branch this is in.
char build[65] = "beta";

int VERSION(int COLOR) { // This will never \n
    // Notes: This int just prints the version. Mod however you like, forkers 
    //        COLOR defines the color you wanna put. 
    printcol(vernum, COLOR);
}


#endif 