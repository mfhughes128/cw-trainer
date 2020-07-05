/*
           MORSE ENDECODER BAREBONES

  Minimum sketch to send/receive Morse code via the serial monitor.

  This example decodes Morse signals present on digital input 7
    (active low, and then also using the internal pullup resistor).
  It also encodes Morse sent via the serial interface to the Arduino,
    on digital output pin 13.

  Copyright (C) 2010, 2012 raron
  GNU GPLv3 license (http://www.gnu.org/licenses)
  Contact: raronzen@gmail.com  (not checked too often..)
  Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/
*/

#include <avr/pgmspace.h>
#include <MorseEnDecoder.h>

// Pin mapping
const byte morseInPin = 7;      
const byte morseOutPin = 13;

// Instantiate Morse objects
morseDecoder morseInput(morseInPin, MORSE_KEYER, MORSE_ACTIVE_LOW);
morseEncoder morseOutput(morseOutPin);

void setup()
{
  Serial.begin(9600);
  Serial.println("Morse EnDecoder barebones demo");
  
  // Setting Morse speed in wpm - words per minute
  // If not set, 13 wpm is default anyway
  morseInput.setspeed(13);
  morseOutput.setspeed(13);
}


void loop()
{
  // Need to call these once per loop
  morseInput.decode();
  morseOutput.encode();

  // SEND MORSE (OUTPUT)
  // Encode and send text received from the serial port (serial monitor)
  if (Serial.available() && morseOutput.available())
  {
    // Get character from serial and send as Morse code
    char sendMorse = Serial.read();
    morseOutput.write(sendMorse);
  }

  // RECEIVE MORSE (INPUT)
  // If a character is decoded from the input, write it to serial port
  if (morseInput.available())
  {
    char receivedMorse = morseInput.read();
    Serial.print(receivedMorse);
    
    // A little error checking    
    if (receivedMorse == '#') Serial.println("< ERROR:too many morse signals! >");
  }

}

