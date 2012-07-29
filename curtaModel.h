//
//    Author: Jeremy Rand
//      Date: July 20, 2012
//
// This is the interface for the Curta emulator model.
//


#ifndef _CURTAMODEL_H
#define _CURTAMODEL_H

#define NUM_OPERAND_DIGITS 11
#define NUM_RESULT_DIGITS 15
#define NUM_COUNTER_DIGITS 8

typedef int8_t tDigit;
typedef int8_t tDigitPos;

extern tDigit operand[NUM_OPERAND_DIGITS];
extern tDigit result[NUM_RESULT_DIGITS];
extern tDigit counter[NUM_COUNTER_DIGITS];

#define IS_VALID_OPERAND_POS(pos) (((pos) >= 0) && ((pos) < NUM_OPERAND_DIGITS))
#define IS_VALID_RESULT_POS(pos) (((pos) >= 0) && ((pos) < NUM_RESULT_DIGITS))
#define IS_VALID_COUNTER_POS(pos) (((pos) >= 0) && ((pos) < NUM_COUNTER_DIGITS))

#define GET_OPERAND_DIGIT(pos) (operand[(pos)])
#define GET_RESULT_DIGIT(pos) (result[(pos)])
#define GET_COUNTER_DIGIT(pos) (counter[(pos)])

// Ranges from 0 to 8
#define BASE_POS_MIN 0
#define BASE_POS_MAX 8
extern tDigitPos basePos;

#define IS_VALID_BASE_POS(pos) (((pos) >= BASE_POS_MIN) && ((pos) < BASE_POS_MAX))

extern tDigitPos selectedOperand;
#define IS_SELECTED_OPERAND(pos) ((pos) == selectedOperand)


typedef void (*tOperandDigitChange)(tDigitPos pos, tDigit oldValue, tDigit newValue);
typedef void (*tSelectedOperandChange)(tDigitPos pos);

extern void clearDevice(void);
extern void initDevice(tOperandDigitChange callback1, tSelectedOperandChange callback2);

extern void incOperandPos(tDigitPos pos);
extern void decOperandPos(tDigitPos pos);

extern void shiftOperandPos(bool left);

extern void shiftResultPos(bool left);

extern void crank(bool isSubtract);

#endif
