/*
Created by: Quinton Odenthal
Date: 8/25/2024
*/

#include "pitches.h"
#include <TM1637.h>
#include "time.h"

// Adjustable tone variables
const int SETUP_TONE = NOTE_D6;
const int ACTIVE_TONE = NOTE_D7;
const int OFF_TONE = NOTE_B3;

// Digital display variables
const int CLK = 12;
const int DIO = 10;
TM1637 tm(CLK, DIO);

// Pin variables
const int BUZZER_PIN = 11;
const int BUTTON_PIN = 2;

// Delay variables
const int MS_BUTTON_PRESSED_CHECK_DELAY = 100;
const unsigned long MS_SECOND = 1000;
const int SEC_SETUP_TIME = 10;
const int SEC_ACTIVE_TIME = 60;
const int MS_TONE_DURATION = 500;
const int MS_DISPLAY_UPDATE_DELAY = 10;

bool onState = false; // True when in running state, False when in off state

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
  // Loop that waits for button to be pressed
  while (onState)
  {
    // Set-up time
    tone(BUZZER_PIN, SETUP_TONE, MS_TONE_DURATION); // Buzzer plays to signal start of set-up time
    delayWithButtonPressedCheck(SEC_SETUP_TIME * MS_SECOND); // Set-up delay
    if (!onState) break;

    // Active time
    tone(BUZZER_PIN, ACTIVE_TONE, MS_TONE_DURATION); // Buzzer plays to signal start of active time
    delayWithButtonPressedCheck(SEC_ACTIVE_TIME * MS_SECOND); // Active delay
    if (!onState) break;
  }

  delay(MS_BUTTON_PRESSED_CHECK_DELAY);
  if (digitalRead(BUTTON_PIN) == LOW) swapState();
}


// This function implements the desired delay (ms) while constantly checking for button input and updating the display
void delayWithButtonPressedCheck(unsigned long msDesiredDelay)
{
  unsigned long START_TIME = millis(); // Marks the start of the delay
  unsigned long TIME_ELAPSED = 0; // Tracks the time elapsed in the current delay
  while (TIME_ELAPSED <= msDesiredDelay)
  {
    // TODO Add a little loop here that updates the display at the desired rate whil maintaining the desired delay before checking the button again
    displayTime(msDesiredDelay - TIME_ELAPSED); // Displays the time left before the delay is over
    delay(MS_BUTTON_PRESSED_CHECK_DELAY);
  
    // If the button is pressed, flip the on state, end the loop
    if (digitalRead(BUTTON_PIN) == LOW)
    {
      swapState();
      Serial.println("Broke from delay loop");
      break;
    }

    TIME_ELAPSED = millis() - START_TIME; // Update the time elapsed in this delay
  }
}


// Displays the given time (ms) as seconds and hundreths of seconds
void displayTime(unsigned long msDisplayTime)
{
  tm.display(0, (msDisplayTime / 10000) % 10); // First digit; Tens of seconds
  tm.display(1, (msDisplayTime / 1000) % 10); // Second digit; Seconds
  tm.display(2, (msDisplayTime / 100) % 10); // Third digit; Tenths of seconds
  tm.display(3, (msDisplayTime / 10) % 10); // Fourth digit; Hundreths of seconds
}

// This function handles the switching from on to off state
void swapState()
{
  onState = onState == false; // Swaps the boolean state value

  if (onState) // Switching from off to on
  {
    // Start the display
    tm.point(true); // Turns on the middle colon
  }
  else // Switching from on to off
  {
    tone(BUZZER_PIN, OFF_TONE, MS_TONE_DURATION); // Buzzer plays to signal turn off

    // Stop the display
    tm.point(false); // Turns off the middle colon
    tm.clearDisplay();
  }
}