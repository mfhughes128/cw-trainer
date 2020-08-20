/*          MORSE ENDECODER
 
 - Morse encoder / decoder classes for the Arduino.

 Copyright (C) 2010-2012 raron

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
 

 Contact: raronzen@gmail.com
 Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/
 
 2020/08/19 - Add Class to manage a speaker
            - Add sidetone to decoder and tone output option to encoder.
 */ 

#include <avr/pgmspace.h>
#include "MorseEnDecoder.h"

// Morse code binary tree table (dichotomic search table)
// ITU with most punctuation (but without non-english characters - for now)
const int morseTreeLevels = 6; // Minus top level, also the max nr. of morse signals
const int morseTableLength = pow(2,morseTreeLevels+1);
const char morseTable[] PROGMEM = 
  " ETIANMSURWDKGOHVF*L*PJBXCYZQ!*54*3***2&*+****16=/***(*7***8*90*"
  "***********?_****\"**.****@***'**-********;!*)*****,****:*******\0";


/*
  Morse Speaker Class
    Generates audible output for encoder and sidetone for
    decoder. Sidetone over-rides output.
*/

MorseSpeaker::MorseSpeaker(int t_spkrPin)
{
  // Setup the speaker output pin
  spkrOut = t_spkrPin;
  pinMode(spkrOut, OUTPUT);
  digitalWrite(spkrOut, LOW);

  // Set initial state
  keyDown = false;
  sideToneOn = false;
  outputToneOn = false;
  
  noTone(spkrOut);
}

void MorseSpeaker::outputTone(boolean t_on)
{
  if (outputToneOn) {
    if (t_on && !keyDown) {
      tone(spkrOut, OUTPUT_TONE_PITCH);
    } else {
      noTone(spkrOut);
    }
  }
}

void MorseSpeaker::sideTone(boolean t_on)
{
  if (sideToneOn) {
    if (t_on) {
      keyDown = true;
      tone(spkrOut, SIDE_TONE_PITCH);
    } else {
      keyDown = false;
      noTone(spkrOut);
    }
  }
}


/*
  Morse Decoder Class
    Translates Morse digital signal or tones to a
    stream of characters.
*/
MorseDecoder::MorseDecoder(int decodePin, boolean listenAudio, boolean morsePullup, MorseSpeaker* Spkr_p)
{
  morseInPin = decodePin;
  morseAudio = listenAudio;
  activeLow = morsePullup;
  MorseSpkr = Spkr_p;

  if (morseAudio == false)
  {
    pinMode(morseInPin, INPUT);
    if (activeLow) digitalWrite (morseInPin, HIGH);
  }

  // Some initial values  
  wpm = 13;
  AudioThreshold = 700;
  debounceDelay = 20;
  dotTime = 1200 / wpm;       // morse dot time length in ms
  dashTime = 3 * 1200 / wpm;
  wordSpace = 7 * 1200 / wpm;

  morseTablePointer = 0;
 
  morseKeyer = LOW;
  morseSignalState = LOW;
  lastKeyerState = LOW;
  MorseSpkr->sideTone(false);

  gotLastSig = true;
  morseSpace = true;
  decodedMorseChar = '\0';
  
  lastDebounceTime = 0;
  markTime = 0;
  spaceTime = 0;
}


void MorseDecoder::setspeed(int value)
{
  wpm = value;
  if (wpm <= 0) wpm = 1;
  dotTime = 1200 / wpm;
  dashTime = 3 * 1200 / wpm;
  wordSpace = 7 * 1200 / wpm;
}


boolean MorseDecoder::available()
{
  if (decodedMorseChar) return true; else return false;
}


char MorseDecoder::read()
{
  char temp = decodedMorseChar;
  decodedMorseChar = '\0';
  return temp;
}


void MorseDecoder::decode()
{
  currentTime = millis();
  
  // Read Morse signals
  if (morseAudio == false)
  {
    // Read the Morse keyer (digital)
    morseKeyer = digitalRead(morseInPin);
    if (activeLow) morseKeyer = !morseKeyer;

    // If the switch changed, due to noise or pressing:
    if (morseKeyer != lastKeyerState) {
      lastDebounceTime = currentTime; // reset timer
      MorseSpkr->sideTone(morseKeyer); // turn sidetone on or off
    }
    // debounce the morse keyer
    if ((currentTime - lastDebounceTime) > debounceDelay)
    {
      // whatever the reading is at, it's been there for longer
      // than the debounce delay, so take it as the actual current state:
      morseSignalState = morseKeyer;
      
      // differentiante mark and space times
      if (morseSignalState) markTime = lastDebounceTime; 
      else spaceTime = lastDebounceTime;
    }
  } else {
    // Read Morse audio signal
    audioSignal = analogRead(morseInPin);
    if (audioSignal > AudioThreshold)
    {
      // If this is a new morse signal, reset morse signal timer
      if (currentTime - lastDebounceTime > dotTime/2)
      {
        markTime = currentTime;
        morseSignalState = true; // there is currently a Morse signal
      }
      lastDebounceTime = currentTime;
    } else {
      // if this is a new pause, reset space time
      if (currentTime - lastDebounceTime > dotTime/2 && morseSignalState == true)
      {
        spaceTime = lastDebounceTime; // not too far off from last received audio
        morseSignalState = false;     // No more signal
      }
    }
  }
  
  // Decode morse code
  if (!morseSignalState)
  {
    if (!gotLastSig)
    {
      if (morseTablePointer < morseTableLength/2-1)
      {
        // if pause for more than half a dot, get what kind of signal pulse (dot/dash) received last
        if (currentTime - spaceTime > dotTime/2)
        {
          // if signal for more than 1/4 dotTime, take it as a morse pulse
          if (spaceTime-markTime > dotTime/4)
          {
            morseTablePointer *= 2;  // go one level down the tree
            // if signal for less than half a dash, take it as a dot
            if (spaceTime-markTime < dashTime/2)
            {
               morseTablePointer++; // point to node for a dot
               gotLastSig = true;
            }
            // else if signal for between half a dash and a dash + one dot (1.33 dashes), take as a dash
            else if (spaceTime-markTime < dashTime + dotTime)
            {
               morseTablePointer += 2; // point to node for a dash
               gotLastSig = true;
            }
          }
        }
      } else { // error if too many pulses in one morse character
        //Serial.println("<ERROR: unrecognized signal!>");
        decodedMorseChar = '#'; // error mark
        gotLastSig = true;
        morseTablePointer = 0;
      }
    }
    // Write out the character if pause is longer than 2/3 dash time (2 dots) and a character received
    if ((currentTime-spaceTime >= (dotTime*2)) && (morseTablePointer > 0))
    {
      decodedMorseChar = pgm_read_byte_near(morseTable + morseTablePointer);
      morseTablePointer = 0;
    }
    // Write a space if pause is longer than wordspace
    if (currentTime-spaceTime > (wordSpace) && morseSpace == false)
    {
      decodedMorseChar = ' ';
      morseSpace = true ; // space written-flag
    }

  } else {
    // while there is a signal, reset some flags
    gotLastSig = false;
    morseSpace = false;
  }
  
  // Save the morse keyer state for next round
  lastKeyerState = morseKeyer;
}


/*
  Morse Encoder Class
    Translates characters to Morse code and keys
    io pin and sends tones as appropriate.
*/

MorseEncoder::MorseEncoder(int encodePin, MorseSpeaker *Spkr_p)
{
  morseOutPin = encodePin;
  MorseSpkr = Spkr_p;
  this->setup_signal();

  // some initial values
  sendingMorse = false;
  encodeMorseChar = '\0';

  wpm = 13;
  dotTime = 1200 / wpm;       // morse dot time length in ms
  dashTime = 3 * 1200 / wpm;
  wordSpace = 7 * 1200 / wpm;
 
}


void MorseEncoder::setspeed(int value)
{
  wpm = value;
  if (wpm <= 0) wpm = 1;
  dotTime = 1200 / wpm;
  dashTime = 3 * 1200 / wpm;
  wordSpace = 7 * 1200 / wpm;
}


boolean MorseEncoder::available()
{
  if (sendingMorse) return false; else return true;
}


void MorseEncoder::write(char temp)
{
  if (!sendingMorse && temp != '*') encodeMorseChar = temp;
}


void MorseEncoder::setup_signal()
{
  pinMode(morseOutPin, OUTPUT);
  digitalWrite(morseOutPin, LOW);
  MorseSpkr->outputTone(false);
}


void MorseEncoder::start_signal(bool startOfChar, char signalType)
{
  digitalWrite(morseOutPin, HIGH);
  MorseSpkr->outputTone(true);
}


void MorseEncoder::stop_signal(bool endOfChar, char signalType)
{
  digitalWrite(morseOutPin, LOW);
  MorseSpkr->outputTone(false);
}


void MorseEncoder::encode()
{
  currentTime = millis();

  if (!sendingMorse && encodeMorseChar)
  {
    // change to capital letter if not
    if (encodeMorseChar > 96) encodeMorseChar -= 32;
  
    // Scan for the character to send in the Morse table
    int p;
    for (p=0; p<morseTableLength+1; p++) if (pgm_read_byte_near(morseTable + p) == encodeMorseChar) break;

    if (p >= morseTableLength) p = 0; // not found, but send a space instead


    // Reverse binary tree path tracing
    int pNode; // parent node
    morseSignals = 0;

    // Travel the reverse path from position p to the top of the morse table
    if (p > 0)
    {
      // build the morse signal (backwards morse signal string from last signal to first)
      pNode = p;
      while (pNode > 0)
      {
        if ( (pNode & 0x0001) == 1)
        {
          // It is a dot
          morseSignalString[morseSignals++] = '.';
        } else {
          // It is a dash
          morseSignalString[morseSignals++] = '-';
        }
        // Find parent node
        pNode = int((pNode-1)/2);
      }
    } else { // Top of Morse tree - Add the top space character
      // cheating a little; a wordspace for a "morse signal"
      morseSignalString[morseSignals++] = ' ';
    }
    
    morseSignalString[morseSignals] = '\0';


    // start sending the the character
    sendingMorse = true;
    sendingMorseSignalNr = morseSignals; // Sending signal string backwards
    sendMorseTimer = currentTime;
    if (morseSignalString[0] != ' ') this->start_signal(true, morseSignalString[morseSignals-1]);
  }


  // Send Morse signals to output
  if (sendingMorse)
  {
    char& currSignalType = morseSignalString[sendingMorseSignalNr-1];
    bool endOfChar = sendingMorseSignalNr <= 1;
    switch (currSignalType)
    {
      case '.': // Send a dot (actually, stop sending a signal after a "dot time")
        if (currentTime - sendMorseTimer >= dotTime)
        {
          this->stop_signal(endOfChar, currSignalType);
          sendMorseTimer = currentTime;
          currSignalType = 'x'; // Mark the signal as sent
        }
        break;
      case '-': // Send a dash (same here, stop sending after a dash worth of time)
        if (currentTime - sendMorseTimer >= dashTime)
        {
          this->stop_signal(endOfChar, currSignalType);
          sendMorseTimer = currentTime;
          currSignalType = 'x'; // Mark the signal as sent
        }
        break;
      case 'x': // To make sure there is a pause between signals
        if (sendingMorseSignalNr > 1)
        {
          // Pause between signals in the same letter
          if (currentTime - sendMorseTimer >= dotTime)
          {
            sendingMorseSignalNr--;
            this->start_signal(false, morseSignalString[sendingMorseSignalNr-1]); // Start sending the next signal
            sendMorseTimer = currentTime;       // reset the timer
          }
        } else {
          // Pause between letters
          if (currentTime - sendMorseTimer >= dashTime)
          {
            sendingMorseSignalNr--;
            sendMorseTimer = currentTime;       // reset the timer
          }
        }
        break;
      case ' ': // Pause between words (minus pause between letters - already sent)
      default:  // Just in case its something else
        if (currentTime - sendMorseTimer > wordSpace - dashTime) sendingMorseSignalNr--;
    }
    if (sendingMorseSignalNr <= 0 )
    {
      // Ready to encode more letters
      sendingMorse = false;
      encodeMorseChar = '\0';
    }
  }
}
