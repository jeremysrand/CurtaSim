//
//    Author: Jeremy Rand
//      Date: July 20, 2012
//
// This is the implementation for the Curta emulator UI.
//


#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <conio.h>
#include <apple2.h>
#include <joystick.h>
#include <tgi.h>

#include "curtaModel.h"
#include "joystick.h"
#include "curtaUI.h"
#include "drivers/a2_hires_drv.h"


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


#define OPERAND_COLOR TGI_COLOR_WHITE
#define OPERAND_OFFSET 12
#define SLIDER_BAR_COLOR TGI_COLOR_BLUE
#define SELECTED_SLIDER_BAR_COLOR TGI_COLOR_VIOLET
#define SLIDER_COLOR TGI_COLOR_ORANGE
#define SELECTED_SLIDER_COLOR TGI_COLOR_BLACK
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
        asm volatile ("STA %w", 0xc030);
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
    tgi_setcolor(TGI_COLOR_BLACK);
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


void textMode(void)
{
    clrscr();
    asm volatile ("STA %w", 0xc051);
}


void graphicsMode(void)
{
    asm volatile ("STA %w", 0xc050);
}


void printInstructions(void)
{
    textMode();
    
    //      0000000001111111111222222222233333333334
    //      1234567890123456789012345678901234567890
    printf("                CURTASIM\n"
           "\n"
           "A CURTA IS A MECHANICAL CALCULATOR THAT\n"
           "LOOKS SOMEWHAT LIKE A PEPPER GRINDER.\n"
           "GOOGLE CURTA AND CHECKOUT SOME PICS.\n"
           "TO REPLACE THE CRANK, THIS SIMULATION\n"
           "USES A JOYSTICK.\n"
           "\n"
           "THE PRIMARY INTERACTION IS PERFORMED\n"
           "BY \"CRANKING\" THE JOYSTICK THROUGH\n"
           "360 DEGREES.  TO PERFORM A CRANK, START\n"
           "WITH THE JOYSTICK CENTERED.  PULL THE\n"
           "JOYSTICK TOWARDS YOU AND THEN CRANK\n"
           "AROUND A CIRCLE CLOCKWISE UNTIL IT IS\n"
           "AGAIN PULLED TOWARDS YOU.  THEN RELEASE\n"
           "TO THE CENTRE POSITION.\n"
           "\n"
           "THE OPERAND IS THE SET OF DIGITS AT THE\n"
           "TOP OF THE SCREEN.  THE SLIDERS CONTROL\n"
           "THE OPERAND.  THE RESULT IS AT THE\n"
           "BOTTOM OF THE SCREEN.  THE COUNTER IS A\n"
           "MULTIPLICAND.\n"
           "\n"
           "PRESS ANY KEY FOR MORE INFORMATION");
    cgetc();
    
    clrscr();
    
    //      0000000001111111111222222222233333333334
    //      1234567890123456789012345678901234567890
    printf("                CURTASIM\n"
           "\n"
           "BELOW THE RESULT IS A CARAT THAT POINTS\n"
           "TO A DIGIT.  THIS IS THE SAME AS THE\n"
           "CARRIAGE POSITION ON A REAL CURTA.\n"
           "\n"
           "THE JOYSTICK OPERATIONS ARE:\n"
           "\n"
           " LEFT/RIGHT - SELECT A DIGIT IN THE\n"
           "         OPERAND\n"
           " UP/DOWN - CHANGE THE SELECTED DIGIT IN\n"
           "         THE OPERAND\n"
           " LEFT/RIGHT WITH BUTTON 0 - CHANGE\n"
           "         THE CARRIAGE POSITION\n"
           " CRANK - ADD OPERAND TIMES CARRIAGE\n"
           "         POSITION TO THE RESULT\n"
           " CRANK WITH BUTTON 0 - SUBTRACT OPERAND\n"
           "         TIMES CARRIAGE POSITION FROM\n"
           "         THE RESULT\n"
           " CRANK WITH BUTTON 1 - CLEAR THE RESULT\n"
           "         AND COUNTER\n"
           "\n"
           "PRESS ANY KEY FOR MORE INFORMATION");
    
    cgetc();
    
    clrscr();
    //      0000000001111111111222222222233333333334
    //      1234567890123456789012345678901234567890
    printf("                CURTASIM\n"
           "\n"
           "THESE KEYBOARD COMMANDS CAN BE USED:\n"
           "\n"
           "    Q - QUIT THE SIMULATION\n"
           "    H - PRINT THIS HELP AGAIN\n"
           "\n"
           "IMAGINE YOU WANT TO MULTIPLY 123 BY 990."
           "PERFORM A CLEAR OPERATION IF RESULT IS\n"
           "NOT ZERO.  USE LEFT/RIGHT/UP/DOWN\n"
           "OPERATIONS TO PUT 123 IN THE OPERAND.\n"
           "HOLD BUTTON 0 AND PERFORM LEFT MOVES TO\n"
           "GET THE CARRIAGE POINTING TO THE\n"
           "THOUSANDS POSITION IN THE RESULT.\n"
           "\n"
           "NOW DO AN ADD OPERATION.  THIS FINDS\n"
           "123 * 1000 = 123000 AND ADDS IT TO THE\n"
           "RESULT.  MOVE THE CARRIAGE TO THE TENS\n"
           "POSITION.  DO A SUBTRACT WHICH SUBTRACTS"
           "123 * 10 = 1230 FROM RESULT.  COUNT SAYS"
           "990 AND RESULT HAS THE ANSWER: 121770\n"
           "\n"
           "PRESS ANY KEY TO START THE SIMULATION");
    
    cgetc();
}


void redrawUI(void)
{
    tDigitPos pos;

    graphicsMode();
    tgi_clear();
    for (pos = 0; pos < NUM_OPERAND_DIGITS; pos++) {
        drawOperand(pos);
    }
    
    // Mixed text and graphics mode
    asm volatile ("STA %w", 0xc053);
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printState();
}


void initUI(void)
{
    // Install drivers
    initDevice(changeOperand, changeSelectedOperand);

    tgi_install(&a2_hires_drv);

    tgi_init();
    redrawUI();
}


bool processNextEvent(void)
{
    bool timeToQuit = false;

    // Exit on ESC
    if (kbhit()) {
        switch (cgetc()) {
            case 27:
            case 'Q':
            case 'q':
                timeToQuit = true;
                break;
                
            case 'H':
            case 'h':
                printInstructions();
                redrawUI();
                break;
        }
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
