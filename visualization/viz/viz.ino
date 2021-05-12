#include <string.h>  //used for some string handling and processing.


// MIC

const int UPDATE_PERIOD = 20;
const uint8_t PIN_1 = 5; //button 1
uint32_t primary_timer;
float sample_rate = 2000; //Hz
float sample_period = (int)(1e6 / sample_rate);

uint16_t raw_reading;
uint16_t scaled_reading_for_green;
uint16_t scaled_reading_for_red;
uint16_t scaled_reading_for_blue;

uint16_t GREEN_LO = 0;
uint16_t GREEN_HI = 2048;
uint16_t RED_LO   = 1024;
uint16_t RED_HI   = 3072;
uint16_t BLUE_LO  = 2048;
uint16_t BLUE_HI  = 4096;

uint16_t BEAT_THRESHOLD = 2048;
//uint16_t BEAT_THRESHOLD = 2048 + 256;

const uint8_t RED = 0;
const uint8_t ORANGE = 1;
const uint8_t YELLOW = 2;
const uint8_t GREEN = 3;
const uint8_t BLUE = 4;
const uint8_t INDIGO = 5;
const uint8_t VIOLET = 6;

const uint8_t NUM_COLORS = 7;
int8_t color_index;
int cooldown;

const int MAX_COLOR_VALUE = 255;

// RGB LED Stuff
const uint32_t G_PWM_CHANNEL = 3;
const uint32_t R_PWM_CHANNEL = 1;
const uint32_t B_PWM_CHANNEL = 2;

const uint32_t G_PIN = 25;
const uint32_t R_PIN = 26;
const uint32_t B_PIN = 27;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  
  // PWM Setup
  ledcSetup(G_PWM_CHANNEL, 75, 8);
  ledcAttachPin(G_PIN, G_PWM_CHANNEL);

  ledcSetup(R_PWM_CHANNEL, 75, 8);
  ledcAttachPin(R_PIN, R_PWM_CHANNEL);

  ledcSetup(B_PWM_CHANNEL, 75, 8);
  ledcAttachPin(B_PIN, B_PWM_CHANNEL);

  // PINs
  pinMode(G_PIN, OUTPUT);
  pinMode(R_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);

  scaled_reading_for_green = 0;
  scaled_reading_for_red = 0;
  scaled_reading_for_blue = 0;

  ledcWrite(G_PWM_CHANNEL, scaled_reading_for_green);
  ledcWrite(R_PWM_CHANNEL, scaled_reading_for_red);
  ledcWrite(B_PWM_CHANNEL, scaled_reading_for_blue);

  color_index = -1;
  cooldown = 0;
}

void loop() {
//  while (!digitalRead(PIN_1)) {
  raw_reading = analogRead(A0);
  Serial.println(raw_reading);

  if (raw_reading > BEAT_THRESHOLD && cooldown == 0) {
    color_index = (color_index + 1) % NUM_COLORS;
    set_colors(color_index);
    cooldown = 100;
  } else {
    if (cooldown % 3 == 0) decay_colors();
    cooldown--;
    if (cooldown < 0) cooldown = 0;
  }

  write_colors();
  while (micros() > primary_timer && micros() - primary_timer < sample_period); //prevents rollover glitch every 71 minutes...not super needed
  primary_timer = micros();

}

//void write_green(uint16_t raw_reading) {
//  if (raw_reading < 1024) {
//    scaled_reading_for_green = 0;
//  } else {
//    scaled_reading_for_green = raw_reading - 1024;
//  }
//
//  if (scaled_reading_for_green > 2572) {
//    scaled_reading_for_green = 2572;
//  }
//
//  scaled_reading_for_green  = scaled_reading_for_green * MAX_COLOR_VALUE / 2572;
//
//  Serial.println(scaled_reading_for_green);
//
// ledcWrite(G_PWM_CHANNEL, scaled_reading_for_green);
//}

bool in_thresh(int val, int low, int high) {
  return (val >= low && val < high);
}

void set_colors(uint8_t COLOR) {
  switch (COLOR) {
    case RED:
      scaled_reading_for_red   = 255;
      scaled_reading_for_blue  = 0;
      scaled_reading_for_green = 0;
      break;
    case ORANGE:
      scaled_reading_for_red   = 255;
      scaled_reading_for_blue  = 0;
      scaled_reading_for_green = 128;
      break;
    case YELLOW:
      scaled_reading_for_red   = 255;
      scaled_reading_for_blue  = 0;
      scaled_reading_for_green = 255;
      break;
    case GREEN:
      scaled_reading_for_red   = 0;
      scaled_reading_for_blue  = 0;
      scaled_reading_for_green = 255;
      break;
    case BLUE:
      scaled_reading_for_red   = 0;
      scaled_reading_for_blue  = 255;
      scaled_reading_for_green = 0;
      break;
    case INDIGO:
      scaled_reading_for_red   = 75;
      scaled_reading_for_blue  = 130;
      scaled_reading_for_green = 0;
      break;
    case VIOLET:
      scaled_reading_for_red   = 255;
      scaled_reading_for_blue  = 255;
      scaled_reading_for_green = 0;
      break;
  }
}

void decay_colors() {
  if (scaled_reading_for_red > 0) {
    scaled_reading_for_red--;
  }

  if (scaled_reading_for_blue > 0) {
    scaled_reading_for_blue--;
  }

  if (scaled_reading_for_green > 0) {
    scaled_reading_for_green--;
  }
}

void write_colors() {
  ledcWrite(G_PWM_CHANNEL, scaled_reading_for_green);
  ledcWrite(R_PWM_CHANNEL, scaled_reading_for_red);
  ledcWrite(B_PWM_CHANNEL, scaled_reading_for_blue);
}

void write_green(uint16_t raw_reading) {
  if (in_thresh(raw_reading, GREEN_LO, GREEN_HI)) {
    scaled_reading_for_green = (raw_reading - GREEN_LO) * MAX_COLOR_VALUE / (GREEN_HI - GREEN_LO);
  } else {
    scaled_reading_for_green = 0;
  }

  Serial.println(scaled_reading_for_green);

 ledcWrite(G_PWM_CHANNEL, scaled_reading_for_green);
}

void write_red(uint16_t raw_reading) {
  if (in_thresh(raw_reading, RED_LO, RED_HI)) {
    scaled_reading_for_red = (raw_reading - RED_LO) * MAX_COLOR_VALUE / (RED_HI - RED_LO);
  } else {
    if (scaled_reading_for_red > 0) {
      scaled_reading_for_red--;
    }
  }

  Serial.println(scaled_reading_for_red);

 ledcWrite(R_PWM_CHANNEL, scaled_reading_for_red);
}

void write_blue(uint16_t raw_reading) {
  if (in_thresh(raw_reading, BLUE_LO, BLUE_HI)) {
    scaled_reading_for_blue = (raw_reading - BLUE_LO) * MAX_COLOR_VALUE / (BLUE_HI - BLUE_LO);
  } else {
    if (scaled_reading_for_blue > 0) {
      scaled_reading_for_blue--;
    }
  }

  Serial.println(scaled_reading_for_blue);

 ledcWrite(B_PWM_CHANNEL, scaled_reading_for_blue);
}
