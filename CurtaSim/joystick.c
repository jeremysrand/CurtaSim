//
//    Author: Jeremy Rand
//      Date: July 20, 2012
//
// This is the implementation for the Curta emulator UI.
//


#include <stdio.h>

#include "joystick.h"


#define PREAD 0xFB1E
#define ROM_SWITCH 0xC082
#define RAM_SWITCH 0xC080
#define BTN0  0xC061
#define BTN1  0xC062


#define JOYSTICK_CENTER 127
#define JOYSTICK_THRESHOLD 60
#define LOWER_THRESHOLD (JOYSTICK_CENTER - JOYSTICK_THRESHOLD)
#define UPPER_THRESHOLD (JOYSTICK_CENTER + JOYSTICK_THRESHOLD)


static uint8_t joystickTemp;


static bool isButtonPressed(int8_t buttonNum)
{
    if (buttonNum) {
        __asm__ volatile("LDA %w", BTN1);
        __asm__ volatile("STA %v", joystickTemp);
    } else {
        __asm__ volatile("LDA %w", BTN0);
        __asm__ volatile("STA %v", joystickTemp);
    }
    return ((joystickTemp > 127) ? true : false);
}


static uint8_t joystickLeftRight(void)
{
    __asm__ volatile("BIT %w", ROM_SWITCH);
    __asm__ volatile("LDX #0");
    __asm__ volatile("JSR %w", PREAD);
    __asm__ volatile("STY %v", joystickTemp);
    __asm__ volatile("BIT %w", RAM_SWITCH);
    return joystickTemp;
}


static uint8_t joystickUpDown(void)
{
    __asm__ volatile("BIT %w", ROM_SWITCH);
    __asm__ volatile("LDX #1");
    __asm__ volatile("JSR %w", PREAD);
    __asm__ volatile("STY %v", joystickTemp);
    __asm__ volatile("BIT %w", RAM_SWITCH);
    return joystickTemp;
}


void getJoystickState(tJoyState *state)
{
    static bool readLeftRight = true;
    static uint8_t axisLeftRight = 127;
    static uint8_t axisUpDown = 127;

    tJoyPos pos = JOY_POS_CENTER;

    if (readLeftRight) {
        int temp = joystickLeftRight();  // Get left/right position
        temp *= 3;
        temp /= 4;
        axisLeftRight /= 4;
        axisLeftRight += temp;
        readLeftRight = false;
    } else {
        int temp = joystickUpDown();
        temp *= 3;
        temp /= 4;
        axisUpDown /= 4;
        axisUpDown += temp;
        readLeftRight = true;
    }

    if (axisLeftRight < LOWER_THRESHOLD) {
        pos = JOY_POS_LEFT;
    } else if (axisLeftRight > UPPER_THRESHOLD) {
        pos = JOY_POS_RIGHT;
    }

    state->button0 = isButtonPressed(0);
    state->button1 = isButtonPressed(1);

    if (axisUpDown < LOWER_THRESHOLD) {
        switch (pos) {
            case JOY_POS_LEFT:
                pos = JOY_POS_UP_LEFT;
                break;
            case JOY_POS_RIGHT:
                pos = JOY_POS_UP_RIGHT;
                break;
            default:
                pos = JOY_POS_UP;
                break;
        }
    } else if (axisUpDown > UPPER_THRESHOLD) {
        switch (pos) {
            case JOY_POS_LEFT:
                pos = JOY_POS_DOWN_LEFT;
                break;
            case JOY_POS_RIGHT:
                pos = JOY_POS_DOWN_RIGHT;
                break;
            default:
                pos = JOY_POS_DOWN;
                break;
        }
    }
    state->position = pos;
    // Add in a bit of extra delay also
    for (pos = 0; pos < 100; pos++)
        ;
}
