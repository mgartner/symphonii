#include <SoftwareSerial.h>
#include <Wiichuck.h>
#include "Wire.h"

// TODO: Are these necessary? If so, comment them - what do they do?
#define MAXANGLE 90
#define MINANGLE -90

// ===============================================================
// WiiChuck
// ===============================================================

// Initialize the WiiChuck protocol.
WiiChuck chuck = WiiChuck();

// TODO: Remove this if it works without it.
int angleStart, currentAngle;

// TODO: Remove this if it works without it.
int tillerStart = 0;

// TODO: Remove this if it works without it.
double angle;

// The last time a drum hit was recognized.
unsigned long previous_millis;

// Total number of drum readings to average.
// TODO: Calibrate this to make it recoginize drum hits.
const int num_drum_readings = 10;

// Array to hold the most recent drum accelerometer readings.
int drum_readings[num_drum_readings];

// The current index to store drum readings.
int drum_index = 0;

// The sum of all the drum readings.
int drum_total = 0;

// The average of all the drum readings.
int drum_average = 0;

// The threshold for reading a drum hit.
const int drum_threshold = 400;

// The minimum time allowed between recognizing drum hits.
// TODO: Calibrate this.
const long drum_interval = 100;

// ===============================================================
// MIDI
// ===============================================================

// Setup the RX and TX pins.
SoftwareSerial mySerial(2, 3);

// The MIDI note value to be played.
byte note = 0;

// Tied to VS1053 Reset line.
byte resetMIDI = 4;

// MIDI traffic indicator.
byte ledPin = 13;

// The instrument to play.
int instrument = 0;

/* 
 * Arduino setup() function.
 * Initializes the WiiChuck and the musical instrument shield.
 */
void setup() {

  // Serial connection for testing.
  Serial.begin(115200);

  // Initialize the WiiChuck.
  chuck.begin();
  chuck.update();

  // TODO: Do I need to calibrate the joy-stick?
  //chuck.calibrateJoy();

  // Set each value in the drum readings array to 0.
  for (int i = 0; i < num_drum_readings; i++) drum_readings[i] = 0;
  
  // Setup soft serial for MIDI control.
  mySerial.begin(31250);

  // Reset the VS1053.
  pinMode(resetMIDI, OUTPUT);
  digitalWrite(resetMIDI, LOW);
  delay(100);
  digitalWrite(resetMIDI, HIGH);
  delay(100);
  talkMIDI(0xB0, 0x07, 120);
}

/* 
 * Arduino loop() function.
 * Detects drum hits, and plays drum sounds.
 */
void loop() {

  // Update the Wiichuck readings.
  chuck.update();
  
  // Get the current time in millis.
  unsigned long current_millis = millis();
  
  // Make drum calculations.
  drum_total = drum_total - drum_readings[drum_index];
  drum_readings[drum_index] = (chuck.readAccelY() + chuck.readAccelZ()) / 2;
  drum_total = drum_total + drum_readings[drum_index];
  drum_average = drum_total / num_drum_readings;
  drum_index = (drum_index + 1) % num_drum_readings;
  
  // If the drum average reading is high enough, and the time since the last
  // drum hit is more than drum_interval, then recognize this as a valid drum
  // hit.
  if (drum_average > drum_threshold && (current_millis - previous_millis) > drum_interval) {

    // Save the current time.
    previous_millis = current_millis;

    // Set the instrument number.
    talkMIDI(0xC0, 38, 0);

    // Play a snare sound.
    noteOn(9, 38, 120);

    // Print a message to the serial socket.
    // TODO: Remove this. For testing only.
    Serial.println("BOOM");
    Serial.println(chuck.readRoll());
    Serial.println();
  }

  // TODO: check if the selected drum type has been changed.
}

/*
 * Send a MIDI note-on message.
 * Like pressing a piano key. Channel ranges from 0-15.
 */
void noteOn(byte channel, byte note, byte attack_velocity) {
  talkMIDI( (0x90 | channel), note, attack_velocity);
}

/*
 * Send a MIDI note-off message.
 * Like releasing a piano key.
 */
void noteOff(byte channel, byte note, byte release_velocity) {
  talkMIDI( (0x80 | channel), note, release_velocity);
}

/*
 * Plays a MIDI note.
 * Doesn't check to see that cmd is greater than 127, or that data values are
 * less than 127.
 */
void talkMIDI(byte cmd, byte data1, byte data2) {
  digitalWrite(ledPin, HIGH);
  mySerial.print(byte(cmd));
  mySerial.print(byte(data1));

  //Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes 
  //(sort of: http://253.ccarh.org/handout/midiprotocol/)
  if( (cmd & 0xF0) <= 0xB0)
    mySerial.print(byte(data2));

  digitalWrite(ledPin, LOW);
}
