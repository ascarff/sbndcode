#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <sstream> 
#include <stdlib.h>

#include "PeakFinder.hh"
#include "ChannelData.hh"

float daqAnalysis::ChannelData::meanPeakHeight() {
  if (peaks.size() == 0) {
    return 0;
  } 

  int total = 0;
  for (unsigned i = 0; i < peaks.size(); i++) {
    // account fot up/down peaks
    if (peaks[i].is_up) {
      total += peaks[i].amplitude - baseline;
    }
    else {
      total += baseline - peaks[i].amplitude;
    }
  }
  return ((float) total) / peaks.size();
}

// only count up peaks
float daqAnalysis::ChannelData::Occupancy() {
  float n_peaks = 0;
  for (unsigned i = 0; i < peaks.size(); i++) {
    if (peaks[i].is_up) {
      n_peaks += 1;
    }
  }
  return n_peaks;
}

std::string daqAnalysis::ChannelData::Print() {
  std::stringstream buffer;
  buffer << "baseline: " << baseline << std::endl;
  buffer << "max: " << max << std::endl;
  buffer << "min: " << min << std::endl;
  buffer << "rms: " << rms << std::endl;
  buffer << "channel_no: " << channel_no << std::endl;
  buffer << "empty: " << empty << std::endl;
  buffer << "threshold: " << threshold << std::endl; 
  buffer << "next_channel_dnoise: " << next_channel_dnoise << std::endl;

  buffer << "peaks: [" << std::endl;
  for (auto &peak: peaks) {
    buffer << "  {" << std::endl;
    buffer << "    amplitude: " << peak.amplitude << std::endl;
    buffer << "    start_tight: " << peak.start_tight << std::endl;
    buffer << "    start_loose: " << peak.start_loose << std::endl;
    buffer << "    end_loose: " << peak.end_loose << std::endl;
    buffer << "    end_tight: " << peak.end_tight << std::endl;
    buffer << "    is_up: " << peak.is_up << std::endl;
    buffer << "  }" << std::endl;
  }
  buffer << "]" << std::endl;

  buffer << "noise_ranges: [" << std::endl;
  for (auto &range: noise_ranges) {
    buffer << "  [ " <<  range[0] << ", " << range[1] << "],"<< std::endl;
  }
  buffer << "]" << std::endl;

  return buffer.str();
}

