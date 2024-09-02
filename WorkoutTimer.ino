/*
Created by: Quinton Odenthal
Date: 9/01/2024
*/

#include "pitches.h"
#include <TM1637.h>
#include "time.h"

// Adjustable tone variables
const int SETUP_TONE = NOTE_D6;
const int ACTIVE_TONE = NOTE_D7;
const int OFF_TONE = NOTE_B3;
const int ON_TONE = NOTE_C4;
const int SHORT_PRESS_TONE = A5;

// Digital display variables
const int CLK = 12;
const int DIO = 10;
TM1637 tm(CLK, DIO);

// Pin variables
const int BUZZER_PIN = 11;
const int BUTTON_PIN = 2;

// Delay variables
//const int MS_BUTTON_PRESSED_CHECK_DELAY = 100;
const unsigned long MS_SECOND = 1000;
const int MS_TONE_DURATION = 500;
const int MS_LONG_PRESS_DELAY = 500;
const int MS_SHORT_PRESS_DELAY = 100;
// const int MS_DISPLAY_UPDATE_DELAY = 10;

// Debounce variables
const unsigned long MS_DEBOUNCE_DELAY = 50;
unsigned long lastDebounceTime = 0;

// State variables
// Holds all the states of the loop. States can be in any order, of any int type, but 0 must always exist and be at index 0
const int LOOP_STATES[] = {0,1,2};
unsigned int loopStateIdx = 0; // Holds the index of the curent state 
int buttonState = HIGH; // LOW == Button pressed, HIGH == Button not pressed
int prevButtonState = HIGH; // Same as above
bool longPressActivated = false; // Flags when an input has been read on a given button press. Only want to allow one activation per press
bool shortPressActivated = false;

void resetPressActivated() {
  longPressActivated = false;
  shortPressActivated = false;
}

int getNextActiveStateIdx(int stateIdx)
{
  int stateArraySize = sizeof(LOOP_STATES) / sizeof(int);
  Serial.print("State Array Size: ");
  Serial.println(stateArraySize);
  if (stateIdx == stateArraySize - 1) { // Going from last active state
    return 1; // First active state
  } else if (stateIdx >= stateArraySize) { // Out of bounds starting index
    return 0; // Return the starting index, usually off
  } else {
    return stateIdx + 1;
  }
}

void setup() {
  // 4-digit display initialization
  tm.init();
  // Display brightness; 0-7
  tm.set(2);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  Serial.begin(9600); // BAUD rate
}

void loop() {
  // This button checking / debounce time logic is copied from Examples->Digitial->Debounce ##############
  // read the state of the switch into a local variable:
  int reading = digitalRead(BUTTON_PIN);

  // If the switch changed, due to noise or pressing:
  if (reading != prevButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > MS_DEBOUNCE_DELAY) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        Serial.println("Let off of button. Resetting button pressed flags");
        resetPressActivated();
      }
    }
    // End of the example code ################################################################

    // Checks if button has been pressed for long enough to be considered a long press
    if (buttonState == LOW && !longPressActivated && (millis() - lastDebounceTime) > MS_LONG_PRESS_DELAY) {

      Serial.println("Long Press Detected");

      if (loopStateIdx == 0) {
        tone(BUZZER_PIN, ON_TONE, MS_TONE_DURATION); // Buzzer plays to signal ON MODE
        loopStateIdx = getNextActiveStateIdx(loopStateIdx); // Change the state to the next active state
        Serial.print("New loop stateIdx: ");
        Serial.println(loopStateIdx);
        Serial.print("New loop state: ");
        Serial.println(LOOP_STATES[loopStateIdx]);
      }
      else // Active state -> Off State
      {
        tone(BUZZER_PIN, OFF_TONE, MS_TONE_DURATION); // Buzzer plays to signal OFF MODE
        loopStateIdx = 0;
      }
      longPressActivated = true; // Flag that a long press has been activated
    }
    // Not a long press, but long enough for a short press
    else if (buttonState == LOW && !shortPressActivated && (millis() - lastDebounceTime) > MS_SHORT_PRESS_DELAY) {

      tone(BUZZER_PIN, SHORT_PRESS_TONE, MS_TONE_DURATION); // Buzzer plays to signal OFF MODE

      Serial.println("Short Press Detected");

      if (loopStateIdx != 0) {
        loopStateIdx = getNextActiveStateIdx(loopStateIdx); // Change the state to the next active state
        resetTimerVariables();
        Serial.print("New loop stateIdx: ");
        Serial.println(loopStateIdx);
        Serial.print("New loop state: ");
        Serial.println(LOOP_STATES[loopStateIdx]);
      }
      
      shortPressActivated = true; // Flag that a short press has been activated
    }
  }

  switch (LOOP_STATES[loopStateIdx]) {
    case 0: // Off state
      offState();
      break;
    case 1: // 10sec setup + 60sec active time
      alternatingTimerMode(10 * MS_SECOND, 60 * MS_SECOND);
      break;
    case 2: // 5sec setup + 10sec active time
      alternatingTimerMode(5 * MS_SECOND, 10 * MS_SECOND);
      break;
    default: // Invalid loopState
      loopStateIdx = 0;
      resetTimerVariables();
      turnOffDisplay();
      break;
  }

  // This is from the example code as well
  // save the reading. Next time through the loop, it'll be the prevButtonState:
  prevButtonState = reading;
  // End example code
}

// Time keeping variables used by the different timer modes
unsigned long msStartTime = 0;
unsigned long msTimeElapsed = 0;
unsigned long msDesiredDelay = 0;

void resetTimerVariables() {
  msStartTime = 0;
  msTimeElapsed = 0;
  msDesiredDelay = 0;
}

void alternatingTimerMode(unsigned long SETUP_TIME, unsigned long ACTIVE_TIME) {
  if (msDesiredDelay == 0) { // Just entering this timer mode
    msDesiredDelay = SETUP_TIME;
    msStartTime = millis();
    tm.point(true); // Turns on the middle colon
  }
  displayTime(msDesiredDelay - msTimeElapsed); // Displays the time left before the delay is over
  msTimeElapsed = millis() - msStartTime; // Update the time elapsed in this delay

  if (msTimeElapsed >= msDesiredDelay) { // Desired Time has been met, swap to the next timer mode
    if (msDesiredDelay == SETUP_TIME) {
      tone(BUZZER_PIN, ACTIVE_TONE, MS_TONE_DURATION); // Buzzer plays to signal start of ACTIVE time
      msDesiredDelay = ACTIVE_TIME;
    } else {
      tone(BUZZER_PIN, SETUP_TONE, MS_TONE_DURATION); // Buzzer plays to signal start of set-up time
      msDesiredDelay = SETUP_TIME;
    }
    msStartTime = millis();
    msTimeElapsed = 0;
  }
}

void offState() {
  turnOffDisplay();
  resetTimerVariables();
}

// Displays the given time (ms) as seconds and hundreths of seconds
void displayTime(unsigned long msDisplayTime)
{
  tm.display(0, (msDisplayTime / 10000) % 10); // First digit; Tens of seconds
  tm.display(1, (msDisplayTime / 1000) % 10); // Second digit; Seconds
  tm.display(2, (msDisplayTime / 100) % 10); // Third digit; Tenths of seconds
  tm.display(3, (msDisplayTime / 10) % 10); // Fourth digit; Hundreths of seconds
}

void turnOffDisplay()
{
    // Stop the display
    tm.point(false); // Turns off the middle colon
    tm.clearDisplay();
}