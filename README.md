# cw-trainer
Morse code trainer Arduino project

From: The article "Arduino CW Trainer" by Tom Lewis, N4TL in the September 2016 QST magazine.

The trainer uses the Koch method. Instead of starting at a slow speed and working up, the trainer begins by sending a few characters at the desired sending rate, of between 20 and 30 wpm. The trainer then listens for the trainee to send the character string back and checks it. After mastering the current set of characters, (i.e. achieving 90% accuracy), the next character from the Koch order can be added to the training set, until the trainee is proficient with the entire alphabet. 

Tom has an excellent set of instructions for assembling the trainer hardware _here_ - http://www.qsl.net/n4tl/arduino/toplevel.htm
I've reworked the original sketch to use the buttons on the AdaFruit LCD shield in place of the PS2 keyboard used in the orginal sketch.

More detailed instructions for how to use this sketch on Tom's trainer are in the Wiki.

Thanks,
Mike Hughes
KC1DMR
