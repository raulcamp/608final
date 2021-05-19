#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <string.h>
#include "Button.h"
#include "HeartbeatSensor.h"
#include <DFRobot_MAX30102.h>

DFRobot_MAX30102 particleSensor;

TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
// TODO: change pins depending on board config
const int BUTTON_PIN1 = 5; 
const int BUTTON_PIN2 = 19;
const int BUTTON_PIN3 = 3;

const int LOOP_PERIOD = 40;
const int MAX_GROUPS = 100;
int state;
int group_state;
int option_state;
int count;
int num_groups;
const int idle = 0;
const int song_menu = 1;
const int vis_menu = 2;
const int idle_menu = 3;
const int groups_menu = 4;
const int pulse_recommendation = 5;
const int recommended_song = 6;
int state_change;

#define IDLE 0
#define PRESSED 1

char network[] = "MIT GUEST"; 
char password[] = "";
/* Having network issues since there are 50 MIT and MIT_GUEST networks?. Do the following:
    When the access points are printed out at the start, find a particularly strong one that you're targeting.
    Let's say it is an MIT one and it has the following entry:
   . 4: MIT, Ch:1 (-51dBm)  4:95:E6:AE:DB:41
   Do the following...set the variable channel below to be the channel shown (1 in this example)
   and then copy the MAC address into the byte array below like shown.  Note the values are rendered in hexadecimal
   That is specified by putting a leading 0x in front of the number. We need to specify six pairs of hex values so:
   a 4 turns into a 0x04 (put a leading 0 if only one printed)
   a 95 becomes a 0x95, etc...
   see starting values below that match the example above. Change for your use:
   Finally where you connect to the network, comment out
     WiFi.begin(network, password);
   and uncomment out:
     WiFi.begin(network, password, channel, bssid);
   This will allow you target a specific router rather than a random one!
*/
uint8_t channel = 1; //network channel on 2.4 GHz
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; //6 byte MAC address of AP you're targeting.

char host[] = "608dev-2.net";
// input spotify or custom username
char username[] = "maker";
// input user oath token (get temporary token here: https://developer.spotify.com/console/get-users-currently-playing-track/?market=&additional_types=)
// when selecting scopes, in addition to 'user-read-currently-playing' please also check 'user-modify-playback-state' so that you can also play shared songs
// tokens expire pretty quickly so you may need to get a new token
char SPOTIFY_OATH_TOKEN[] = "BQAxs0DQlzkMjbBpB5d2TMuy4oaxm0mXMKPunzwoyTT7dThnH4kmvqMnLwNrUExTedvC9n0hI4_rj_1QqMjHkcgKo4Q6_QYJoAHWICrIDXibu-dBW216xIWWNxHiEMX1WpAwhgSvxGwMFCSlShTJGA";

// AUDIO VISUALIZATION

float beats[1000];
float segment_times[5000];
uint8_t pitches[5000];
char key[20];

uint16_t rawReading;

float sample_rate = 2000; //Hz
float sample_period = (int)(1e6 / sample_rate);

uint16_t raw_reading;
uint16_t scaled_reading_for_green;
uint16_t scaled_reading_for_red;
uint16_t scaled_reading_for_blue;

uint16_t BEAT_THRESHOLD = 2048;
//uint16_t BEAT_THRESHOLD = 2048 - 512;

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

const int FAST_COOLDOWN = 15;
const int MID_COOLDOWN = 30;
const int SLOW_COOLDOWN = 50;

int cur_cooldown = FAST_COOLDOWN;

const int MAX_COLOR_VALUE = 255;

// RGB LED Stuff
const uint32_t G_PWM_CHANNEL = 2;
const uint32_t R_PWM_CHANNEL = 1;
const uint32_t B_PWM_CHANNEL = 3;

const uint32_t G_PIN = 25;
const uint32_t R_PIN = 26;
const uint32_t B_PIN = 27;

const int NOTIF_FETCH_TIME = 20000;

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t IN_BUFFER_SIZE = 2000;
const uint16_t OUT_BUFFER_SIZE = 2000; //size of buffer to hold HTTP response
char request[IN_BUFFER_SIZE];
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request

char recSongBuffer[OUT_BUFFER_SIZE];
char songBuffer[OUT_BUFFER_SIZE];
char songURI[OUT_BUFFER_SIZE];

char groups[MAX_GROUPS][80];
char invite[1000];
char shared[1000];
char liked[1000];

unsigned long timer;

uint8_t old_reading; //for remember button value and detecting edges

Button left(BUTTON_PIN1);
Button middle(BUTTON_PIN2);
Button right(BUTTON_PIN3);

HeartbeatSensor hbs;
int bpm;
bool usingPulse;

unsigned long primary_timer;

void setup() {

  int tryCounter = 0;
  usingPulse = true;
  while (!particleSensor.begin() && tryCounter < 5) {
    Serial.println("MAX30102 was not found");
    tryCounter++;
    delay(1000);
  }

  if (tryCounter >= 5) usingPulse = false;

  
  Serial.begin(115200); //for debugging if needed.
  delay(100);

  setupWifi();

  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1.75);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to red foreground, black background
  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  timer = millis();
  state = idle;
  option_state = 0;
  group_state = 0;
  invite[0] = '\0';
  shared[0] = '\0';
  liked[0] = '\0';
  state_change = 0; // false

  particleSensor.sensorConfiguration(/*ledBrightness=*/60, /*sampleAverage=*/SAMPLEAVG_8, \
                                  /*ledMode=*/MODE_MULTILED, /*sampleRate=*/SAMPLERATE_400, \
                                  /*pulseWidth=*/PULSEWIDTH_411, /*adcRange=*/ADCRANGE_16384);

  // VIZ SETUP
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
  
  reset_color_palette();

  hbs = HeartbeatSensor();

  primary_timer = millis();
  state_change = true;
}


void loop() {
  // button input
  int leftReading = left.update();
  int middleReading = middle.update();
  int rightReading = right.update();

  // pulse reading
  if (usingPulse) {
    bpm = hbs.update(particleSensor.getIR());
  }

  // audio
  rawReading = analogRead(A0);
  visualizeMusic(rawReading);

  handleDisplay(leftReading, middleReading, rightReading);
}

void handleDisplay(int leftReading, int middleReading, int rightReading) {
  fetchNotifications();
  
  tft.setCursor(0,0,1); // reset cursor at the top of the screen

  if (state_change) {
    tft.fillScreen(TFT_BLACK);
    state_change = false;
  }

  switch(state){

    case idle: //Menu State
        idleState(leftReading, middleReading, rightReading);
        break;
    case song_menu: //Here the user has option to fetch vis or display song name
        songMenuState(leftReading, middleReading, rightReading);
        break;
    case vis_menu: //Vis menu
        visMenuState(leftReading, middleReading, rightReading);
        break;  
    case groups_menu: // Groups menu
        groupsMenuState(leftReading, middleReading, rightReading);
        break;
    case pulse_recommendation:
        pulseRecommendationState(leftReading, middleReading, rightReading);
        break;
    case recommended_song:
        recommendedSongState(leftReading, middleReading, rightReading);
        break;
  }
}

void fetchNotifications() {
  if (millis() - timer > NOTIF_FETCH_TIME) {
    request[0] = '\0'; //set 0th byte to null
    sprintf(request, "GET http://608dev-2.net/sandbox/sc/team65/raul/final_project_server_code.py?action=get_invites&username=%s\r\n",username);
    strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
    strcat(request, "\r\n"); //add blank line!
    do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
    Serial.println(response);
    if (strcmp(response,"NO INVITES") != 0) {
      strcat(invite, response);
      Serial.println("HERE");
      state_change = 1;
    } else {
      request[0] = '\0'; //set 0th byte to null
      sprintf(request, "GET http://608dev-2.net/sandbox/sc/team65/raul/final_project_server_code.py?action=get_shared&username=%s\r\n",username);
      strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
      strcat(request, "\r\n"); //add blank line!
      do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
      if (strcmp(response,"NO SHARED") != 0) {
        strcat(shared, response);
        state_change = 1;
      } 
    }
    
    timer = millis();
  }
}

//////////////////////
/////// STATES ///////
//////////////////////

// IDLE

void idleState(int leftReading, int middleReading, int rightReading) {
  tft.println("Hello!\nIf you are currently listening to a song,\nthen press the left button to display song info.");

  state_change = false;

  if(leftReading){ //button 1 is pressed --> displays song on lcd 
  // send request and retrieve song
      state = song_menu;
      state_change = true;

      tft.println("Requesting Song...");
  }
}

// SONG MENU

void songMenuState(int leftReading, int middleReading, int rightReading) {


  // Song Title/Artist display
  tft.println(songBuffer);

  // handle state changes
  if (state_change == 1) {
    tft.fillRect(0, 15, 127, 114, TFT_BLACK);
    state_change = 0;
  }

  displayBPM();
  
  if (invite[0] != '\0') {
    handleInvite(leftReading, rightReading);
  } else if (shared[0] != '\0') {
    handleShared(leftReading, rightReading);
  } else { // Song Menu
    
    //tft.setCursor(1, 15);
    tft.println("\nPress left -> \n       Sync Viz");
    //tft.setCursor(1, 48);
    tft.println("\nLong Press left -> \n   BPM-based Song Rec");
    //tft.setCursor(1, 81);
    tft.println("\nPress middle -> \n       Like Song");
    //tft.setCursor(1, 114, 1);
//    tft.println("\n\nStep Count: INSERT STEP COUNT MODULE HERE");
    tft.println("\nPress right -> \n       Share Song");
    
    tft.setCursor(0,0,1); // reset cursor at the top of the screen
    
    if(leftReading == 1){ // sync visualization
      state = vis_menu;
      tft.fillScreen(TFT_BLACK);
      count = 0;

    }
    else if(leftReading == 2) {
      state = pulse_recommendation;
      tft.fillScreen(TFT_BLACK);
      count = 0;
    }
    else if(middleReading){ // Like song
      tft.fillScreen(TFT_BLACK);
      tft.println("Liking Song...");
      count = 0;
      delay(2000);
      tft.fillScreen(TFT_BLACK);
      initOptions();
      state = groups_menu;
    }
    else if(rightReading){ // Share song with current session
      tft.fillScreen(TFT_BLACK);
      tft.println("Sharing Song...");
      count = 1;
      delay(2000);
      tft.fillScreen(TFT_BLACK);
      initOptions();
      state = groups_menu;
    }
  }
}

void handleInvite(int leftReading, int rightReading) {
  char output[80];
  sprintf(output, "\n\nNew invite: ->You have been invited to %s.", response);
  tft.setCursor(1, 15, 2);
  tft.println(output);
  tft.setCursor(1, 114, 1);

  if (leftReading) {
    char body[100];
    sprintf(body,"action=rejected_join&username=%s&group_name=%s", username, invite);
    int body_len = strlen(body);
    sprintf(request,"POST http://608dev-2.net/sandbox/sc/team65/raul/final_project_server_code.py HTTP/1.1\r\n");
    strcat(request,"Host: 608dev-2.net\r\n");
    strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request+strlen(request),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
    strcat(request,"\r\n"); //new line from header to body
    strcat(request,body); //body
    strcat(request,"\r\n"); //new line
    do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
    invite[0] = '\0';
    state_change = 1;
  }

  if (rightReading) {
    char body[100];
    sprintf(body,"action=invited_join&username=%s&group_name=%s", username, invite);
    int body_len = strlen(body);
    sprintf(request,"POST http://608dev-2.net/sandbox/sc/team65/raul/final_project_server_code.py HTTP/1.1\r\n");
    strcat(request,"Host: 608dev-2.net\r\n");
    strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request+strlen(request),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
    strcat(request,"\r\n"); //new line from header to body
    strcat(request,body); //body
    strcat(request,"\r\n"); //new line
    do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
    invite[0] = '\0';
    state_change = 1;
  }
}

void handleShared(int leftReading, int rightReading) {
  char output[80];
  sprintf(output, "\n\nNew shared song: ->The song %s has been shared with you.", response);
  
  tft.setCursor(1, 15, 2);
  tft.println(output);
  tft.setCursor(1, 114, 1);

  if(leftReading) {
    shared[0] = '\0';
    state_change = 1;
  }
  if (rightReading) {
    shared[0] = '\0';
    tft.println("\nRequesting Song...");
    playSong();
    tft.fillScreen(TFT_BLACK);
    state_change = 1;
  }
}

void handleLiked(int rightReading) {
  char output[80];  
  sprintf(output, "\n\nThe song %s is popular in your group! Do you want to listen to it?", response);
  
  tft.setCursor(1, 15, 2);
  tft.println(output);
  tft.setCursor(1, 114, 1);
  
  if (rightReading || response[0] == 'Z') {
    liked[0] = '\0';
    state_change = 1;
  }
}

// VISUALIZATION MENU

void visMenuState(int leftReading, int middleReading, int rightReading) {
  tft.fillScreen(TFT_BLACK);
  tft.println("Syncing...");
  syncMusic();
  state = song_menu;
}

// GROUPS MENU

void groupsMenuState(int leftReading, int middleReading, int rightReading) {
  if (leftReading) { // scroll next group
    option_state = (option_state + 1) % num_groups;
    nextOption(option_state);
  
  }
  else if (middleReading) { // confirm group
    if (count == 0) {
        char body[100];
        sprintf(body,"action=like&username=%s&group_name=%s&song=%s", username, groups[option_state], songURI);
        int body_len = strlen(body);
        sprintf(request,"POST http://608dev-2.net/sandbox/sc/team65/raul/final_project_server_code.py HTTP/1.1\r\n");
        strcat(request,"Host: 608dev-2.net\r\n");
        strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(request+strlen(request),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        strcat(request,"\r\n"); //new line from header to body
        strcat(request,body); //body
        strcat(request,"\r\n"); //new line
        do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
    } else {
        char body[100];
        sprintf(body,"action=share&username=%s&group_name=%s&song=%s", username, groups[option_state], songURI);
        int body_len = strlen(body);
        sprintf(request,"POST http://608dev-2.net/sandbox/sc/team65/raul/final_project_server_code.py HTTP/1.1\r\n");
        strcat(request,"Host: 608dev-2.net\r\n");
        strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(request+strlen(request),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        strcat(request,"\r\n"); //new line from header to body
        strcat(request,body); //body
        strcat(request,"\r\n"); //new line
        do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
    }
  } else if (rightReading) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0,0,1);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    char out[100];
    if (count == 0) {
        sprintf(out, "Sent liked songs to groups");
    } else {
        sprintf(out, "Sent shared songs to groups");
    }
    tft.println(out);
    delay(2000);
    tft.fillScreen(TFT_BLACK);
    state = song_menu;
    option_state = 0;
  }
}

// HEARTRATE SONG RECOMMENDATION

void pulseRecommendationState(int leftReading, int middleReading, int rightReading) {
  state_change = false;
  tft.println(" !");
  getSongByBPM(bpm);
  state = recommended_song;
  state_change = true;
}

void displayBPM() {
  if (bpm > 45) {
    tft.printf("\nHeart BPM: %i\n", bpm);
  } else {
    tft.printf("\nSearching for Heartbeat...\n", bpm);
  }
}

// RECOMMENDED SONG

void recommendedSongState(int leftReading, int middleReading, int rightReading) {
  state_change = false;
  tft.println(recSongBuffer);
  tft.println("\n Press left button to go back to home.");

  if (leftReading) {
    tft.fillScreen(TFT_BLACK);
    tft.println("Going back...");
    state = song_menu;
    state_change = true;
  }
}

void getSongByBPM(int bpm) {
  memset(response, 0, OUT_BUFFER_SIZE);
  memset(request, 0, sizeof(request));
  memset(recSongBuffer, 0, OUT_BUFFER_SIZE);
  // http://608dev-2.net/sandbox/sc/team65/michael/bpm.py?bpm=65
  sprintf(request, "GET http://608dev-2.net/sandbox/sc/team65/michael/bpm.py?bpm=%i HTTP/1.1\r\n",bpm);
  strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
  strcat(request, "\r\n"); //add blank line!
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  strcpy(recSongBuffer, response);
}

// VISUALIZATION

void visualizeMusic(uint16_t raw_reading) {
  if (raw_reading > BEAT_THRESHOLD && cooldown == 0) {
    color_index = (color_index + 1) % num_colors;
    cur_rgb_color = color_palette[color_index];
    set_colors(cur_rgb_color);
    cooldown = cur_cooldown;
  } else {
    if (cooldown % 3 == 0) {
      decay_colors();
    }
  }

  cooldown--;
  if (cooldown < 0) cooldown = 0;

  write_colors();
}

void syncMusic() {
  set_color_palette();
}

void lookup(char* response, int response_size, char* key) {
  char request_buffer[200];
  sprintf(request_buffer, "GET /sandbox/sc/team65/dan/get_spotify_data.py?%s HTTP/1.1\r\n", key);
  strcat(request_buffer, "Host: 608dev-2.net\r\n");
  strcat(request_buffer, "\r\n"); //new line from header to body

  do_http_request("608dev-2.net", request_buffer, response, response_size, RESPONSE_TIMEOUT, true);
  response[5] = '\0';
}

void reset_color_palette() {
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
    Serial.println("HIGH TEMPO COLORS");
    color_palette = color_palettes[0];
    num_colors = palet_sizes[0];
    cur_cooldown = FAST_COOLDOWN;
  } else if (color_metric > MID_THRESH) {
    Serial.println("MID TEMPO COLORS");
    color_palette = color_palettes[1];
    num_colors = palet_sizes[1];
    cur_cooldown = MID_COOLDOWN;
  } else {
    Serial.println("LOW TEMPO COLORS");
    color_palette = color_palettes[2];
    num_colors = palet_sizes[2];
    cur_cooldown = SLOW_COOLDOWN;
  }
  color_index = 0;
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

void getCurrentSong() {
  request[0] = '\0'; //set 0th byte to null
  sprintf(request, "GET http://608dev-2.net/sandbox/sc/team65/raul/final_project_server_code.py?action=get_song&token=%s HTTP/1.1\r\n", SPOTIFY_OATH_TOKEN);
  strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
  strcat(request, "\r\n"); //add blank line!
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  char *p = strtok(response,"`");
  sprintf(songBuffer, p);
  p = strtok(NULL, "`");
  sprintf(songURI, p);
  Serial.println(songBuffer);
  Serial.println(songURI);
}

void playSong() {
  char body[100];
  sprintf(body,"action=play_song&song=%s&token=%s", username, shared, SPOTIFY_OATH_TOKEN);
  int body_len = strlen(body);
  sprintf(request,"POST http://608dev-2.net/sandbox/sc/team65/raul/final_project_server_code.py HTTP/1.1\r\n");
  strcat(request,"Host: 608dev-2.net\r\n");
  strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request+strlen(request),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
  strcat(request,"\r\n"); //new line from header to body
  strcat(request,body); //body
  strcat(request,"\r\n"); //new line
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
}

void initOptions() {
  request[0] = '\0'; //set 0th byte to null
  sprintf(request, "GET http://608dev-2.net/sandbox/sc/team65/raul/final_project_server_code.py?action=get_groups&username=%s\r\n",username);
  strcat(request, "Host: 608dev-2.net\r\n"); //add more to the end
  strcat(request, "\r\n"); //add blank line!
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  StaticJsonDocument<500> group_doc;
  deserializeJson(group_doc, response);
  num_groups = group_doc["num_groups"];
  if (num_groups > 10) {
    num_groups = 10;
  }
  for(int i = 0; i < num_groups; i++) {
    groups[i][0] = '\0';
    strcat(groups[i], group_doc["groups"][i]);
  }

  tft.setCursor(5, 0, 2);
  tft.fillRect(0, 0, 100, 15, TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_BLACK);
  tft.println(groups[0]);
    
  for(int i = 1; i < num_groups; i++) {
    tft.setCursor(5, 15*i, 2);
    tft.fillRect(0, 15*i, 100, 15, TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println(groups[i]);
  }
}

void nextOption(int option) {
  tft.setCursor(5, 0, 2);
  for(int i = 0; i < num_groups; i++) {
    tft.setCursor(5, 15*i, 2);
    if (i == option) {
      tft.fillRect(0, 15*i, 100, 15, TFT_GREEN);
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
    } else {
      tft.fillRect(0, 15*i, 100, 15, TFT_BLACK);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
    }
    tft.println(groups[i]);
  }
//  switch (option) {
//    case 0:
//      tft.setCursor(5, 60, 2);
//      tft.fillRect(0, 60, 100, 15, TFT_BLACK);
//      tft.setTextColor(TFT_GREEN, TFT_BLACK);
//      tft.println(groups[4]);
//
//      tft.setCursor(5, 0, 2);
//      tft.fillRect(0, 0, 100, 15, TFT_GREEN);
//      tft.setTextColor(TFT_BLACK, TFT_BLACK);
//      tft.println(groups[0]);
//      break;
//    case 1:
//      tft.setCursor(5, 0, 2);
//      tft.fillRect(0, 0, 100, 15, TFT_BLACK);
//      tft.setTextColor(TFT_GREEN, TFT_BLACK);
//      tft.println(groups[0]);
//
//      tft.setCursor(5, 15, 2);
//      tft.fillRect(0, 15, 100, 15, TFT_GREEN);
//      tft.setTextColor(TFT_BLACK, TFT_BLACK);
//      tft.println(groups[1]);
//      break;
//    case 2:
//      tft.setCursor(5, 15, 2);
//      tft.fillRect(0, 15, 100, 15, TFT_BLACK);
//      tft.setTextColor(TFT_GREEN, TFT_BLACK);
//      tft.println(groups[1]);
//
//      tft.setCursor(5, 30, 2);
//      tft.fillRect(0, 30, 100, 15, TFT_GREEN);
//      tft.setTextColor(TFT_BLACK, TFT_BLACK);
//      tft.println(groups[2]);
//      break;
//    case 3:
//      tft.setCursor(5, 30, 2);
//      tft.fillRect(0, 30, 100, 15, TFT_BLACK);
//      tft.setTextColor(TFT_GREEN, TFT_BLACK);
//      tft.println(groups[2]);
//
//      tft.setCursor(5, 45, 2);
//      tft.fillRect(0, 45, 100, 15, TFT_GREEN);
//      tft.setTextColor(TFT_BLACK, TFT_BLACK);
//      tft.println(groups[3]);
//      break;
//    case 4:
//      tft.setCursor(5, 45, 2);
//      tft.fillRect(0, 45, 100, 15, TFT_BLACK);
//      tft.setTextColor(TFT_GREEN, TFT_BLACK);
//      tft.println(groups[3]);
//
//      tft.setCursor(5, 60, 2);
//      tft.fillRect(0, 60, 100, 15, TFT_GREEN);
//      tft.setTextColor(TFT_BLACK, TFT_BLACK);
//      tft.println(groups[4]);
//      break;
//  }
}

// NETWORK STUFF

void setupWifi() {
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
      uint8_t* cc = WiFi.BSSID(i);
      for (int k = 0; k < 6; k++) {
        Serial.print(*cc, HEX);
        if (k != 5) Serial.print(":");
        cc++;
      }
      Serial.println("");
    }
  }
  delay(100); //wait a bit (100 ms)

  //if using regular connection use line below:
  WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);


  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 6) { //can change this to more attempts
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);  //acceptable since it is in the setup function.
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
}

/*----------------------------------
  char_append Function:
  Arguments:
     char* buff: pointer to character array which we will append a
     char c:
     uint16_t buff_size: size of buffer buff

  Return value:
     boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

/*----------------------------------
   do_http_request Function:
   Arguments:
      char* host: null-terminated char-array containing host to connect to
      char* request: null-terminated char-arry containing properly formatted HTTP request
      char* response: char-array used as output for function to contain response
      uint16_t response_size: size of response buffer (in bytes)
      uint16_t response_timeout: duration we'll wait (in ms) for a response from server
      uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
   Return value:
      void (none)
*/
void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line!
        break;
      }
//      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
//    memset(response, 0, response_size);
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");
  } else {
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}
