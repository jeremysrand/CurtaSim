//
//    Author: Jeremy Rand
//      Date: July 20, 2012
//
// This is the interface for the Curta emulator UI.
//


#include <stdbool.h>

#ifndef _CURTAUI_H
#define _CURTAUI_H

extern void printInstructions(void);
extern void initUI(void);
extern void shutdownUI(void);
extern bool processNextEvent(void);

#endif
