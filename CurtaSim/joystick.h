//
//    Author: Jeremy Rand
//      Date: July 29, 2012
//
// This is an interface for the Apple // joystick.  Note that I used to use
// the cc65 joystck interface but found it lacking on real hardware.  It
// worked fine on an emulator but I think it was testing the second axis of
// the joystick too quickly leading to inaccuracies.
//


#include <stdint.h>
#include <stdbool.h>


typedef int8_t tJoyPos;
typedef struct tJoyState {
    tJoyPos position;
    bool button0;
    bool button1;
} tJoyState;


#define JOY_POS_CENTER 0
#define JOY_POS_DOWN 1
#define JOY_POS_DOWN_LEFT 2
#define JOY_POS_LEFT 3
#define JOY_POS_UP_LEFT 4
#define JOY_POS_UP 5
#define JOY_POS_UP_RIGHT 6
#define JOY_POS_RIGHT 7
#define JOY_POS_DOWN_RIGHT 8


void getJoystickState(tJoyState *state);
