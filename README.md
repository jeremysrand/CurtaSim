CurtaSim
========

This is a simulation of the [Curta mechanical calculator](https://en.wikipedia.org/wiki/Curta) written for the Apple //.

![CurtaSim Screenshot](/curta.png "CurtaSim Screenshot")

[Download a disk image](https://github.com/jeremysrand/CurtaSim/releases/download/1.1/curta.dsk)

A Curta looks somewhat like a pepper grinder.  To replace the crank, this simulation uses a joystick.

The primary interaction is performed by "cranking" the joystick through 360 degrees.  To perform a crank, start with the joystick centered.  Pull the joystick towards you and then crank around a circle clockwise until it is again pulled towards you.  Then release the joystick back to the centre position.

The operand is the set of digits at the top of the screen.  The sliders control the operand.  The result is at the bottom of the screen.  The counter is a multiplicand.  Below the result is a carat that points to a digit.  This is the same as the carriage position on a real curta.

The joystick operations are:
   * Left/Right - Select a digit in the operand.
   * Up/Down - Change the selected digit in the operand.
   * Left/Right with Button 0 Down - Change the carriage position.
   * Crank - Add the operand times the carriage position to the result.  If the carriage position is pointing to the hundreds position, then 100 times the operand is added to the result.
   * Crank with Button 0 Down - Subtract the operand times the carriage postion from the result.
   * Crank with Button 1 Down - Clear the result and counter values.

These keyboard commands can be used:
   * Q - Quit the simulation.
   * H - Print help information.

Imagine you want to multiply 123 by 990.  Perform a clear operation if result is not zero.  Use Left/Right/Up/Down operations to put 123 in the operand.  Hold button 0 down and perform Left/Right moves to get the carriage pointing to the thousands position in the result.

Now do an Add operation.  The calculator multiplies the operand by 1000 from the carriage position which results in 123,000 which is then added to the result.  The result was zero so now the result is 123,000.  Note the counter is 1000.  The operand (123) multiplied by the counter (1000) is the result (123,000).

Move the carriage to the tens position.  Do a Subtract operation.  The calculate multiplies the operand by 10 from the carriage psition which results in 1230 which is then subtracted from the result.  The result was 123,000 so now the result is 121,770.  Note that the counter is now 990.  Again, the operand (123) multiplied by the counter (990) is the result (121,770).

If you want more information about how to use a Curta, check out [these Curta manuals](http://www.curtamania.com/curta/code/curta_manuals.html).
