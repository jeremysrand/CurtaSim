//
//    Author: Jeremy Rand
//      Date: July 20, 2012
//
// This is the implementation for the Curta emulator UI.
//


#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <conio.h>
#include <apple2.h>
#include <joystick.h>
#include <tgi.h>
#include <tgi/tgi-mode.h>

#include "curtaModel.h"
#include "joystick.h"
#include "curtaUI.h"


// Extern symbols for graphics drivers
extern char a2e_hi;


typedef int8_t tAction;

#define ACTION_OPERAND_SHIFT_LEFT 0
#define ACTION_OPERAND_SHIFT_RIGHT 1
#define ACTION_OPERAND_INC 2
#define ACTION_OPERAND_DEC 3
#define ACTION_RESULT_SHIFT_LEFT 4
#define ACTION_RESULT_SHIFT_RIGHT 5
#define ACTION_ADD 6
#define ACTION_SUBTRACT 7
#define ACTION_CLEAR 8
#define ACTION_NULL 9


#define OPERAND_COLOR COLOR_WHITE
#define OPERAND_OFFSET 4
#define SLIDER_BAR_COLOR COLOR_BLUE
#define SELECTED_SLIDER_BAR_COLOR COLOR_VIOLET
#define SLIDER_COLOR COLOR_ORANGE
#define SELECTED_SLIDER_COLOR COLOR_BLACK
#define SLIDER_X_BORDER 8
#define SLIDER_Y_BORDER 15
#define SLIDER_BAR_WIDTH 12
#define SLIDER_BAR_HEIGHT 130
#define SLIDER_BAR_SPACING 20

#define SLIDER_X_OFFSET 2
#define SLIDER_Y_OFFSET 1
#define SLIDER_WIDTH 8
#define SLIDER_HEIGHT 11
#define SLIDER_Y_SPACING (SLIDER_HEIGHT + (2 * SLIDER_Y_OFFSET))


// Bad news here.  This code in this module uses the defines for the
// number of digits in counter and result except for this display
// buffer and the offsets into it.  The code assumes a specific size.
// It doesn't really have to.  I could build the template string at
// runtime and even statically allocate the buffer based on a formula
// of the sizes, but that just seemed too much work.  Fix it if you
// want to change the width of these fields
static char displayBuffer[] = 
"\n\n"
"Counter:               0 0 0 0 0 0 0 0\n"
" Result: 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
"                                     ^";

#define COUNTER_OFFSET 25
#define RESULT_OFFSET 50
#define BASE_OFFSET 103


static void playSound(int8_t freq, int8_t duration)
{
    while (duration > 0) {
        asm ("STA %w", 0xc030);
        while (freq > 0) {
            freq--;
        }
        duration--;
    }
}


static void updateCounter(void)
{
    tDigitPos pos;
    char *ptr = &(displayBuffer[COUNTER_OFFSET]);

    for(pos = NUM_COUNTER_DIGITS - 1; pos >= 0; pos--) {
        *ptr = GET_COUNTER_DIGIT(pos) + '0';
        ptr+=2;
    }
}


static void updateResult(void)
{
    tDigitPos pos;
    char *ptr = &(displayBuffer[RESULT_OFFSET]);

    for(pos = NUM_RESULT_DIGITS - 1; pos >= 0; pos--) {
        *ptr = GET_RESULT_DIGIT(pos) + '0';
        ptr+=2;
    }

    ptr = &(displayBuffer[BASE_OFFSET]);
    for(pos = BASE_POS_MAX - 1; pos >= BASE_POS_MIN; pos--) {
        if (pos == basePos) {
            *ptr = '^';
        } else {
            *ptr = ' ';
        }
        ptr+=2;
    }
}


static void printState(void)
{
    updateCounter();
    updateResult();
    puts(displayBuffer);
}


static void drawSlider(char xPos, tDigit digit, char color)
{
    tgi_setcolor(color);
    tgi_bar(xPos + SLIDER_X_OFFSET,
            SLIDER_Y_BORDER + SLIDER_Y_OFFSET + (SLIDER_Y_SPACING * digit),
            xPos + SLIDER_X_OFFSET + SLIDER_WIDTH,
            SLIDER_Y_BORDER + SLIDER_Y_OFFSET + (SLIDER_Y_SPACING * digit) + SLIDER_HEIGHT);
}


static void drawBar(char xPos, tDigit digit, bool isSelected)
{
    char barColor = SLIDER_BAR_COLOR;
    char sliderColor = SLIDER_COLOR;

    if (isSelected) {
        barColor = SELECTED_SLIDER_BAR_COLOR;
        sliderColor = SELECTED_SLIDER_COLOR;
    }

    // Draw slider bar
    tgi_setcolor(barColor);
    tgi_bar(xPos, SLIDER_Y_BORDER, xPos + SLIDER_BAR_WIDTH, SLIDER_Y_BORDER + SLIDER_BAR_HEIGHT);

    // Draw slider
    drawSlider(xPos, digit, sliderColor);
}


static void drawText(char xPos, tDigit digit)
{
    char buffer[2];

    // Clear old text
    tgi_setcolor(COLOR_BLACK);
    tgi_bar(xPos, 0, xPos + SLIDER_BAR_WIDTH, SLIDER_Y_BORDER - 1);
    
    // Draw text label
    buffer[0] = digit + '0';
    buffer[1] = '\0';
    tgi_setcolor(OPERAND_COLOR);
    tgi_outtextxy(xPos + OPERAND_OFFSET, 0, buffer);
}


static void changeOperand(tDigitPos pos, tDigit oldValue, tDigit newValue)
{
    char xPos;
    char barColor = SELECTED_SLIDER_BAR_COLOR;
    char sliderColor = SELECTED_SLIDER_COLOR;

    if (!IS_VALID_OPERAND_POS(pos))
        return;

    xPos = SLIDER_X_BORDER + (SLIDER_BAR_SPACING * (NUM_OPERAND_DIGITS - pos - 1));

    playSound(100, 10);
    drawText(xPos, newValue);
    drawSlider(xPos, oldValue, SELECTED_SLIDER_BAR_COLOR);
    drawSlider(xPos, newValue, SELECTED_SLIDER_COLOR);
}


static void changeSelectedOperand(tDigitPos pos)
{
    char xPos;
    tDigit digit;

    if (!IS_VALID_OPERAND_POS(pos))
        return;

    digit = GET_OPERAND_DIGIT(pos);
    xPos = SLIDER_X_BORDER + (SLIDER_BAR_SPACING * (NUM_OPERAND_DIGITS - pos - 1));

    drawBar(xPos, digit, IS_SELECTED_OPERAND(pos));
}


static void drawOperand(tDigitPos pos)
{
    char xPos;
    tDigit digit;

    if (!IS_VALID_OPERAND_POS(pos))
        return;

    digit = GET_OPERAND_DIGIT(pos);
    xPos = SLIDER_X_BORDER + (SLIDER_BAR_SPACING * (NUM_OPERAND_DIGITS - pos - 1));

    drawText(xPos, digit);
    drawBar(xPos, digit, IS_SELECTED_OPERAND(pos));
}


static tAction getNextAction(void)
{
    static bool firstCall = true;
    static tJoyPos oldJoyPos;
    static unsigned int possibleActions = 0xffff;

    tJoyState joyState;
    tAction result = ACTION_NULL;

    if (firstCall) {
        oldJoyPos = JOY_POS_CENTER;
        firstCall = false;
        return result;
    }

    getJoystickState(&joyState);

    if ((joyState.position != JOY_POS_CENTER) && 
        (joyState.position != JOY_POS_DOWN)) {
        if (possibleActions == (1 << ACTION_ADD)) {
            playSound(400, 10);
        }
        if (possibleActions == (1 << ACTION_SUBTRACT)) {
            playSound(100, 10);
        }
    }

    if (joyState.position == oldJoyPos) {
        return result;
    } else if (oldJoyPos == JOY_POS_CENTER) {
        if (joyState.position == JOY_POS_LEFT) {
            if (joyState.button0) {
                possibleActions = (1 << ACTION_RESULT_SHIFT_LEFT);
            } else {
                possibleActions = (1 << ACTION_OPERAND_SHIFT_LEFT);
            }
        } else if (joyState.position == JOY_POS_RIGHT) {
            if (joyState.button0) {
                possibleActions = (1 << ACTION_RESULT_SHIFT_RIGHT);
            } else {
                possibleActions = (1 << ACTION_OPERAND_SHIFT_RIGHT);
            }
        } else if (joyState.position == JOY_POS_UP) {
            possibleActions = (1 << ACTION_OPERAND_DEC);
        } else if (joyState.position == JOY_POS_DOWN) {
            possibleActions = (1 << ACTION_OPERAND_INC);
            if (joyState.button0) {
                possibleActions |= (1 << ACTION_SUBTRACT);
            } else if (joyState.button1) {
                possibleActions |= (1 << ACTION_CLEAR);
            } else {
                possibleActions |= (1 << ACTION_ADD);
            }
        }
    } else {
        if (possibleActions & (1 << ACTION_OPERAND_SHIFT_LEFT)) {
            if (joyState.position == JOY_POS_CENTER) {
                result = ACTION_OPERAND_SHIFT_LEFT;
            } else {
                possibleActions &= (~(1 << ACTION_OPERAND_SHIFT_LEFT));
            }
        }

        if (possibleActions & (1 << ACTION_OPERAND_SHIFT_RIGHT)) {
            if (joyState.position == JOY_POS_CENTER) {
                result = ACTION_OPERAND_SHIFT_RIGHT;
            } else {
                possibleActions &= (~(1 << ACTION_OPERAND_SHIFT_RIGHT));
            }
        }

        if (possibleActions & (1 << ACTION_OPERAND_INC)) {
            if (joyState.position == JOY_POS_CENTER) {
                result = ACTION_OPERAND_INC;
            } else {
                possibleActions &= (~(1 << ACTION_OPERAND_INC));
            }
        }

        if (possibleActions & (1 << ACTION_OPERAND_DEC)) {
            if (joyState.position == JOY_POS_CENTER) {
                result = ACTION_OPERAND_DEC;
            } else {
                possibleActions &= (~(1 << ACTION_OPERAND_DEC));
            }
        }

        if (possibleActions & (1 << ACTION_RESULT_SHIFT_LEFT)) {
            if (joyState.position == JOY_POS_CENTER) {
                result = ACTION_RESULT_SHIFT_LEFT;
            } else {
                possibleActions &= (~(1 << ACTION_RESULT_SHIFT_LEFT));
            }
        }

        if (possibleActions & (1 << ACTION_RESULT_SHIFT_RIGHT)) {
            if (joyState.position == JOY_POS_CENTER) {
                result = ACTION_RESULT_SHIFT_RIGHT;
            } else {
                possibleActions &= (~(1 << ACTION_RESULT_SHIFT_RIGHT));
            }
        }

        if (possibleActions & (1 << ACTION_ADD)) {
            if ((joyState.position == JOY_POS_DOWN) &&
                (oldJoyPos == JOY_POS_DOWN_RIGHT)) {
                result = ACTION_ADD;
                possibleActions = 0;
            } else if (joyState.position != oldJoyPos + 1) {
                possibleActions &= (~(1 << ACTION_ADD));
            }
        } else if (possibleActions & (1 << ACTION_SUBTRACT)) {
            if ((joyState.position == JOY_POS_DOWN) &&
                (oldJoyPos == JOY_POS_DOWN_RIGHT)) {
                result = ACTION_SUBTRACT;
                possibleActions = 0;
            } else if (joyState.position != oldJoyPos + 1) {
                possibleActions &= (~(1 << ACTION_SUBTRACT));
            }
        } else if (possibleActions & (1 << ACTION_CLEAR)) {
            if ((joyState.position == JOY_POS_DOWN) &&
                (oldJoyPos == JOY_POS_DOWN_RIGHT)) {
                result = ACTION_CLEAR;
                possibleActions = 0;
            } else if (joyState.position != oldJoyPos + 1) {
                possibleActions &= (~(1 << ACTION_CLEAR));
            } else if ((joyState.position == JOY_POS_UP_LEFT) ||
                       (joyState.position == JOY_POS_DOWN_RIGHT)) {
                playSound(100, 50);
            }
        }

        if (joyState.position == JOY_POS_CENTER) {
            possibleActions = -1;
        }
    }

    oldJoyPos = joyState.position;
    
    return result;
}


void initUI(void)
{
    // Install drivers
    tDigitPos pos;

    initDevice(changeOperand, changeSelectedOperand);

    tgi_install(&a2e_hi);

    tgi_init();
    tgi_clear();
    for (pos = 0; pos < NUM_OPERAND_DIGITS; pos++) {
        drawOperand(pos);
    }

    // Mixed text and graphics mode
    asm ("STA %w", 0xc053);
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printState();
}


bool processNextEvent(void)
{
    bool timeToQuit = false;

    // Exit on ESC
    if ((kbhit()) &&
        (cgetc() == 27)) {
        timeToQuit = true;
    }
    switch (getNextAction()) {
        case ACTION_NULL:
            break;
        case ACTION_OPERAND_SHIFT_LEFT:
            shiftOperandPos(true);
            break;

        case ACTION_OPERAND_SHIFT_RIGHT:
            shiftOperandPos(false);
            break;

        case ACTION_OPERAND_INC:
            incOperandPos(selectedOperand);
            break;

        case ACTION_OPERAND_DEC:
            decOperandPos(selectedOperand);
            break;

        case ACTION_RESULT_SHIFT_LEFT:
            shiftResultPos(true);
            playSound(100,10);
            printState();
            playSound(200,20);
            break;

        case ACTION_RESULT_SHIFT_RIGHT:
            shiftResultPos(false);
            playSound(100,10);
            printState();
            playSound(200,20);
            break;

        case ACTION_ADD:
            crank(false);
            printState();
            break;

        case ACTION_SUBTRACT:
            crank(true);
            printState();
            break;

        case ACTION_CLEAR:
            clearDevice();
            printState();
            break;
    }
    return timeToQuit;
}
  

void shutdownUI(void)
{   
    // Uninstall drivers
    tgi_uninstall(); 
}
