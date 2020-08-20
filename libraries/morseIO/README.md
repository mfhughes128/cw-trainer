# morseIO
Morse code encoder and decoder library

From: MorseEnDecode library by raronzen@gmail.com

The MorseEnDecode library customized for use by the CW Trainer.
 - Add tone output and sidetone for key input.
 - Increase limit for characters to be considered in a word from 2/3 wordtime to 1x wordtime.
 
 The tone output is used by both the encode and decode functions, so a MorseSpeaker class is created to manage
 the speaker and arbitrate between the two users. Sidetone is given priority, so keying up will interrupt
 MorseEncoder output.
 
 This library is a quick hack to simplify the cw-trainer and add sidetone to the input. A more general solution
 is to abstract the I/O interface from the Encode/Decode function. That work is being pursued in the MorseEnDecode
 project.
 
 Thanks, Mike Hughes KC1DMR
 
