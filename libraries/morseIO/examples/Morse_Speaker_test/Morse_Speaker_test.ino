/*
           MORSE ENDECODER TONE TEST

Sketch to test tone generation for Morse output and sidetone

This example generates one second tones at the pitches specified for the
morse output and the morse input sidetone.

Copyright (C) 2010, 2012 raron
  GNU GPLv3 license (http://www.gnu.org/li
*/

#include <MorseEnDecoder.h>

// Pin mapping
const byte morseSpkrPin = 11;

typedef void (MorseSpeaker::*SpkrFunc)(boolean t_on);
SpkrFunc outTone = &MorseSpeaker::outputTone;
SpkrFunc sideTone = &MorseSpeaker::sideTone;

// Instantiate speaker object
MorseSpeaker MorseSound(morseSpkrPin);

void setup()
{
	// Enable encode and decode output
	MorseSound.sideToneOn = true;
	MorseSound.outputToneOn = true;
}

void loop()
{
  delay(1000);

  // Sound Morse Output tone
  playTone(1000, outTone);
	
	delay(1000);

  // Sound Morse sidetone
  playTone(1000, sideTone);
}

void playTone(int duration, SpkrFunc func)
{
  int ttp = duration;
  (MorseSound.*func)(true);
  delay(ttp);
  (MorseSound.*func)(false);
}
