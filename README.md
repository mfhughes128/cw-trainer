# cw-trainer
Morse code trainer Arduino project

From: The article "Arduino CW Trainer" by Tom Lewis, N4TL in the September 2016 QST magazine.

The original sketch has been reworked to use the buttons on the AdaFruit LCD shield and a set of menus to replace the PS2 keyboard used in the orginal sketch. The LCD shield has five buttons which are referred to as 'up', 'down', 'left', 'right' and 'select'.

On power up the first menu displayed is - 
    N4TL CW Trainer
    >Start Trainer
    >Start Decoder
    >Set Preferences
    >Run PARIS Test
Only one line of the menu can be shown at a time so the 'up' and 'down' buttons scroll through the options. In this menu the 'left' and 'right' buttons do nothing. The 'select' button starts the option currently showing.

The options are -
    - 'Start Trainer' begins sending characters and listening for the trainee's response. The characters sent are shown on the top line of the LCD. The characters received from the trainee are shown on the bottom line. If the received string does not match what was sent, the LCD backlight turns red and the same string is sent again. Pressing any of the buttons will return to the main menu.
    - 'Start Decoder' begins listening for Morse characters and displays them on the lower line of the display. The upper line is reserved for now. Pressing any button will return to the main menu.
    - 'Set Preferences' opens a second menu to set the trainer parameters, such as sending speed, number of characters to send and the character set to send. This menu is described further below.
    - 'Run Paris Test' starts a loop which just sends the word "PARIS" once a second, using the currently set parameters for speed and so on. Pressing any button will return to the main menu.

Selecting 'Set Preferences' from the main menu opens another menu used to set training parameters. The name of a parameter will be displayed on the top line of the LCD and the parameter's current value will be displayed on the bottom line. The 'up' and 'down' buttons scroll through the parameters and the 'left and 'right' buttons decrement or increment  the value. The 'select' button saves the current parameter values to EEprom and returns to the main menu.

The parameters are -
    "Code Group Size:"         - The number of characters to send [1 to 15]
    "Character Delay:"         - The delay between characters in a group in 0.01 second increments [0 - 30]
    "Code Speed:"              - Speed in wpmC [20 - 30]
    "Character Set:"           - The character set to pick from when sending
      [  1=alpha,                  - Alpha characters in alphabetical order
         2=numbers,                - Just the numbers
         3=special chars.,         - .,? and etc.
         4=all alphabetic,         - All of the above
         5=Koch order,             - Characters in Koch order
         6=same as 5 for now ]     - Reserved for future expansion
    "Koch Number:"                 - How deep to go into the Koch table [1 - 40]
    "Skip Characters"              - Characters to skip at the beginning of the Koch table.
    "Out:0=key,1=spk"              - Selects the output pin and analog or digital output mode.

The values of these parameters are restored from EEPROM on power up.
