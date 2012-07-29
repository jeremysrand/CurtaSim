//
//    Author: Jeremy Rand
//      Date: July 20, 2012
//
// This is the main entry point for the Curta emulator.
//


#include <stdint.h>
#include <stdbool.h>

#include "curtaUI.h"


int main(void)
{
    bool timeToQuit = false;
    initUI();

    while (!timeToQuit) {
        timeToQuit= processNextEvent();
    }

    shutdownUI();
    return 0;
}
