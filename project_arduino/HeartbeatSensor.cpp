#include "Arduino.h"
#include "HeartbeatSensor.h"
#include <cmath>

const int REST = 0;
const int PULSE_UP = 1;
const int PULSE_DOWN = 2;

HeartbeatSensor::HeartbeatSensor() {
  state = REST;
  start_time = millis();
  std = 0;
  mean = 0;
  beat_count = 0;
  bpm = 0;
}

int HeartbeatSensor::update(long reading) {
    updateReadings(reading);
    mean = mean_std[0];
    std = mean_std[1];
    switch (state) {
      case REST:
        if (reading > mean + std) {
          state = PULSE_UP;
        }
        break;
      case PULSE_UP:
        if (reading < mean + (std / 2)) {
          state = PULSE_DOWN;
        }
        break;
      case PULSE_DOWN:
        beat_count++;
        int duration_in_seconds = (millis() - start_time) / (1000);
        if (duration_in_seconds == 0) duration_in_seconds = 1;
        float bps = ((float) beat_count) / ((float) duration_in_seconds);
        bpm = bps * 60;
        state = REST;
        break;
    }
    int dur_in_s = (millis() - start_time) / (1000);
    return bpm;
  }

void HeartbeatSensor::updateReadings(long reading) {
    long avg = 0;
    for (int i = NUM_READINGS - 1; i > 0; i--) {
      previous_readings[i] = previous_readings[i-1];
      avg += (previous_readings[i-1] / NUM_READINGS);
    }
    previous_readings[0] = reading;
    avg += reading / NUM_READINGS;
    long std_dev = 0;
    for (int i = 0; i < NUM_READINGS; i++) {
      std_dev += pow(previous_readings[i] - avg, 2) / NUM_READINGS;
    }
    std_dev = sqrt(std_dev);
    mean_std[0] = avg;
    mean_std[1] = std_dev;
  }
