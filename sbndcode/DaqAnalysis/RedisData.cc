#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <cassert> 
#include <ctime>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "ChannelData.hh"
#include "HeaderData.hh"
#include "RedisData.hh"
#include "ChannelMap.hh"

// Implementing StreamDataMean
void daqAnalysis::StreamDataMean::Add(unsigned index, float dat) {
  _data[index] = (_n_values * _data[index] + dat/_n_points_per_time[index]) / (_n_values + 1);
}

void daqAnalysis::StreamDataMean::Incl() {
  _n_values ++;
}

void daqAnalysis::StreamDataMean::Clear() {
  _n_values = 0;
}

float daqAnalysis::StreamDataMean::Take(unsigned index) {
  float ret = _data[index];
  _data[index] = 0;
  return ret;
}

void daqAnalysis::StreamDataMean::Update(bool taken) {
  // clear out _n_values if taken
  if (taken) Clear();
  // otherwise increment it
  else Incl();
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


float daqAnalysis::StreamDataVariableMean::Take(unsigned index) {
  // update from the most recent instance
  AddInstance(index);
  // return and clear data
  float ret = _data[index];
  _data[index] = 0;
  _n_values[index] = 0;
  return ret;
}

void daqAnalysis::StreamDataVariableMean::Update(bool taken) {
  // if the data was taken, then the current instance data was already added 
  // to the running data
  
  // otherwise, we need to do it now
  if (!taken) {
    for (unsigned index = 0; index < _instance_data.size(); index++) {
      AddInstance(index);
    }
  }
}

// Implementing StreamDataMax
void daqAnalysis::StreamDataMax::Add(unsigned index, float dat) {
  if (_data[index] < dat) _data[index] = dat;
}

float daqAnalysis::StreamDataMax::Take(unsigned index) {
  float ret = _data[index];
  _data[index] = 0;
  return ret;
}

// Defining string literal template parameters for inheritors of DetectorMetric
char REDIS_NAME_RMS[] = "rms";
char REDIS_NAME_OCCUPANCY[] = "hit_occupancy";
char REDIS_NAME_DNOISE[] = "next_channel_dnoise";
char REDIS_NAME_BASLINE[] = "baseline";
char REDIS_NAME_PULSE_HEIGHT[] = "pulse_height";

