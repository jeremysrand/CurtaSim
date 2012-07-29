//
//    Author: Jeremy Rand
//      Date: July 20, 2012
//
// This is the implementation for the Curta emulator model.
//


#include <stddef.h>

#include "curtaModel.h"


tDigit operand[NUM_OPERAND_DIGITS];
tDigit result[NUM_RESULT_DIGITS];
tDigit counter[NUM_COUNTER_DIGITS];

tDigitPos basePos;

tDigitPos selectedOperand;


static tOperandDigitChange operandDigitCallback = NULL;
static tSelectedOperandChange selectedOperandCallback = NULL;


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


void initDevice(tOperandDigitChange callback1, tSelectedOperandChange callback2)
{
    tDigitPos pos;

    operandDigitCallback = callback1;
    selectedOperandCallback = callback2;

    clearDevice();
    basePos = 0;
    selectedOperand = 0;

    for (pos = 0; pos < NUM_OPERAND_DIGITS; pos++) {
        result[pos] = 0;
    }
}
    

void incOperandPos(tDigitPos pos)
{
    tDigit oldValue;

    if (!IS_VALID_OPERAND_POS(pos))
        return;

    if (operand[pos] == 9)
        return;

    oldValue = operand[pos];
    operand[pos]++;

    if (operandDigitCallback != NULL)
        operandDigitCallback(pos, oldValue, operand[pos]);
}


void decOperandPos(tDigitPos pos)
{
    tDigit oldValue;

    if (!IS_VALID_OPERAND_POS(pos))
        return;

    if (operand[pos] == 0)
        return;

    oldValue = operand[pos];
    operand[pos]--;

    if (operandDigitCallback != NULL)
        operandDigitCallback(pos, oldValue, operand[pos]);
}


void shiftOperandPos(bool left)
{
    tDigitPos newPos = selectedOperand;
    tDigitPos oldPos = selectedOperand;

    if (left) {
        newPos++;
        if (!IS_VALID_OPERAND_POS(newPos)) {
            return;
        }
    } else {
        newPos--;
        if (!IS_VALID_OPERAND_POS(newPos)) {
            return;
        }
    }

    selectedOperand = newPos;
    if (selectedOperandCallback != NULL) {
        selectedOperandCallback(oldPos);
        selectedOperandCallback(newPos);
    }
}

void shiftResultPos(bool left)
{
    tDigitPos newPos = basePos;
    if (left) {
        newPos++;
        if (!IS_VALID_BASE_POS(newPos))
            return;
    } else {
        newPos--;
        if (!IS_VALID_BASE_POS(newPos))
            return;
    }
    basePos = newPos;
}


static void subOperation(tDigit *digits, tDigitPos pos, tDigitPos maxPos, tDigit toSub)
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


static void addOperation(tDigit *digits, tDigitPos pos, tDigitPos maxPos, tDigit toAdd)
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
        subOperation(counter, basePos, NUM_COUNTER_DIGITS, 1);
        for (pos = 0; pos < NUM_OPERAND_DIGITS; pos++) {
            subOperation(result, basePos+pos, NUM_RESULT_DIGITS, operand[pos]);
        }
    } else {
        addOperation(counter, basePos, NUM_COUNTER_DIGITS, 1);
        for (pos = 0; pos < NUM_OPERAND_DIGITS; pos++) {
            addOperation(result, basePos+pos, NUM_RESULT_DIGITS, operand[pos]);
        }
    }
}

