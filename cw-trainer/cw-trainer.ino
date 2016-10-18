/*
  Arduino Morse code Trainer by Tom Lewis, N4TL.   February 21, 2016
  This sketch sends a few random Morse code characters, then waits for the student to send them back with an external keyer.
  If the same characters are sent back the sketch sends new random characters. If they are wrong the sketch sends the same characters over.
  The PS2 keyboard allows the user to change some operational parameters.
  See the CW Trainer article.docx file for more information.
  73 N4TL

  Uses code by Glen Popiel, KW5GP, found in his book, Arduino for Ham Radio, published by the ARRL.

  Modified and added LCD display by Glen Popiel - KW5GP

  Uses MORSE ENDECODER Library by raronzen
  Copyright (C) 2010, 2012 raron
  GNU GPLv3 license (http://www.gnu.org/licenses)
  Contact: raronzen@gmail.com  (not checked too often..)
  Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/
*/

#include <EEPROM.h>
#include <Morse.h>
#include <avr/pgmspace.h>
#include <MorseEnDecoder.h>  // Morse EnDecoder Library
#include <Wire.h>    //I2C Library
#include <PS2Keyboard.h>

// changed the lcd library
//#include <LiquidCrystal_I2C.h>  // Liquid Crystal I2C Library

#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00

// for the Morse code decoder
#define morseInPin 2
#define Speed_Pin A0

// for the Morse code transmitter
const int beep_pin = 11;  // Pin for CW tone
const int key_pin = 12;   // Pin for CW Key
unsigned int beep_on = 1; // 0 = Key, 1 = Beep
unsigned int key_speed = 25; //  morse keying speed
unsigned int Key_speed_adj = 3; // correction for keying speed

const int lcd_end = 16; // set width of LCD
const int lcd_address = 0x27; // I2C LCD Address
const int lcd_lines = 2; // Number of lines on LCD
String text;  // Variable to hold LCD receive text
String tx_text = ""; // sent text
char cw_rx = ' '; // Variable for incoming Morse character
char cw_rx1; char cw_rx2; char cw_rx3; char cw_rx4;
char cw_rx5; char cw_rx6; char cw_rx7; char cw_rx8;
char cw_rx9; char cw_rx10;
char cw_tx = ' '; // variable for sending a morse character
char cw_tx1; char cw_tx2; char cw_tx3; char cw_tx4;
char cw_tx5; char cw_tx6; char cw_tx7; char cw_tx8;
char cw_tx9; char cw_tx10;
unsigned int expected_number = 1; // expected number of cw characters to be received
unsigned int received_number = 0; // actual number of cw characters received
const int DataPin = 5; // Set PS2 Keyboard Data Pin
const int IRQpin =  3; // Set PS2 Keyboard Clock Pin
char c = ' ';  // character from the keyboard
unsigned int count = 0;
unsigned int firstpass = 0;
unsigned int firstpass2 = 0;
unsigned int i;
unsigned int j;
unsigned int error = 0;
unsigned int num_of_char;  // how many character to use
unsigned int start_point = 0;
unsigned int char_to_use = 1; // defines which character set to send the student.
// 1 = 26 alpha characters
// 2 = numbers
// 3 = miscellaneous characters
// 4 = all characters
// 5 = all 40 in Koch order, follow with how many char to use
// 5 starts at 1 and goes to the number of characters selected.
// 6 = all 40 in Koch order, follow with "staring point".
// 6 starts at the "staring point" set and goes to the number set in F5.

// Morse receive decoder
morseDecoder morseInput(morseInPin, MORSE_KEYER, MORSE_ACTIVE_LOW);  // Define the Morse objects
// Morse sending pin
Morse morse(beep_pin, key_speed, beep_on); //default to beep on pin 11
PS2Keyboard keyboard;  // define the PS2Keyboard

void setup()
{
  Serial.begin(9600);
  keyboard.begin(DataPin, IRQpin);  // Start the Keyboard
  lcd.begin(16, 2);
  lcd.setBacklight(WHITE);
  lcd.print("N4TL CW Trainer");
  lcd.setCursor(0, 1);
  lcd.print("G, D, or F keys");
  Serial.print("N4TL CW Trainer");
  Serial.print("\n");

  // load previous used values from the EEPROM
  // first check to find out if values were
  // previously saved. The first value will be
  // 170 if the values were saved.

  if (170 == EEPROM.read(0))
  {
    expected_number = EEPROM.read(1);
    key_speed   = EEPROM.read(2);
    char_to_use = EEPROM.read(3);
    num_of_char = EEPROM.read(4);
    start_point = EEPROM.read(5);
    beep_on     = EEPROM.read(6);
  }
  Serial.print("Expected number = ");
  Serial.print(expected_number );
  Serial.print("\n");

  Serial.print("key_speed = ");
  Serial.print(key_speed );
  Serial.print("\n");

  Serial.print("character set_to_use = ");
  Serial.print(char_to_use );
  Serial.print("\n");

  Serial.print("koch num_of_char = ");
  Serial.print(num_of_char );
  Serial.print("\n");

  Serial.print("koch start_point = ");
  Serial.print(start_point );
  Serial.print("\n");

  Serial.print("beep_on = ");
  Serial.print(beep_on );
  Serial.print("\n");

  // make sure values are within the correct range
  if (expected_number > 15) expected_number = 15;
  if (key_speed > 30) key_speed = 30;
  if (char_to_use > 6) char_to_use = 6;
  if (start_point > 39 ) start_point = 39;
  if (num_of_char > 40 ) num_of_char = 40;
  if (start_point > num_of_char) start_point = num_of_char;
  if (beep_on > 1) beep_on = 1;

  setup_morse();

}  // End Setup

void loop()
{
  // get users input. Runs until G is pressed
  // once G is pressed this code is not run again.
  while ((c != 'G') && (c != 'D'))
  {
    if (keyboard.available())    // Check the keyboard to see if a key has been pressed
    {

      c = keyboard.read();      // read the key
      c = toupper(c);

      //how many CW char to send before going to receive
      if (c == PS2_LEFTARROW) //less
      {
        expected_number--;
        if (expected_number <= 0) expected_number = 1;
        lcd.setCursor(0, 0);
        lcd.print("num char ");
        lcd.print(expected_number);
        lcd.print("       ");
      }
      else if (c == PS2_RIGHTARROW) //more
      {
        expected_number++;
        if (expected_number > 15) expected_number = 15;
        //lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("num char ");
        lcd.print(expected_number);
        lcd.print("       ");
      }
      // change the CW speed
      else if (c == PS2_UPARROW) //faster
      {
        key_speed++;
        if (key_speed > 30) key_speed = 30;
        // lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("speed ");
        lcd.print(key_speed);
        setup_morse();
      }
      else if (c == PS2_DOWNARROW) // slower
      {
        key_speed--;
        if (key_speed < 20) key_speed = 20;
        // lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("speed ");
        lcd.print(key_speed);
        setup_morse();
      }
      // these select the character set to use.
      else if (c == PS2_F1)
      {
        char_to_use = 1;
        lcd.setCursor(0, 0);
        lcd.print("26 alpha        ");
      }
      else if (c == PS2_F2)
      {
        char_to_use = 2;
        lcd.setCursor(0, 0);
        lcd.print("Numbers         ");
      }
      else if (c == PS2_F3)
      {
        char_to_use = 3;
        lcd.setCursor(0, 0);
        lcd.print(", . ? /         ");
      }
      else if (c == PS2_F4)
      {
        char_to_use = 4;
        lcd.setCursor(0, 0);
        lcd.print("40 chars alphabetical");
      }
      else if (c == PS2_F5)
      {
        char_to_use = 5;
        lcd.setCursor(0, 0);
        lcd.print("40 char Koch order  ");
        lcd.setCursor(0, 1);
        lcd.print("How many chars? ");
        num_of_char = get_number_kb();
        if (num_of_char > 40 ) num_of_char = 40;
        lcd.setCursor(0, 0);
        lcd.print("   ");
        lcd.setCursor(0, 0);
        lcd.print(num_of_char);
        lcd.setCursor(0, 1);
        lcd.print("G to go or F1-F6");
      }
      else if (c == PS2_F6)
      {
        char_to_use = 6;
        lcd.setCursor(0, 0);
        lcd.print("40 char Koch order");
        lcd.setCursor(0, 1);
        lcd.print("Enter start number");
        start_point = get_number_kb();
        if (start_point < 1) start_point = 1;
        if (start_point > num_of_char) start_point = num_of_char;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("start = ");
        lcd.print(start_point);
        lcd.setCursor(0, 1);
        lcd.print("End = ");
        lcd.print(num_of_char);
      }
      else if (c == PS2_F9) // alternate between beep and relay out
      {
        lcd.setCursor(0, 0);
        if (beep_on == 0)
        {
          beep_on = 1;
          setup_morse();
          lcd.print("speaker on      ");
        }
        else
        {
          beep_on = 0;
          setup_morse();
          lcd.print("speaker off     ");
        }
      }
      else if (c == PS2_F10)
      {
        EEPROM.write(0, 170);
        EEPROM.write(1, expected_number);
        EEPROM.write(2, key_speed);
        EEPROM.write(3, char_to_use);
        EEPROM.write(4, num_of_char);
        EEPROM.write(5, start_point);
        EEPROM.write(6, beep_on);
      }
    }
  } // End keyboard while

  //===============================
  // Morse code send and receive
  //===============================
  while (c == 'G') // go
  {
    if (firstpass == 0)
    {
      randomSeed(micros()); // random seed = microseconds since start.
      lcd.clear();
      firstpass++;
    }
    //===============================
    // Start of the sending loop
    //===============================
    // serial print for debug

    Serial.print("Top of the sending loop \n");
    Serial.print("Expected number = ");
    Serial.print(expected_number );
    Serial.print("\n");

    lcd.setCursor(0, 1); // Set the cursor to bottom line, left
    lcd.print("                ");  // clear lower line
    tx_text = ("");

    for (i = 1; i < (expected_number + 1); i++)
    {
      Serial.print(" i = ");
      Serial.print(i);
      Serial.print(" ");
      if (error == 0 )  // if the received characters are correct send new ones.
      {
        lcd.setBacklight(WHITE);
        //lcd.setBacklight(LCD_DISPLAYOFF);
        if (char_to_use == 1) {
          j = random(10, 36);    // alpha
          cw_tx = get_numbered_char(j);
        }
        else if (char_to_use == 2) {
          j = random(0, 10);    // numbers
          cw_tx = get_numbered_char(j);
        }
        else if (char_to_use == 3) {
          j = random(36, 40);    // other
          cw_tx = get_numbered_char(j);
        }
        else if (char_to_use == 4) {
          j = random(0, 40);    // all
          cw_tx = get_numbered_char(j);
        }
        else if (char_to_use == 5) {
          j = random(0, num_of_char); // koch order, start 0
          cw_tx = get_numbered_char_koch(j);
        }
        else if (char_to_use == 6) {
          j = random(start_point - 1, num_of_char); // koch order, start moveable
          cw_tx = get_numbered_char_koch(j);
        }
        Serial.print(" j = ");
        Serial.print(j);
        Serial.print(" ");

        morse.send(cw_tx);   // Send the character
        Serial.print(cw_tx); // debug print
        Serial.print('\n');
        if (i == 1)  cw_tx1 = cw_tx;
        if (i == 2)  cw_tx2 = cw_tx;
        if (i == 3)  cw_tx3 = cw_tx;
        if (i == 4)  cw_tx4 = cw_tx;
        if (i == 5)  cw_tx5 = cw_tx;
        if (i == 6)  cw_tx6 = cw_tx;
        if (i == 7)  cw_tx7 = cw_tx;
        if (i == 8)  cw_tx8 = cw_tx;
        if (i == 9)  cw_tx9 = cw_tx;
        if (i == 10) cw_tx10 = cw_tx;
      }
      else // send the original characters
      {
        if (i == 1) cw_tx = cw_tx1;
        if (i == 2) cw_tx = cw_tx2 ;
        if (i == 3) cw_tx = cw_tx3 ;
        if (i == 4) cw_tx = cw_tx4 ;
        if (i == 5) cw_tx = cw_tx5 ;
        if (i == 6) cw_tx = cw_tx6 ;
        if (i == 7) cw_tx = cw_tx7 ;
        if (i == 8) cw_tx = cw_tx8 ;
        if (i == 9) cw_tx = cw_tx9 ;
        if (i == 10) cw_tx = cw_tx10 ;
        morse.send(cw_tx);   // Send the character
        Serial.print(cw_tx);
        Serial.print('\n');
      }
      // send the sent text to the LCD
      tx_text = tx_text + cw_tx;
      lcd.setCursor(0, 1); // Set the cursor to bottom line, left
      lcd.print(tx_text);  // Display the sent char
    }

    // Now check the students sending
    Serial.print("top of the check Loop \n");
    lcd.setCursor(0, 0); // Set the cursor to top line, left
    lcd.print("                ");
    text = ("");
    error = 0;
    received_number = 0;

    while (received_number < expected_number)
    {
      // Receive a CW characters
      morseInput.decode();  // Decode incoming CW
      // wait for cw char to be received
      if (morseInput.available()) // If there is a character available
      {
        cw_rx = morseInput.read(); // Read the CW character
        if (cw_rx != ' ')          // don't check blank characters
        {
          received_number++;
          if (received_number == 1 && (cw_tx1 != cw_rx)) error = 1;
          if (received_number == 2 && (cw_tx2 != cw_rx)) error = 1;
          if (received_number == 3 && (cw_tx3 != cw_rx)) error = 1;
          if (received_number == 4 && (cw_tx4 != cw_rx)) error = 1;
          if (received_number == 5 && (cw_tx5 != cw_rx)) error = 1;
          if (received_number == 6 && (cw_tx6 != cw_rx)) error = 1;
          if (received_number == 7 && (cw_tx7 != cw_rx)) error = 1;
          if (received_number == 8 && (cw_tx8 != cw_rx)) error = 1;
          if (received_number == 9 && (cw_tx9 != cw_rx)) error = 1;
          if (received_number == 10 && (cw_tx10 != cw_rx)) error = 1;
          if (error == 1 )
          {
            // if there is an error stop receiving
            // code by setting received number high
            received_number = 20;
            lcd.setBacklight(RED);
          }
          Serial.print("received number = ");
          Serial.print(received_number);
          Serial.print('\n');
          // Removed the scrolling LCD display code because
          // it took too long. The next incoming character.
          // would not be decoded correctly
          text = text + cw_rx; // Set up the text to display on the LCD
          lcd.setCursor(0, 0); // Set the cursor to top, left
          lcd.print(text);  // Display the CW text
        }
      }
    }
  }
  //
  // CW decoder only, use this section to check your keyers output to this decoder.
  //
  while (c == 'D')  // decode cw, if D is entered the sketch stays in the short loop.
  {
    if (firstpass2 == 0)
    {
      lcd.clear();
      lcd.setCursor(0, 1);
      firstpass2++;
      lcd.print("CW Decoder");
      lcd.setCursor(0, 0);
    }
    morseInput.decode();  // Decode incoming CW
    if (morseInput.available())  // If there is a character available
    {
      cw_rx = morseInput.read();  // Read the CW character
      Serial.print(cw_rx); // send character to the debug serial monitor
      lcd.print(cw_rx);  // Display the CW character
      count++;
      if (count > 31) count = 0;
      if (count == 16) lcd.setCursor(0, 1);
      if (count == 0) lcd.setCursor(0, 0);
    }
  }
}  // End Main Loop


// Start of the functons.
// changes a random number to an alpha numeric character.
char get_numbered_char(int i) {
  char ch;
  if (i == 0) ch = '0';
  if (i == 1) ch = '1';
  if (i == 2) ch = '2';
  if (i == 3) ch = '3';
  if (i == 4) ch = '4';
  if (i == 5) ch = '5';
  if (i == 6) ch = '6';
  if (i == 7) ch = '7';
  if (i == 8) ch = '8';
  if (i == 9) ch = '9';
  if (i == 10) ch = 'A';
  if (i == 11) ch = 'B';
  if (i == 12) ch = 'C';
  if (i == 13) ch = 'D';
  if (i == 14) ch = 'E';
  if (i == 15) ch = 'F';
  if (i == 16) ch = 'G';
  if (i == 17) ch = 'H';
  if (i == 18) ch = 'I';
  if (i == 19) ch = 'J';
  if (i == 20) ch = 'K';
  if (i == 21) ch = 'L';
  if (i == 22) ch = 'M';
  if (i == 23) ch = 'N';
  if (i == 24) ch = 'O';
  if (i == 25) ch = 'P';
  if (i == 26) ch = 'Q';
  if (i == 27) ch = 'R';
  if (i == 28) ch = 'S';
  if (i == 29) ch = 'T';
  if (i == 30) ch = 'U';
  if (i == 31) ch = 'V';
  if (i == 32) ch = 'W';
  if (i == 33) ch = 'X';
  if (i == 34) ch = 'Y';
  if (i == 35) ch = 'Z';
  if (i == 36) ch = ',';
  if (i == 37) ch = '.';
  if (i == 38) ch = '/';
  if (i == 39) ch = '?';
  Serial.print("First cw function i = ");
  Serial.print(i);
  Serial.print(" ");
  return ch;
}

// changes a random number to an alpha numeric character.
// koch order
char get_numbered_char_koch(int i) {
  char ch;
  if (i == 0) ch = 'K';
  if (i == 1) ch = 'M';
  if (i == 2) ch = 'R';
  if (i == 3) ch = 'S';
  if (i == 4) ch = 'U';
  if (i == 5) ch = 'A';
  if (i == 6) ch = 'P';
  if (i == 7) ch = 'T';
  if (i == 8) ch = 'L';
  if (i == 9) ch = 'O'; // Letter o
  if (i == 10) ch = 'W';
  if (i == 11) ch = 'I';
  if (i == 12) ch = '.';
  if (i == 13) ch = 'N';
  if (i == 14) ch = 'J';
  if (i == 15) ch = 'E';
  if (i == 16) ch = 'F';
  if (i == 17) ch = '0'; // Number zero
  if (i == 18) ch = 'Y';
  if (i == 19) ch = 'V';
  if (i == 20) ch = '.';
  if (i == 21) ch = 'G';
  if (i == 22) ch = '5';
  if (i == 23) ch = '/';
  if (i == 24) ch = 'Q';
  if (i == 25) ch = '9';
  if (i == 26) ch = 'Z';
  if (i == 27) ch = 'H';
  if (i == 28) ch = '3';
  if (i == 29) ch = '8';
  if (i == 30) ch = 'B';
  if (i == 31) ch = '?';
  if (i == 32) ch = '4';
  if (i == 33) ch = '2';
  if (i == 34) ch = '7';
  if (i == 35) ch = 'C';
  if (i == 36) ch = '1';
  if (i == 37) ch = 'D';
  if (i == 38) ch = '6';
  if (i == 39) ch = 'X';
  Serial.print("Second cw function i = ");
  Serial.print(i);
  Serial.print(" ");
  return ch;
}

//get a one or two digit number from the keyboard

int get_number_kb()
{
  static char input[10];
  int i;
  char c = '\0';
  char kb_string[10];
  int  number = 0;
  for (i = 1; i < (10); i++) kb_string[i] = 0; // clear string
  Serial.print("Start kb function ");
  lcd.setCursor(0, 0);
  i = 0;
  while ((c != '\r') && (i < 3))
  {
    if (keyboard.available()) // Check the keyboard to see if a key has been pressed
    {
      c = keyboard.read();      // read the key
      kb_string[i] = c;
      lcd.print(c);
      i++;
    }
  }
  // convert character string to number
  number = 0;
  for (i = 0; kb_string[i] >= '0' && kb_string[i] <= '9'; ++i)
    number = (10 * number) + (kb_string[i] - '0');
  Serial.print(" kb number = ");  Serial.println( number );
  Serial.print(" ");
  return number;
}

// morse code setup
// The speed of the sent characters was measured with MRP40.
// the sent code was slow, an adjustment is necessary to speed it up.
// with an Key_speed_adj = 3, the measured output speed is
// 20 wpm measured to be 19.9 wpm
// 25 wpm measured to be 24.8 wpm
// 30 wpm measured to be 28.7 wpm

void setup_morse()  // set beep on or off, correct pin out and set speed
{
  if (beep_on == 1)
    Morse morse(beep_pin, key_speed + Key_speed_adj , beep_on);
  else
    Morse morse(key_pin, key_speed + Key_speed_adj , beep_on);
}



