#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <apple2.h>
#include <apple2enh.h>
#include <conio.h>
#include <joystick.h>
#include <tgi.h>
#include <tgi/tgi-mode.h>


// Extern symbols for joystick and graphics drivers
extern char a2e_stdjoy;
extern char a2e_hi;

#define NUM_OPERAND_DIGITS 11
#define NUM_RESULT_DIGITS 15
#define NUM_COUNTER_DIGITS 8

typedef int8_t tDigit;
typedef int8_t tDigitPos;
typedef int8_t tAction;
typedef int8_t tJoyPos;

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

#define JOY_POS_CENTER 0
#define JOY_POS_DOWN 1
#define JOY_POS_DOWN_LEFT 2
#define JOY_POS_LEFT 3
#define JOY_POS_UP_LEFT 4
#define JOY_POS_UP 5
#define JOY_POS_UP_RIGHT 6
#define JOY_POS_RIGHT 7
#define JOY_POS_DOWN_RIGHT 8

tDigit operand[NUM_OPERAND_DIGITS];
tDigit result[NUM_RESULT_DIGITS];
tDigit counter[NUM_COUNTER_DIGITS];

// Ranges from 0 to 8
#define RESULT_POS_MIN 0
#define RESULT_POS_MAX 8
tDigitPos resultPos;
tDigitPos selectedOperand;


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


void drawOperand(tDigitPos pos);


void clearDevice(void)
{
    tDigitPos pos;
    
    for (pos = 0; pos < NUM_RESULT_DIGITS; pos++) {
        result[pos] = 0;
    }
    for (pos = 0; pos < NUM_COUNTER_DIGITS; pos++) {
        counter[pos] = 0;
    }
}


void resetDevice(void)
{
    tDigitPos pos;

    clearDevice();
    resultPos = 0;
    selectedOperand = 0;

    for (pos = 0; pos < NUM_OPERAND_DIGITS; pos++) {
        result[pos] = 0;
    }
}
    

void incOperandPos(tDigitPos pos)
{
    if (pos >= NUM_OPERAND_DIGITS)
        return;

    if (operand[pos] == 9)
        return;

    operand[pos]++;
    drawOperand(pos);
}


void decOperandPos(tDigitPos pos)
{
    if (pos >= NUM_OPERAND_DIGITS)
        return;

    if (operand[pos] == 0)
        return;

    operand[pos]--;
    drawOperand(pos);
}


void shiftOperandPos(bool left)
{
    tDigitPos newPos = selectedOperand;
    tDigitPos oldPos = selectedOperand;

    if (left) {
        newPos++;
        if (newPos >= NUM_OPERAND_DIGITS) {
            return;
        }
    } else {
        newPos--;
        if (newPos < 0) {
            return;
        }
    }

    selectedOperand = newPos;
    drawOperand(oldPos);
    drawOperand(newPos);
}

void shiftResultPos(bool left)
{
    if (left) {
        if (resultPos + 1 >= RESULT_POS_MAX)
            return;
        resultPos++;
    } else {
        if (resultPos <= RESULT_POS_MIN)
            return;
        resultPos--;
    }
}


void subOperation(tDigit *digits, tDigitPos pos, tDigitPos maxPos, tDigit toSub)
{
    if (toSub == 0)
        return;

    if (pos >= maxPos)
        return;

    digits[pos]-=toSub;
    while (digits[pos] < 0) {
        digits[pos]+=10;
        subOperation(digits, pos+1, maxPos, 1);
    }
}


void addOperation(tDigit *digits, tDigitPos pos, tDigitPos maxPos, tDigit toAdd)
{
    if (toAdd == 0)
        return;

    if (pos >= maxPos)
        return;

    digits[pos]+=toAdd;
    while (digits[pos] > 9) {
        digits[pos]-=10;
        addOperation(digits, pos+1, maxPos, 1);
    }
}


void crank(bool isSubtract)
{
    tDigitPos pos;
    if (isSubtract) {
        subOperation(counter, resultPos, NUM_COUNTER_DIGITS, 1);
        for (pos = 0; pos < NUM_OPERAND_DIGITS; pos++) {
            subOperation(result, resultPos+pos, NUM_RESULT_DIGITS, operand[pos]);
        }
    } else {
        addOperation(counter, resultPos, NUM_COUNTER_DIGITS, 1);
        for (pos = 0; pos < NUM_OPERAND_DIGITS; pos++) {
            addOperation(result, resultPos+pos, NUM_RESULT_DIGITS, operand[pos]);
        }
    }
}


void printDigits(char *label, tDigit *digits, tDigitPos maxPos)
{
    tDigitPos pos;

    printf("%7s:", label);
    for(pos = maxPos - 1; pos >= 0; pos--) {
        printf(" %d", digits[pos]);
    }
    printf("\n");
}


void printState(void)
{
    tDigitPos pos;
    printDigits("Counter", counter, NUM_COUNTER_DIGITS);
    printDigits("Result", result, NUM_RESULT_DIGITS);

    printf("      ");
    for(pos = 0; pos < NUM_RESULT_DIGITS - resultPos; pos++) {
        printf("  ");
    }
    printf(" ^\n");
}


void drawOperand(tDigitPos pos)
{
    char xPos;
    char buffer[2];
    tDigit digit;

    if (pos < 0)
        return;
    if (pos >= NUM_OPERAND_DIGITS)
        return;

    xPos = SLIDER_X_BORDER + (SLIDER_BAR_SPACING * (NUM_OPERAND_DIGITS - pos - 1));
    digit = operand[pos];

    // Clear old bar
    tgi_setcolor(COLOR_BLACK);
    tgi_bar(xPos, 0, xPos + SLIDER_BAR_WIDTH, SLIDER_Y_BORDER + SLIDER_BAR_HEIGHT);
    
    // Draw text label
    buffer[0] = digit + '0';
    buffer[1] = '\0';
    tgi_setcolor(OPERAND_COLOR);
    tgi_outtextxy(xPos + OPERAND_OFFSET, 0, buffer);

    // Draw slider bar
    if (selectedOperand == pos) {
        tgi_setcolor(SELECTED_SLIDER_BAR_COLOR);
    } else {
        tgi_setcolor(SLIDER_BAR_COLOR);
    }
    tgi_bar(xPos, SLIDER_Y_BORDER, xPos + SLIDER_BAR_WIDTH, SLIDER_Y_BORDER + SLIDER_BAR_HEIGHT);

    // Draw slider
    if (selectedOperand == pos) {
        tgi_setcolor(SELECTED_SLIDER_COLOR);
    } else {
        tgi_setcolor(SLIDER_COLOR);
    }
    tgi_bar(xPos + SLIDER_X_OFFSET,
            SLIDER_Y_BORDER + SLIDER_Y_OFFSET + (SLIDER_Y_SPACING * digit),
            xPos + SLIDER_X_OFFSET + SLIDER_WIDTH,
            SLIDER_Y_BORDER + SLIDER_Y_OFFSET + (SLIDER_Y_SPACING * digit) + SLIDER_HEIGHT);
}


tJoyPos getJoyPos(char joyState)
{
    if (JOY_BTN_UP(joyState)) {
        if (JOY_BTN_LEFT(joyState)) {
            return JOY_POS_UP_LEFT;
        } else if (JOY_BTN_RIGHT(joyState)) {
            return JOY_POS_UP_RIGHT;
        } else {
            return JOY_POS_UP;
        }
    } else if (JOY_BTN_DOWN(joyState)) {
        if (JOY_BTN_LEFT(joyState)) {
            return JOY_POS_DOWN_LEFT;
        } else if (JOY_BTN_RIGHT(joyState)) {
            return JOY_POS_DOWN_RIGHT;
        } else {
            return JOY_POS_DOWN;
        }
    } else {
        if (JOY_BTN_LEFT(joyState)) {
            return JOY_POS_LEFT;
        } else if (JOY_BTN_RIGHT(joyState)) {
            return JOY_POS_RIGHT;
        } else {
            return JOY_POS_CENTER;
        }
    }
}


tAction getNextAction(void)
{
    static bool firstCall = true;
    static char oldJoyState;
    static tJoyPos oldJoyPos;
    static unsigned int possibleActions = 0xffff;

    char joyState;
    tJoyPos joyPos;
    tAction result = ACTION_NULL;

    if (firstCall) {
        oldJoyState = joy_read(JOY_1);
        oldJoyPos = getJoyPos(oldJoyState);
        firstCall = false;
        return result;
    }

    joyState = joy_read(JOY_1);
    joyPos = getJoyPos(joyState);

    if ((JOY_BTN_FIRE2(oldJoyState) &&
        (!JOY_BTN_FIRE2(joyState)))) {
        oldJoyState = joyState;
        result = ACTION_CLEAR;
    } else if (joyPos == oldJoyPos) {
        oldJoyState = joyState;
        return result;
    } else if (oldJoyPos == JOY_POS_CENTER) {
        if (joyPos == JOY_POS_LEFT) {
            if (JOY_BTN_FIRE(joyState)) {
                possibleActions = (1 << ACTION_RESULT_SHIFT_LEFT);
            } else {
                possibleActions = (1 << ACTION_OPERAND_SHIFT_LEFT);
            }
        } else if (joyPos == JOY_POS_RIGHT) {
            if (JOY_BTN_FIRE(joyState)) {
                possibleActions = (1 << ACTION_RESULT_SHIFT_RIGHT);
            } else {
                possibleActions = (1 << ACTION_OPERAND_SHIFT_RIGHT);
            }
        } else if (joyPos == JOY_POS_UP) {
            possibleActions = (1 << ACTION_OPERAND_DEC);
        } else if (joyPos == JOY_POS_DOWN) {
            possibleActions = (1 << ACTION_OPERAND_INC);
            if (JOY_BTN_FIRE(joyState)) {
                possibleActions |= (1 << ACTION_SUBTRACT);
            } else {
                possibleActions |= (1 << ACTION_ADD);
            }
        }
    } else {
        if (possibleActions & (1 << ACTION_OPERAND_SHIFT_LEFT)) {
            if (joyPos == JOY_POS_CENTER) {
                result = ACTION_OPERAND_SHIFT_LEFT;
            } else {
                possibleActions &= (~(1 << ACTION_OPERAND_SHIFT_LEFT));
            }
        }

        if (possibleActions & (1 << ACTION_OPERAND_SHIFT_RIGHT)) {
            if (joyPos == JOY_POS_CENTER) {
                result = ACTION_OPERAND_SHIFT_RIGHT;
            } else {
                possibleActions &= (~(1 << ACTION_OPERAND_SHIFT_RIGHT));
            }
        }

        if (possibleActions & (1 << ACTION_OPERAND_INC)) {
            if (joyPos == JOY_POS_CENTER) {
                result = ACTION_OPERAND_INC;
            } else {
                possibleActions &= (~(1 << ACTION_OPERAND_INC));
            }
        }

        if (possibleActions & (1 << ACTION_OPERAND_DEC)) {
            if (joyPos == JOY_POS_CENTER) {
                result = ACTION_OPERAND_DEC;
            } else {
                possibleActions &= (~(1 << ACTION_OPERAND_DEC));
            }
        }

        if (possibleActions & (1 << ACTION_RESULT_SHIFT_LEFT)) {
            if (joyPos == JOY_POS_CENTER) {
                result = ACTION_RESULT_SHIFT_LEFT;
            } else {
                possibleActions &= (~(1 << ACTION_RESULT_SHIFT_LEFT));
            }
        }

        if (possibleActions & (1 << ACTION_RESULT_SHIFT_RIGHT)) {
            if (joyPos == JOY_POS_CENTER) {
                result = ACTION_RESULT_SHIFT_RIGHT;
            } else {
                possibleActions &= (~(1 << ACTION_RESULT_SHIFT_RIGHT));
            }
        }

        if (possibleActions & (1 << ACTION_ADD)) {
            if ((joyPos == JOY_POS_DOWN) &&
                (oldJoyPos == JOY_POS_DOWN_RIGHT)) {
                result = ACTION_ADD;
                possibleActions = 0;
            } else if (joyPos != oldJoyPos + 1) {
                possibleActions &= (~(1 << ACTION_ADD));
            }
        } else if (possibleActions & (1 << ACTION_SUBTRACT)) {
            if ((joyPos == JOY_POS_DOWN) &&
                (oldJoyPos == JOY_POS_DOWN_RIGHT)) {
                result = ACTION_SUBTRACT;
                possibleActions = 0;
            } else if (joyPos != oldJoyPos + 1) {
                possibleActions &= (~(1 << ACTION_SUBTRACT));
            }
        }

        if (joyPos == JOY_POS_CENTER) {
            possibleActions = -1;
        }
    }

    oldJoyPos = joyPos;
    oldJoyState = joyState;
    
    return result;
}


void graphicalInterface(void)
{
    // Install drivers
    tDigitPos pos;

    joy_install(&a2e_stdjoy);
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

    while (true) {
        if ((kbhit()) &&
            (cgetc() == ' ')) {
            break;
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
                printState();
                break;

            case ACTION_RESULT_SHIFT_RIGHT:
                shiftResultPos(false);
                printState();
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
    }
   
    // Uninstall drivers
    tgi_uninstall(); 
    joy_uninstall();
}


int main(void)
{
    graphicalInterface();
    return 0;
}
