#ifndef HeartbeatSensor_h
#define HeartbeatSensor_h
#include "Arduino.h"
const int NUM_READINGS = 100;
class HeartbeatSensor {
  public:
  long previous_readings[NUM_READINGS];
  int beat_count;
  long start_time;
  long mean_std[2];
  long mean;
  long std;
  int bpm;
  int state;
    HeartbeatSensor();
    int update(long reading);
    void updateReadings(long reading);
};
#endif
