/*
           MORSE ENCODER TONE OUTPUT

  Minimum sketch to send Morse code for characters received from via the serial stream
  out via a tone on an analog out pin.

  It also encodes Morse sent via the serial interface to the Arduino,
    on analog output pin 13.

Original implementation:
  Copyright (C) 2010, 2012 raron
  GNU GPLv3 license (http://www.gnu.org/licenses)
  Contact: raronzen@gmail.com  (not checked too often..)
  Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/

pluggable output refactoring:
  Contact: albert.denhaan@gmail.com  (not checked too often..)
*/

#include <avr/pgmspace.h>
#include <MorseEnDecoder.h>
#include "pitches.h"

/** provide an alternate implemtnation to the default digitalWrite with tone and text instead 
*/
class morseEncoderTone: public morseEncoder
{
public:
  morseEncoderTone(int encodePin);
protected:
  void setup_signal();
  void start_signal(bool startOfChar, char signalType);
  void stop_signal(bool endOfChar, char signalType);
};


morseEncoderTone::morseEncoderTone(int encodePin) : morseEncoder(encodePin) 
{
}


void morseEncoderTone::setup_signal()
{
  pinMode(morseOutPin, OUTPUT);
  digitalWrite(morseOutPin, LOW);
}

void morseEncoderTone::start_signal(bool startOfChar, char signalType) 
{
  noTone(this->morseOutPin);
  if(startOfChar)
    Serial.print('!');
    
  switch (signalType) {
    case '.':
      Serial.print("dit");
      break;
    case '-':
      Serial.print("dah");
      break;
    default:
      Serial.print(signalType);
      break;
  }
  tone(this->morseOutPin, NOTE_A3);
}

void morseEncoderTone::stop_signal(bool endOfChar, char signalType) 
{
  noTone(this->morseOutPin);
  if (endOfChar) {
    Serial.println(' ');
  } else {
    Serial.print(' ');
  }
}

/* now use the new class */

// Pin mapping 
const byte morseOutPin = 13;  // make sure this is compatible with the Tone class!
morseEncoderTone morseOutput(morseOutPin);

void setup()
{
  Serial.begin(9600);
  Serial.println("Morse Encoder tone demo");
  
  // Setting Morse speed in wpm - words per minute
  // If not set, 13 wpm is default anyway
  morseOutput.setspeed(13);
}


void loop()
{
  // Need to call this once per loop
  morseOutput.encode();

  // SEND MORSE (OUTPUT)
  // Encode and send text received from the serial port (serial monitor)
  if (Serial.available() && morseOutput.available())
  {
    // Get character from serial and send as Morse code
    char sendMorse = Serial.read();
    morseOutput.write(sendMorse);
  }
}

