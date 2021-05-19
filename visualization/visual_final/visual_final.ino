#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include<math.h>
#include<string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp32-hal.h"

TFT_eSPI tft = TFT_eSPI();
char network[] = "Hinckley 1";
char password[] = "StayHinckley1";

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t OUT_BUFFER_SIZE = 20000; //size of buffer to hold HTTP response
char old_response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
float beats[1000];
float segment_times[5000];
uint8_t pitches[5000];
char key[20];

// MIC

const int UPDATE_PERIOD = 20;
const int BUTTON1_PIN = 5;
const int BUTTON2_PIN = 19;
uint32_t primary_timer;
float sample_rate = 2000; //Hz
float sample_period = (int)(1e6 / sample_rate);

uint16_t raw_reading;
uint16_t scaled_reading_for_green;
uint16_t scaled_reading_for_red;
uint16_t scaled_reading_for_blue;

uint16_t BEAT_THRESHOLD = 2048;
//uint16_t BEAT_THRESHOLD = 2048 + 256;

typedef struct {
  int r;
  int g;
  int b;
} rgb_color;

rgb_color cur_rgb_color;

rgb_color red = {255,0,0};
rgb_color orange = {255,128,0};
rgb_color yellow = {255,255,0};
rgb_color green = {0,255,0};
rgb_color blue = {0,0,255};
rgb_color indigo = {75,0,130};
rgb_color violet = {255,0,255};
rgb_color white = {255,255,255};

const float UP_THRESH = 1.4;
const float MID_THRESH = 0.7;

// color palettes to be used based on features of song
rgb_color HI_COLORS[5] = {red, orange, yellow, green, blue};
rgb_color RAINBOW[7] = {red, orange, yellow, green, blue, indigo, violet};
rgb_color LO_COLORS[4] = {green, blue, indigo, violet};

rgb_color* color_palettes[3] = {HI_COLORS, RAINBOW, LO_COLORS};
int palet_sizes[3] = {5, 7, 4};

rgb_color* color_palette;

int num_colors = 7;
int8_t color_index;
int cooldown;
uint8_t decay_rate = 1;

const int FAST_COOLDOWN = 150;
const int MID_COOLDOWN = 300;
const int SLOW_COOLDOWN = 500;

int cur_cooldown = MID_COOLDOWN;

const int MAX_COLOR_VALUE = 255;

// RGB LED Stuff
const uint32_t G_PWM_CHANNEL = 2;
const uint32_t R_PWM_CHANNEL = 1;
const uint32_t B_PWM_CHANNEL = 3;

const uint32_t G_PIN = 27;
const uint32_t R_PIN = 14;
const uint32_t B_PIN = 12;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200); //for debugging if needed.
  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  
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

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  scaled_reading_for_green = 0;
  scaled_reading_for_red = 0;
  scaled_reading_for_blue = 0;

  ledcWrite(G_PWM_CHANNEL, scaled_reading_for_green);
  ledcWrite(R_PWM_CHANNEL, scaled_reading_for_red);
  ledcWrite(B_PWM_CHANNEL, scaled_reading_for_blue);

  color_index = -1;
  cooldown = 0;
  
  reset_color_palette();
  
}

void loop() {
  if (!digitalRead(BUTTON1_PIN)) {
    set_color_palette();
  }
  if (!digitalRead(BUTTON2_PIN)) {
    reset_color_palette();
  }
  raw_reading = analogRead(A0);
//  Serial.println(raw_reading);

  if (raw_reading > BEAT_THRESHOLD && cooldown == 0) {
    color_index = (color_index + 1) % num_colors;
    cur_rgb_color = color_palette[color_index];
    set_colors(cur_rgb_color);
    cooldown = cur_cooldown;
  } else {
    if (cooldown % 3 == 0) {
      decay_colors();
    }
    cooldown--;
    if (cooldown < 0) cooldown = 0;
  }

  write_colors();
  while (micros() > primary_timer && micros() - primary_timer < sample_period); //prevents rollover glitch every 71 minutes...not super needed
  primary_timer = micros();

}

void lookup(char* response, int response_size, char* key) {
  char request_buffer[200];
  sprintf(request_buffer, "GET /sandbox/sc/team65/dan/get_spotify_data.py?%s HTTP/1.1\r\n", key);
  strcat(request_buffer, "Host: 608dev-2.net\r\n");
  strcat(request_buffer, "\r\n"); //new line from header to body

  do_http_request("608dev-2.net", request_buffer, response, response_size, RESPONSE_TIMEOUT, true);
}

void reset_color_palette() {
  Serial.println("Reset");
  color_palette = RAINBOW;
  num_colors = 7;
  color_index = 0;
}

void set_color_palette() {
  sprintf(key, "color_metric");
  memset(response, 0, sizeof(response));
  lookup(response, sizeof(response), key);
  float color_metric = atof(response);
  int idx;
  if (color_metric > UP_THRESH) {
    color_palette = color_palettes[0];
    num_colors = palet_sizes[0];
    cur_cooldown = FAST_COOLDOWN;
  } else if (color_metric > MID_THRESH) {
    color_palette = color_palettes[1];
    num_colors = palet_sizes[1];
    Serial.println(num_colors);
    cur_cooldown = MID_COOLDOWN;
  } else {
    color_palette = color_palettes[2];
    num_colors = palet_sizes[2];
    Serial.println(num_colors);
    cur_cooldown = SLOW_COOLDOWN;
  }
  color_index = 0;
  Serial.println(color_metric);
}

bool in_thresh(int val, int low, int high) {
  return (val >= low && val < high);
}

void set_colors(rgb_color COLOR) {
  scaled_reading_for_red   = COLOR.r;
  scaled_reading_for_blue  = COLOR.b;
  scaled_reading_for_green = COLOR.g;
}

void decay_colors() {
  if (scaled_reading_for_red > 0) {
    scaled_reading_for_red -= decay_rate;
  }
  if (scaled_reading_for_blue > 0) {
    scaled_reading_for_blue -= decay_rate;
  }
  if (scaled_reading_for_green > 0) {
    scaled_reading_for_green -= decay_rate;
  }
}

void write_colors() {
  ledcWrite(G_PWM_CHANNEL, scaled_reading_for_green);
  ledcWrite(R_PWM_CHANNEL, scaled_reading_for_red);
  ledcWrite(B_PWM_CHANNEL, scaled_reading_for_blue);
}
