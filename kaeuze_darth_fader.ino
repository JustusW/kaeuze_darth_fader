#include <Bounce.h> 
#include <EEPROM.h>
#include <MIDI.h>

// MIDI constants
#define MIDI_OUTPUT_CHANNEL 1
#define MIDI_VELOCITY_ON    99
#define MIDI_VELOCITY_OFF   0

// MIDI note numbers assigned to specific actions
#define MIDI_NOTE_STOP      60
#define MIDI_NOTE_PAUSE     61
#define MIDI_NOTE_NEXT      62
#define MIDI_NOTE_UNUSED1   63
#define MIDI_NOTE_PREVIOUS  64
#define MIDI_NOTE_PLAY      65

// MIDI control channel for fader and its range
#define MIDI_FADER_CHANNEL  10
#define MIDI_FADER_MAX      127
#define MIDI_FADER_MIN      0

// Pin definitions for buttons and fader
#define PIN_BTN_STOP        0
#define PIN_BTN_PAUSE       1
#define PIN_BTN_TOPRIGHT    2
#define PIN_BTN_TOPMIDDLE   3
#define PIN_BTN_TOPLEFT     4
#define PIN_BTN_PLAY        5

#define PIN_FADER_IN        A6
#define PIN_FADER_OUT       A5

// Debounce time for button presses
#define BTN_DEBOUNCE_TIME   50

// EEPROM memory addresses for fader calibration values
#define ADR_EEPROM_CALIBRATION_LOW    10
#define ADR_EEPROM_CALIBRATION_HIGH   20

// Calibration offset values for fader
#define FADER_CALIBRATION_OFFSET_LOW  5
#define FADER_CALIBRATION_OFFSET_HIGH 5

// Button debouncing objects for each button
Bounce btn_play       = Bounce(PIN_BTN_PLAY, BTN_DEBOUNCE_TIME);
Bounce btn_stop       = Bounce(PIN_BTN_STOP, BTN_DEBOUNCE_TIME);
Bounce btn_pause      = Bounce(PIN_BTN_PAUSE, BTN_DEBOUNCE_TIME);
Bounce btn_topright   = Bounce(PIN_BTN_TOPRIGHT, BTN_DEBOUNCE_TIME);
Bounce btn_topmiddle  = Bounce(PIN_BTN_TOPMIDDLE, BTN_DEBOUNCE_TIME);
Bounce btn_topleft    = Bounce(PIN_BTN_TOPLEFT, BTN_DEBOUNCE_TIME);

// Variables to store fader calibration values
int16_t low;
int16_t high;

// Variable to store the previous fader value
int16_t old_val;

/* Calibrate the fader with the given current value.
   Attempts to update the EEPROM value if either low or
   high calibration target is changed.
   Note: EEPROM.put will not do superfluous writes 
   if no value change, but we still try to call it 
   conservatively.
*/
void calibrate_fader (int16_t val) {
  if (val > high) {
    high = val; // Update the high value if necessary
    EEPROM.put(ADR_EEPROM_CALIBRATION_HIGH, high); // Store new high value in EEPROM
  }
  if (val < low) {
    low = val; // Update the low value if necessary
    EEPROM.put(ADR_EEPROM_CALIBRATION_LOW, low); // Store new low value in EEPROM
  }
}

/* Read Fader state and send any updates via MIDI.
   First we set the out-pin to high, to have a value to read.
   A delay is provided for the value to stabilize. After
   reading the current analog value (10bit, 0-1024) the
   output is deactivated, calibration is updated if necessary,
   and then the value is mapped to the range provided by
   MIDI_FADER_MIN and MIDI_FADER_MAX. An Offset is then
   applied to guarantee full range availability in the SCS.
*/
void readFader() {
  digitalWrite(PIN_FADER_OUT, HIGH);  // Set fader output pin high reading
  delay(10); // Allow time for value to stabilize
  int16_t val = analogRead(PIN_FADER_IN);  // Read the analog fader value
  digitalWrite(PIN_FADER_OUT, LOW);  // Set the output pin low after reading

  calibrate_fader(val);  // Calibrate the fader values if necessary
  
  // Map the fader value to the MIDI range with calibration offset
  val = map(val, low, high, MIDI_FADER_MIN - FADER_CALIBRATION_OFFSET_LOW, MIDI_FADER_MAX + FADER_CALIBRATION_OFFSET_HIGH);
  val = min(val, MIDI_FADER_MAX);  // Ensure the value is within max range
  val = max(val, MIDI_FADER_MIN);  // Ensure the value is within min range

  // Send MIDI control change if the fader value has changed
  if (val != old_val) {
    usbMIDI.sendControlChange(MIDI_FADER_CHANNEL, val, MIDI_OUTPUT_CHANNEL);
  }
  old_val = val;  // Update the previous value
}

/* Update the button state and send corresponding MIDI messages.
   The button is updated using Bounce library. If a falling edge
   (button press) is detected, a Note On message is sent. If a 
   rising edge (button release) is detected, a Note Off message 
   is sent.
*/
void btn_update(Bounce *btn, int note) {
  btn->update();  // Update the state of the button

  // Check for falling edge (button press) and send Note On
  if (btn->fallingEdge()) {
    usbMIDI.sendNoteOn(note, MIDI_VELOCITY_ON, MIDI_OUTPUT_CHANNEL);
  }
  // Check for rising edge (button release) and send Note Off
  if (btn->risingEdge()) {
    usbMIDI.sendNoteOff(note, MIDI_VELOCITY_OFF, MIDI_OUTPUT_CHANNEL);
  }
}

/* Initialize fader by setting the output pin and reading
   stored calibration values from EEPROM.
*/
void setup_fader() {
  pinMode(PIN_FADER_OUT, OUTPUT);  // Set the fader output pin as an output

  // Read the stored low and high calibration values from EEPROM
  EEPROM.get(ADR_EEPROM_CALIBRATION_LOW, low);
  EEPROM.get(ADR_EEPROM_CALIBRATION_HIGH, high);
}

/* Initialize buttons by setting them as inputs with pull-up resistors enabled.
*/
void setup_btn() {
  pinMode(PIN_BTN_STOP, INPUT_PULLUP);
  pinMode(PIN_BTN_PAUSE, INPUT_PULLUP);
  pinMode(PIN_BTN_TOPRIGHT, INPUT_PULLUP);
  pinMode(PIN_BTN_TOPMIDDLE, INPUT_PULLUP);
  pinMode(PIN_BTN_TOPLEFT, INPUT_PULLUP);
  pinMode(PIN_BTN_PLAY, INPUT_PULLUP);
}

void setup() {
  setup_fader();  // Initialize the fader
  setup_btn();    // Initialize the buttons

  Serial.begin(115200);  // Start serial communication for debugging (if necessary)
}

void loop() {
  // Mapping buttons to MIDI Notes
  btn_update(&btn_play,      MIDI_NOTE_PLAY);
  btn_update(&btn_stop,      MIDI_NOTE_STOP);
  btn_update(&btn_pause,     MIDI_NOTE_PAUSE);
  btn_update(&btn_topright,  MIDI_NOTE_NEXT);
  btn_update(&btn_topmiddle, MIDI_NOTE_UNUSED1);
  btn_update(&btn_topleft,   MIDI_NOTE_PREVIOUS);

  readFader();  // Read the fader

  // Ensure MIDI interface is working by reading any incoming messages
  while (usbMIDI.read()) {}
}
