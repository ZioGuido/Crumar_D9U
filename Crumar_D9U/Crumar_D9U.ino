////////////////////////////////////////////////////////////////////
// Crumar Drawbar Controller D9U
// by Guido Scognamiglio 
// Runs on Atmel ATmega32U4 Arduino Leonardo (with MIDI USB Library)
// Reads 9 analog inputs from internal ADCs
// Sends MIDI CC numbers 12-20 or 21-29 according to selected mode
// Last update: July 2018
// 

////////////////////////////////////////////////////////////////////
// This is where you can define your CC numbers for the Bank 0 or 1
int CCMap[2][9] = 
{
  { 12, 13, 14, 15, 16, 17, 18, 19, 20 }, // Upper drawbars
  { 21, 22, 23, 24, 25, 26, 27, 28, 29 }  // Lower drawbars
};

////////////////////////////////////////////////////////////////////
// You should not modify anything else below this line 
// unless you know what you're doing.
////////////////////////////////////////////////////////////////////

// Define I/O pins
#define LED_RED     15
#define LED_GREEN   16
#define BUTTON      5

// Define global modes
#define DEBOUNCE_TIME 150
#define DEADBAND    8

// Include libraries
#include <EEPROM.h>
#include <MIDIUSB.h>
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// Init global variables
int mode = 1; // Should be either 0 or 1
int prev_val[9] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
int debounce_timer = DEBOUNCE_TIME;

// ADC reference map
int ADCmap[9] = { A0, A1, A2, A3, A6, A7, A8, A9, A10 };
int ADCcnt = 0;

// Called then the pushbutton is depressed
void set_mode()
{
    digitalWrite(LED_RED, mode ? LOW : HIGH);
    digitalWrite(LED_GREEN, mode ? HIGH : LOW);
    EEPROM.write(0x01, mode);
}

// Called to generate the MIDI CC message
void SendMidiCC(int channel, int num, int value)
{
  midiEventPacket_t CC = {0x0B, 0xB0 | channel, num, value};
  MidiUSB.sendMIDI(CC);
  MidiUSB.flush();

  // Midi lib wants channels 1~16
  MIDI.sendControlChange(num, value, channel+1);
}

// Called to check whether a drawbar has been moved
void DoDrawbar(int d, int value)
{
  // Get difference from current and previous value
  int diff = abs(value - prev_val[d]);
  
  // Exit this function if the new value is not within the deadband
  if (diff <= DEADBAND) return;
  
  // Store new value
  prev_val[d] = value;    

  // Get the 7 bit value
  int val7bit = value >> 3;
  
  // Send Midi 
  SendMidiCC(mode > 0 ? 1 : 0, CCMap[mode][d], val7bit);
}

// The setup routine runs once when you press reset:
void setup() 
{
  // Initialize serial MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);  

  // Set up digital I/Os
  pinMode(BUTTON, INPUT_PULLUP); // Button
  pinMode(LED_RED, OUTPUT);      // Led 1
  pinMode(LED_GREEN, OUTPUT);    // Led 2
  
  // Recall mode from memory and set
  // Make sure mode is either 0 or 1
  mode = EEPROM.read(0x01) > 0 ? 1 : 0;
  set_mode();
}

// The loop routine runs over and over again forever:
void loop() 
{
  // Read analog inputs (do the round robin)
  DoDrawbar(ADCcnt, analogRead(ADCmap[ADCcnt]));
  if (++ADCcnt > 8) ADCcnt = 0;

  // Read Button
  if (digitalRead(BUTTON) == LOW)
  {
    if (debounce_timer > 0) --debounce_timer;
  } else {
    debounce_timer = DEBOUNCE_TIME;
  }
  
  if (debounce_timer == 2) 
  {
    mode = !mode; // Reverse
    set_mode(); // and Set!
  }
}
