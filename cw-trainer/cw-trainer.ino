/****************************************
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
*****************************************/

#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <Morse.h>
#include <MorseEnDecoder.h>  // Morse EnDecoder Library
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

// These #defines make it easy to set the LCD backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00

// Application preferences global
//  Char set values:
//    1 = 26 alpha characters
//    2 = numbers
//    3 = punctuation characters
//    4 = all characters in alphabetical order
//    5 = all characters in Koch order. Two prefs set range-
//            KOCH_NUM is number to use
//            KOCH_SKIP is number to skip
//    6 = reserved
#define SAVED_FLG 0   // will be 170 if settings have been saved to EEPROM
#define GROUP_NUM 1   // expected number of cw characters to be received
#define KEY_SPEED 2   // morse keying speed (WPM)
#define CHAR_SET  3   // defines which character set to send the student.
#define KOCH_NUM  4   // how many character to use
#define KOCH_SKIP 5   // characters to skip in the Koch table
#define OUT_MODE  6   // 0 = Key, 1 = Speaker
byte prefs[7];        // Table of preference values

// Main menu strings
const char msg0[] PROGMEM = "N4TL CW Trainer ";
const char msg1[] PROGMEM = "-Start Trainer  ";
const char msg2[] PROGMEM = "-Start Decoder  ";
const char msg3[] PROGMEM = "-Set Preferences";
const char* const main_menu[] PROGMEM = {msg0, msg1, msg2, msg3};

// Prefs menu strings
const char prf0[] PROGMEM = "Saving to EEPROM";
const char prf1[] PROGMEM = "Code Group Size:";
const char prf2[] PROGMEM = "Code Speed:     ";
const char prf3[] PROGMEM = "Character Set:  ";
const char prf4[] PROGMEM = "Koch Number:    ";
const char prf5[] PROGMEM = "Skip Characters:";
const char prf6[] PROGMEM = "Out:0=key,1=spk";
const char* const prefs_menu[] PROGMEM = {prf0, prf1, prf2, prf3, prf4, prf5,prf6};

// Line buffer for LCD
char line_buf[17];
char blnk_ln[] {"                "};

//Character sets
const char koch[] PROGMEM = {'K','M','R','S','U','A','P','T','L','O','W','I','.','N','J','E','F',
'0','Y','V','.','G','5','/','Q','9','Z','H','3','8','B','?','4','2','7','C','1','D','6','X'};
const char alpha[] PROGMEM = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G',
'H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',',','.','/','?'};
const char* const char_sets[] PROGMEM = {alpha, koch};
char ch_buf[40];  // Buffer for character set

// IO definitions
const int morseInPin = 2;
const int beep_pin = 11;  // Pin for CW tone
const int key_pin = 12;   // Pin for CW Key

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();  // LCD class
morseDecoder morseInput(morseInPin, MORSE_KEYER, MORSE_ACTIVE_LOW);  // Morse receiver class
Morse morse(beep_pin, prefs[KEY_SPEED], prefs[OUT_MODE]); //default to beep on pin 11  // Morse sender class

//====================
// Setup Function
//====================
void setup()
{
  const int lcd_len = 16; // set width of LCD
  const int lcd_lines = 2; // Number of lines on LCD
  const int lcd_address = 0x27; // I2C LCD Address
  const int ser_baud = 9600;  // Serial baud rate
  
  // Start serial debug port
  Serial.begin(9600);
  while(!Serial);
  Serial.println("N4TL CW Trainer");

  // Start LCD
  lcd.begin(16, 2);
  lcd.setBacklight(WHITE);

  // Initialize application preferences
  prefs_init();

}  // end setup()


//====================
// Main Loop Function
//====================
void loop()
{
  byte op_mode;

  // Run the main menu
  op_mode = get_mode();

  // Dispatch the selected operation
  switch (op_mode) {
    case 1:
      morse_trainer();
      break;
    case 2:
      morse_decode();
      break;
    case 3:
      set_prefs();
      break;
  }  //end dispatch switch  
}  // end loop()


//====================
// Operating Mode Menu Function
//====================
byte get_mode()
{
  byte entry = 1;
  byte buttons = 0;
  boolean done = false;
  
  // Clear the LCD and display menu heading
  lcd.clear();
  lcd.setCursor(0,0);
  strcpy_P(line_buf, (char*)pgm_read_word(&(main_menu[0])));
  lcd.print(line_buf);
  
  do {
    lcd.setCursor(0, 1);
    strcpy_P(line_buf, (char*)pgm_read_word(&(main_menu[entry])));
    lcd.print(line_buf);
    delay(250);
    while(!(buttons = lcd.readButtons()));  // wait for button press
    if (buttons & BUTTON_UP) --entry;
    if (buttons & BUTTON_DOWN) ++entry;
    entry = constrain(entry, 1, 3);
    if (buttons & BUTTON_SELECT) done = true; 
    while(lcd.readButtons());  // wait for button release
  } while(!done);
  
  return entry;
}  // end get_mode()


//====================
// Set Preferences menu Function
//====================
void set_prefs()
{
  byte pref = 1;
  const byte n_prefs = sizeof(prefs)-1;
  byte p_val = 0;
  byte tmp;
  byte buttons = 0;
  boolean next = false;
  boolean done = false;

  // Clear the display
  lcd.clear();
  Serial.println("Set pref started");

  // Loop displaying preference values and let user change them
  do {
    // Display the selected preference
    lcd.setCursor(0,0);
    strcpy_P(line_buf, (char*)pgm_read_word(&(prefs_menu[pref])));
    lcd.print(line_buf);
    p_val = prefs[pref];

    do {
      // Display the pref current value and wait for button press
      lcd.setCursor(0,1);
      lcd.print(" = ");
      lcd.print(p_val);
      lcd.print("          ");
      delay(250);
      while(!(buttons = lcd.readButtons()));

      // Handle button press in priority order
      if (buttons & BUTTON_SELECT) {
        next = true;
        done = true;
      } else if (buttons & BUTTON_UP) {
        tmp = --pref;
        pref = constrain(tmp, 1, n_prefs);
        next = true;
      } else if (buttons & BUTTON_DOWN) {
        tmp = ++pref;
        pref = constrain(tmp, 1, n_prefs);
        next = true;
      } else if (buttons & BUTTON_RIGHT) {
        p_val = prefs_set(pref, ++p_val);
      } else if (buttons & BUTTON_LEFT) {
        p_val = prefs_set(pref, --p_val);
      }
      
    }while(!next);  // end of values loop
    next = false;  // Reset next prefs flag.

  } while(!done);  // end of prefs loop

  // Signal user select button was detected
  lcd.clear();
  lcd.setCursor(0,0);
  strcpy_P(line_buf, (char*)pgm_read_word(&(prefs_menu[SAVED_FLG])));
  lcd.print(line_buf);
  delay(500);
  while(lcd.readButtons());  // wait for button release
  
  // Save all prefs before returning.
  prefs_set(SAVED_FLG, 170);  // Set perfs saved flag
  for (int i=0; i<7; i++) {
    EEPROM.write(i, prefs[i]);
  }
}  // end set_prefs()


//====================
// Morse code send and receive
//====================
void morse_trainer()
{
  char cw_tx[17];   // Buffer for characters to send
  byte cset, lo, hi; // Set of characters to send the trainee
  byte i,j;
  byte buttons;
  unsigned int received_number = 0; // actual number of cw characters received
  unsigned int error = 0;

  // Init ===========================================================
  Serial.println("Morse trainer started");
  randomSeed(micros()); // random seed = microseconds since start.
  
  // Initialize Morse sender
  setup_morse();

  // Setup character set
  switch (prefs[CHAR_SET]) {
    case 1:  // alpha characters
      cset = 0;
      lo = 10;
      hi = 35;
      break;
    case 2:  // numbers
      cset = 0;
      lo = 0;
      hi = 9;
      break;
    case 3:  // punctuation
      cset = 0;
      lo = 36;
      hi = 39;
      break;
    case 4:  // all alphabetic
      cset = 0;
      lo = 0;
      hi = 39;
      break;
    case 5:  // Koch order
      cset = 1;
      lo = prefs[KOCH_SKIP];
      hi = prefs[KOCH_NUM];
      break;
    case 6:  // Koch order (same as 5 for now)
      cset = 1;
      lo = prefs[KOCH_SKIP];
      hi = prefs[KOCH_NUM];
      break;
  }

  // Copy the chosen character set from program memory to working buffer 
  strcpy_P(ch_buf, (char*)pgm_read_word(&(char_sets[cset])));

  
  // Training loop =======================================================
  do {
    Serial.print("\nTop of the sending loop ");
    lcd.clear();
    lcd.setCursor(0, 0); // Set the cursor to top line, left

    // Send characters to trainee
    for (i = 0; i < (prefs[GROUP_NUM]); i++)
    {
      if (error == 0 ) { // if no error on last round, generate new text.
        j = random(lo, hi);
        cw_tx[i] = ch_buf[j];
      }

      lcd.print(cw_tx[i]);  // Display the sent char
      morse.send(cw_tx[i]);   // Send the character
      Serial.print(cw_tx[i]); // debug print
    }
    
  } while(!(buttons = lcd.readButtons()));

  while(lcd.readButtons());  // wait for button release

}  // end morse_trainer()

/*
  while (true) // go
  {
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
  }  // end While
*/


//=====================================
// CW decoder only, use this section to check your keyers output to this decoder.
//=====================================
void morse_decode()
{
  unsigned int count = 0;
  unsigned int firstpass2 = 0;
  Serial.println("Morse decoder started");
    /*

  while (true)  // decode cw, if D is entered the sketch stays in the short loop.
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
  }  // end of while
    */
}  // end of morse_decode()




//===========================
// Restore app preferences from EEPROM if
// values are saved, else set to defaults.
// Send values to serial port on debug. 
//===========================
void prefs_init()
{
  // Restore app settings from the EEPROM if the saved
  // flag value is 170, otherwise init to defaults.
  if (EEPROM.read(0) == 170)
  {
    prefs_set(SAVED_FLG, 170);
    for (int idx = 1; idx < 7; idx++)
    {
      prefs_set(idx,EEPROM.read(idx));
    }
  }
  else
  {
    prefs_set(SAVED_FLG, 0);  // Prefs not saved
    prefs_set(GROUP_NUM, 1);  // Send/receive groups of 1 char to start
    prefs_set(KEY_SPEED, 25); // Send at 25 wpm to start
    prefs_set(CHAR_SET, 5);   // Use Koch order char set
    prefs_set(KOCH_NUM, 5);   // Use first 5 char in Koch set
    prefs_set(KOCH_SKIP, 0);  // Don't skip over any char to start
    prefs_set(OUT_MODE, 1);   // Output to speaker
  }
}


//========================
// Set preference specified in arg1 to value in arg2
// Constrain prefs values to defined limits
// Echo value to serial port
//========================
byte prefs_set(unsigned int indx, byte val)
{
  const byte lo_lim[] {0, 1, 20, 1, 1, 0, 0};  // Table of lower limits of preference values
  const byte hi_lim[] {170, 15, 30, 6, 40, 39, 1};  // Table of uppper limits of preference values
  byte new_val;

  // Set new value
  new_val = constrain(val, lo_lim[indx], hi_lim[indx]);

  // Dispatch on preference index to do debug print
  switch (indx) {
    case SAVED_FLG:
      Serial.print("Saved flag = ");
      break;
    case GROUP_NUM:
      Serial.print("Group size = ");
      break;
    case KEY_SPEED:
      Serial.print("Key speed = ");
      break;
    case CHAR_SET:
      Serial.print("Character set = ");
      break;
    case KOCH_NUM:
      Serial.print("Koch number = ");
      break;
    case KOCH_SKIP:
      if (new_val >= prefs[KOCH_NUM]) new_val = prefs[KOCH_NUM]-1;
      Serial.print("Skip = ");
      break;
    case OUT_MODE:
      Serial.print("Output mode = ");
      break;
    default:
      Serial.print("Preference index out of range\n");
  }

  // Print and save new value before returning it
  Serial.println(new_val);
  prefs[indx] = new_val;
  return new_val;
}


//=========================================
// The speed of the sent characters was measured with MRP40.
// the sent code was slow, an adjustment is necessary to speed it up.
// with an Key_speed_adj = 3, the measured output speed is
// 20 wpm measured to be 19.9 wpm
// 25 wpm measured to be 24.8 wpm
// 30 wpm measured to be 28.7 wpm
//=========================================
void setup_morse()  // set beep on or off, correct pin out and set speed
{
  unsigned int Key_speed_adj = 3; // correction for keying speed
  unsigned int key_speed = prefs[KEY_SPEED];
  unsigned int beep_on = prefs[OUT_MODE];
  if (beep_on == 1)
    Morse morse(beep_pin, key_speed + Key_speed_adj , beep_on);
  else
    Morse morse(key_pin, key_speed + Key_speed_adj , beep_on);
}

