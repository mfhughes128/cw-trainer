#ifndef MorseEnDecoder_H
#define MorseEnDecoder_H

#include <Arduino.h>
#include <pitches.h>

#define OUTPUT_TONE_PITCH NOTE_A3
#define SIDE_TONE_PITCH NOTE_C4
#define MORSE_AUDIO true
#define MORSE_KEYER false
#define MORSE_ACTIVE_LOW true
#define MORSE_ACTIVE_HIGH false


class MorseSpeaker
{
  public:
    MorseSpeaker(int t_spkrPin);
    void outputTone(boolean t_on);
    void sideTone(boolean t_on);
    boolean outputToneOn;
    boolean sideToneOn;
  private:
    boolean keyDown;
  protected:
    int spkrOut;
};


class MorseDecoder
{
  public:
    MorseDecoder(int decodePin, boolean listenAudio, boolean morsePullup, MorseSpeaker*);
    void decode();
    void setspeed(int value);
    char read();
    boolean available();
    int AudioThreshold;
    long debounceDelay;     // the debounce time. Keep well below dotTime!!
    boolean morseSignalState;  
  private:
    int morseInPin;         // The Morse input pin
    int audioSignal;
    MorseSpeaker *MorseSpkr;
    int morseTablePointer;
    int wpm;                // Word-per-minute speed
    long dotTime;           // morse dot time length in ms
    long dashTime;
    long wordSpace;
    boolean morseSpace;     // Flag to prevent multiple received spaces
    boolean gotLastSig;     // Flag that the last received morse signal is decoded as dot or dash
    boolean morseKeyer;
    boolean lastKeyerState;
    boolean morseAudio;
    boolean activeLow;
    long markTime;          // timers for mark and space in morse signal
    long spaceTime;         // E=MC^2 ;p
    long lastDebounceTime;  // the last time the input pin was toggled
    long currentTime;       // The current (signed) time
    char decodedMorseChar;  // The last decoded Morse character
};


class MorseEncoder
{
  public:
    MorseEncoder(int encodePin, MorseSpeaker*);
    void encode();
    void setspeed(int value);
    void write(char temp);
    boolean available();
    int morseSignals;       // nr of morse signals to send in one morse character
    char morseSignalString[7];// Morse signal for one character as temporary ASCII string of dots and dashes
  private:
    MorseSpeaker *MorseSpkr;
    char encodeMorseChar;   // ASCII character to encode
    boolean sendingMorse;
    int wpm;                // Word-per-minute speed
    long dotTime;           // morse dot time length in ms
    long dashTime;
    long wordSpace;
    int morseSignalPos;
    int sendingMorseSignalNr;
    long sendMorseTimer;
    long lastDebounceTime;
    long currentTime;
 protected:
    int morseOutPin;
    virtual void setup_signal();
    virtual void start_signal(bool startOfChar, char signalType);
    virtual void stop_signal(bool endOfChar, char signalType);
};
      
#endif
