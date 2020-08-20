/*
           MORSE ENDECODER BAREBONES

  Minimum sketch to send/receive Morse code via the serial monitor.

  This example decodes Morse signals present on digital input 2
    (active low, and then also using the internal pullup resistor).
  It also encodes Morse sent via the serial interface to the Arduino,
    on digital output pin 12.
  Tone output from the encoder and sidetone from the decoder are sent
    to a speaker on pin 11.

  Copyright (C) 2010, 2012 raron
  GNU GPLv3 license (http://www.gnu.org/licenses)
  Contact: raronzen@gmail.com  (not checked too often..)
  Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/
*/

#include <avr/pgmspace.h>
#include <MorseEnDecoder.h>

// Pin mapping
const byte morseInPin = 2;
const byte morseSpkrPin = 11;
const byte morseOutPin = 12;

// Instantiate speaker, key input and outoput for Morse Trainer
MorseSpeaker Spkr(morseSpkrPin);
MorseDecoder MorseInput(morseInPin, MORSE_KEYER, MORSE_ACTIVE_LOW, &Spkr);
MorseEncoder MorseOutput(morseOutPin, &Spkr);

void setup()
{
  Serial.begin(9600);
  Serial.println("Morse Encoder/Decoder barebones demo");
  
  // Enable speaker output for the encoder and decoder
  Spkr.outputToneOn = true;
  Spkr.sideToneOn = true;

  // Set Encode and Decode speed in wpm. Default is 13 wpm.
  MorseInput.setspeed(13);
  MorseOutput.setspeed(13);
}

void loop()
{
  // Need to call these once per loop
  MorseInput.decode();
  MorseOutput.encode();

  // SEND MORSE (OUTPUT)
  // Encode and send text received from the serial port (serial monitor)
  if (Serial.available() && MorseOutput.available())
  {
    // Get character from serial and send as Morse code
    char sendMorse = Serial.read();
    MorseOutput.write(sendMorse);
  }

  // RECEIVE MORSE (INPUT)
  // If a character is decoded from the input, write it to serial port
  if (MorseInput.available())
  {
    char receivedMorse = MorseInput.read();
    Serial.print(receivedMorse);
    
    // A little error checking    
    if (receivedMorse == '#') Serial.println("< ERROR:too many morse signals! >");
  }

}
