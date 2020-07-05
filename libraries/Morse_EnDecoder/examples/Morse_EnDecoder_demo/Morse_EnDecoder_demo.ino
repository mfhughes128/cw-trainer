/*
                MORSE ENDECODER DEMO

  Demonstrates using Morse encoder and decoder classes for the Arduino.

  Local Morse code feedback to both serial and Morse output (unless sending 
    at the same time as receiving or keying in morse), for morse training demo.
  Formatted serial port output - Serial monitor looks a bit nicer.


  This example decodes Morse code signals with a speed of 13 WPM present on
    digital input 7 (active low, and also using the internal pullup resistor).

  It also encodes Morse code sent via the serial interface to the Arduino,
    on digital output pin 13. Speed is 13 WPM also (easily changed in code).
  
  It can also decode audible signals, if using the constant MORSE_AUDIO
    instead of MORSE_KEYER, but then it is important to note that the
    input pin nr. will be for ANALOG inputs (0-5 on Atmega 168 - 328),
    and not the digital inputs.



  Copyright (C) 2010, 2012 raron
  
  GNU GPLv3 license:
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
   
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
   
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
   
  
  Contact: raronzen@gmail.com  (not checked too often..)
  Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/
*/

#include <avr/pgmspace.h>
#include <MorseEnDecoder.h>

// Pin mappings
const byte morseInPin = 7;      
const byte morseOutPin = 13;

// Instantiate Morse objects
morseDecoder morseInput(morseInPin, MORSE_KEYER, MORSE_ACTIVE_LOW);
morseEncoder morseOutput(morseOutPin);

// Variables dealing with formatting the output somewhat
// by inserting CR's (carriage returns)
long lastTransmissionTime;
long currentTime;
boolean transmissionEnded = true; // Flag to mark old transmission is finished

// Minimum transmission pause time to insert carriage returns (CR)
// Adjust depending on Morse speed. IE 13 wpm = 646 ms between words (no CR).
const long transmissionPaused   = 1000; // Suitable for 13 wpm?


void setup()
{
  Serial.begin(9600);
  Serial.println("Morse EnDecoder demo");
  
  // Setting Morse speed in wpm - words per minute
  // If not set, 13 wpm is default anyway
  morseInput.setspeed(13);
  morseOutput.setspeed(13);
  
  lastTransmissionTime = (long)millis();
}



void loop()
{
  currentTime = (long)millis();
  
  // Needs to call these once per loop
  morseInput.decode();
  morseOutput.encode();

  // SEND MORSE (OUTPUT)
  // Encode and send text received from the serial port (serial monitor)
  if (Serial.available() && morseOutput.available())
  {
    // Get character from serial and send as Morse code
    char sendMorse = Serial.read();
    morseOutput.write(sendMorse);
    
    // Not strictly needed, but used to get morseSignalString before it is destroyed
    // (E.g. for morse training purposes)
    morseOutput.encode();

    // Also write sent character + Morse code to serial port/monitor
    Serial.write(' ');
    Serial.write(sendMorse);
    // Morse code in morseSignalString is now backwards
    for (int i=morseOutput.morseSignals; i>0; i--)
    {
      Serial.write(morseOutput.morseSignalString[i-1]);
    }
  }


  // RECEIVE MORSE (INPUT)
  // If a character is decoded from the input, write it to serial port
  if (morseInput.available())
  {
    // Get decoded Morse code character and write it to serial port/monitor
    char receivedMorse = morseInput.read();
    Serial.print(receivedMorse);
    
    // A little error checking    
    if (receivedMorse == '#') Serial.println("< ERROR:too many morse signals! >");
  }


  // Local Morse code feedback from input if not sending Morse simultaneously
  if (morseOutput.available()) digitalWrite(morseOutPin, morseInput.morseSignalState);


  // Check if ongoing transmission (not yet transmission pause)
  if (!morseOutput.available() || morseInput.morseSignalState == true)
  {
    // reset last transmission timer and flag
    lastTransmissionTime = currentTime;
    transmissionEnded = false;
  }

  // Format output with carriage returns after a transmission pause
  if ((currentTime - lastTransmissionTime) > transmissionPaused)
  {
    if (transmissionEnded == false)
    {
      // Separate the transmissions somewhat in the serial monitor with CR's
      for (int cr=0; cr<2; cr++) Serial.println("");  // some carriage returns..
      
      // Finally set the flag to prevent continous carriage returns
      transmissionEnded = true;
    }
  }
}

