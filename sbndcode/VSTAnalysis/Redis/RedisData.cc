#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <cassert> 
#include <ctime>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "../ChannelData.hh"
#include "../HeaderData.hh"
#include "../ChannelMap.hh"

#include "RedisData.hh"

// Implementing StreamDataMean
void daqAnalysis::StreamDataMean::Add(unsigned index, float dat) {
  _instance_data[index] += dat/_n_points_per_time[index];
}

// clear data
void daqAnalysis::StreamDataMean::Clear() {
  for (unsigned index = 0; index < _data.size(); index++) {
    _data[index] = 0;
  }
  _n_values = 0;
}

float daqAnalysis::StreamDataMean::Data(unsigned index) {
  float ret = _data[index];
  return ret;
}

void daqAnalysis::StreamDataMean::Update() {
  // update instance data
  for (unsigned index = 0; index < _instance_data.size(); index++) {
    AddInstance(index);
  }
  // increment _n_values
  _n_values ++;
}

// takes new data value out of instance and puts it in _data (not idempotent)
void daqAnalysis::StreamDataMean::AddInstance(unsigned index) {
  // add instance data to data
  _data[index] = (_data[index] * _n_values + _instance_data[index]) / (_n_values + 1);

  // clear instance data
  _instance_data[index] = 0;
}

// Implementing StreamDataVariableMean
void daqAnalysis::StreamDataVariableMean::Add(unsigned index, float dat) {
  // don't take values near zero
  if (dat < 1e-4) {
    return;
  }
  // add to the counter at this time instance
  _instance_data[index] = (_n_values_current_instance[index] * _instance_data[index] + dat) / (_n_values_current_instance[index] + 1);
  _n_values_current_instance[index] ++;
}

// takes new data value out of instance and puts it in _data (note: is idempotent)
void daqAnalysis::StreamDataVariableMean::AddInstance(unsigned index) {
  // ignore if there's no data at this time instance
  if (_n_values_current_instance[index] == 0) {
    return;
  }
  // add instance data to data
  _data[index] = (_data[index] * _n_values[index] + _instance_data[index]) / (_n_values[index] + 1);

  // clear instance data
  _instance_data[index] = 0;
  _n_values_current_instance[index] = 0;
}


float daqAnalysis::StreamDataVariableMean::Data(unsigned index) {
  // return data
  float ret = _data[index];
  return ret;
}

// clear data
void daqAnalysis::StreamDataVariableMean::Clear() {
  for (unsigned index = 0; index < _data.size(); index++) {
    _data[index] = 0;
    _n_values[index] = 0;
  }
}

void daqAnalysis::StreamDataVariableMean::Update() {
  // add instance data and increment n_values
  for (unsigned index = 0; index < _instance_data.size(); index++) {
    AddInstance(index);
    _n_values[index] += 1;
  }
}

// Implementing StreamDataMax
void daqAnalysis::StreamDataMax::Add(unsigned index, unsigned dat) {
  if (_data[index] < dat) _data[index] = dat;
}

unsigned daqAnalysis::StreamDataMax::Data(unsigned index) {
  float ret = _data[index];
  return ret;
}

// clear data
void daqAnalysis::StreamDataMax::Clear() {
  for (unsigned index = 0; index < _data.size(); index++) {
    _data[index] = 0;
  }
}

// Implementing StreamDataSum
void daqAnalysis::StreamDataSum::Add(unsigned index, unsigned dat) {
  _data[index] += dat;
}

unsigned daqAnalysis::StreamDataSum::Data(unsigned index) {
  float ret = _data[index];
  return ret;
}

// clear data
void daqAnalysis::StreamDataSum::Clear() {
  for (unsigned index = 0; index < _data.size(); index++) {
    _data[index] = 0;
  }
}

// Defining string literal template parameters for inheritors of DetectorMetric
char REDIS_NAME_RMS[] = "rms";
char REDIS_NAME_OCCUPANCY[] = "hit_occupancy";
char REDIS_NAME_DNOISE[] = "next_channel_dnoise";
char REDIS_NAME_BASLINE[] = "baseline";
char REDIS_NAME_PULSE_HEIGHT[] = "pulse_height";
char REDIS_NAME_RAWHIT_OCCUPANCY[] = "rawhit_occupancy";
char REDIS_NAME_RAWHIT_PULSE_HEIGHT[] = "rawhit_pulse_height";

// and of HeaderMetric
char REDIS_NAME_EVENT_NO[] = "event_no";
char REDIS_NAME_TRIG_FRAME_NO[] = "trig_frame_no";
char REDIS_NAME_FRAME_NO[] = "frame_no";
char REDIS_NAME_BLOCKS[] = "blocks";


