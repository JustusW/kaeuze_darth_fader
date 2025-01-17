#include <Bounce.h>
#include <EEPROM.h>
#include <MIDI.h>

#define MIDI_OUTPUT_CHANNEL 1
#define MIDI_VELOCITY_ON    99
#define MIDI_VELOCITY_OFF   0

#define MIDI_NOTE_STOP      60
#define MIDI_NOTE_PAUSE     61
#define MIDI_NOTE_NEXT      62
#define MIDI_NOTE_UNUSED1   63
#define MIDI_NOTE_PREVIOUS  64
#define MIDI_NOTE_PLAY      65

#define MIDI_FADER_CHANNEL  10
#define MIDI_FADER_MAX      127
#define MIDI_FADER_MIN      0

#define PIN_BTN_STOP        0
#define PIN_BTN_PAUSE       1
#define PIN_BTN_TOPRIGHT    2
#define PIN_BTN_TOPMIDDLE   3
#define PIN_BTN_TOPLEFT     4
#define PIN_BTN_PLAY        5

#define PIN_FADER_IN        A6
#define PIN_FADER_OUT       A5

#define BTN_DEBOUNCE_TIME   50

#define ADR_EEPROM_CALIBRATION_LOW    10
#define ADR_EEPROM_CALIBRATION_HIGH   20

#define FADER_CALIBRATION_OFFSET_LOW  5
#define FADER_CALIBRATION_OFFSET_HIGH 5


Bounce btn_play       = Bounce(PIN_BTN_PLAY, BTN_DEBOUNCE_TIME);
Bounce btn_stop       = Bounce(PIN_BTN_STOP, BTN_DEBOUNCE_TIME);
Bounce btn_pause      = Bounce(PIN_BTN_PAUSE, BTN_DEBOUNCE_TIME);
Bounce btn_topright   = Bounce(PIN_BTN_TOPRIGHT, BTN_DEBOUNCE_TIME);
Bounce btn_topmiddle  = Bounce(PIN_BTN_TOPMIDDLE, BTN_DEBOUNCE_TIME);
Bounce btn_topleft    = Bounce(PIN_BTN_TOPLEFT, BTN_DEBOUNCE_TIME);

int16_t low;
int16_t high;

int16_t old_val;

void calibrate_fader (int16_t val) {
  if (val > high) {
    high = val;
    EEPROM.put(ADR_EEPROM_CALIBRATION_HIGH, high);
  }
  if (val < low) {
    low = val;
    EEPROM.put(ADR_EEPROM_CALIBRATION_LOW, low);
  }
}

void readFader() {
  digitalWrite(PIN_FADER_OUT, HIGH);
  delay(10);
  int16_t val = analogRead(PIN_FADER_IN);
  digitalWrite(PIN_FADER_OUT, LOW);

  calibrate_fader(val);
  
  val = map(val, low, high, MIDI_FADER_MIN - FADER_CALIBRATION_OFFSET_LOW, MIDI_FADER_MAX + FADER_CALIBRATION_OFFSET_HIGH);
  val = min(val, MIDI_FADER_MAX);
  val = max(val, MIDI_FADER_MIN);
  
  if ( val != old_val ) {
    usbMIDI.sendControlChange(MIDI_FADER_CHANNEL, val, MIDI_OUTPUT_CHANNEL);
  }
  old_val = val;
}

void btn_update(Bounce *btn, int note) {
  btn->update();

  if (btn->fallingEdge()) {
    usbMIDI.sendNoteOn(note, MIDI_VELOCITY_ON, MIDI_OUTPUT_CHANNEL);
  }
  if (btn->risingEdge()) {
    usbMIDI.sendNoteOn(note, MIDI_VELOCITY_OFF, MIDI_OUTPUT_CHANNEL);
  }
}

void setup_fader() {
  EEPROM.get(ADR_EEPROM_CALIBRATION_LOW, low);
  EEPROM.get(ADR_EEPROM_CALIBRATION_HIGH, high);
}

void setup() {
  setup_fader();

  pinMode(PIN_BTN_STOP, INPUT_PULLUP);
  pinMode(PIN_BTN_PAUSE, INPUT_PULLUP);
  pinMode(PIN_BTN_TOPRIGHT, INPUT_PULLUP);
  pinMode(PIN_BTN_TOPMIDDLE, INPUT_PULLUP);
  pinMode(PIN_BTN_TOPLEFT, INPUT_PULLUP);
  pinMode(PIN_BTN_PLAY, INPUT_PULLUP);

  pinMode(PIN_FADER_OUT, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  btn_update(&btn_play,      MIDI_NOTE_PLAY);
  btn_update(&btn_stop,      MIDI_NOTE_STOP);
  btn_update(&btn_pause,     MIDI_NOTE_PAUSE);
  btn_update(&btn_topright,  MIDI_NOTE_NEXT);
  btn_update(&btn_topmiddle, MIDI_NOTE_UNUSED1);
  btn_update(&btn_topleft,   MIDI_NOTE_PREVIOUS);

  readFader();

  while (usbMIDI.read()) {}
}
