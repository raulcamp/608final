#include "Arduino.h"
#include "Button.h"

Button::Button(int p) {
  flag = 0;  
  state = 0;
  pin = p;
  state_2_start_time = millis();
  button_change_time = millis(); 
  debounce_duration = 10;
  long_press_duration = 1000;
  button_pressed = 0;
}

void Button::read() {
  uint8_t button_state = digitalRead(pin);  
  button_pressed = !button_state;
}
int Button::update() {
  read();
  flag = 0;
  if (state==0) { // Unpressed, rest state
    if (button_pressed) {
      state = 1;
      button_change_time = millis();
    }
  } else if (state==1) { //Tentative pressed
    if (!button_pressed) {
      state = 0;
      button_change_time = millis();
    } else if (millis()-button_change_time >= debounce_duration) {
      state = 2;
      state_2_start_time = millis();
    }
  } else if (state==2) { // Short press
    if (!button_pressed) {
      state = 4;
      button_change_time = millis();
    } else if (millis()-state_2_start_time >= long_press_duration) {
      state = 3;
    }
  } else if (state==3) { //Long press
    if (!button_pressed) {
      state = 4;
      button_change_time = millis();
    }
  } else if (state==4) { //Tentative unpressed
    if (button_pressed && millis()-state_2_start_time < long_press_duration) {
      state = 2; // Unpress was temporary, return to short press
      button_change_time = millis();
    } else if (button_pressed && millis()-state_2_start_time >= long_press_duration) {
      state = 3; // Unpress was temporary, return to long press
      button_change_time = millis();
    } else if (millis()-button_change_time >= debounce_duration) { // A full button push is complete
      state = 0;
      if (millis()-state_2_start_time < long_press_duration) { // It is a short press
        flag = 1;
      } else {  // It is a long press
        flag = 2;
      }
    }
  }
  return flag;
}
