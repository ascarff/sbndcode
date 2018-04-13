#ifndef _sbnddaq_analysis_WaveformData
#define _sbnddaq_analysis_WaveformData

#include <vector>
#include <array>
#include <string>
#include <json/writer.h>

#include "PeakFinder.hh"

namespace daqAnalysis {
class ChannelData {
public:
  unsigned channel_no;
  bool empty;
  float baseline;
  int16_t max;
  int16_t min;
  float rms;
  // TEMPORARY: save the correlation
  // between "adjacent" channels for debugging purposes
  float last_channel_correlation;
  float next_channel_correlation;
  // and their sum-rms
  float last_channel_sum_rms;
  float next_channel_sum_rms;
  // thresholds
  int16_t threshold;
  std::vector<int16_t> waveform;
  std::vector<double> fft_real;
  std::vector<double> fft_imag;
  std::vector<PeakFinder::Peak> peaks;
  std::vector<std::array<unsigned, 2>> noise_ranges;

  Json::Value GetJson(); 
  std::string Jsonify();
  std::string JsonifyPretty();
  float meanPeakHeight();
  float Occupancy();

  // zero initialize
  ChannelData():
    channel_no(0),
    empty(true /* except for empty by default*/),
    baseline(0),
    max(0),
    min(0),
    rms(0),
    last_channel_correlation(0),
    next_channel_correlation(0),
    last_channel_sum_rms(0),
    next_channel_sum_rms(0),
    threshold(0)
  {}

  ChannelData(unsigned channel):
    channel_no(channel),
    empty(true /* except for empty by default*/),
    baseline(0),
    max(0),
    min(0),
    rms(0),
    last_channel_correlation(0),
    next_channel_correlation(0),
    last_channel_sum_rms(0),
    next_channel_sum_rms(0),
    threshold(0)
  {}
};

class ReducedChannelData {
public:
  unsigned channel_no;
  bool empty;
  int16_t baseline;
  float rms;
  unsigned n_peaks;
  float mean_peak_amplitude;

  // zero initialize
  ReducedChannelData():
    channel_no(0),
    empty(true /* except for empty by default*/),
    baseline(0),
    rms(0),
    n_peaks(0),
    mean_peak_amplitude(0)
  {}

  ReducedChannelData(ChannelData &channel_data) {
    channel_no = channel_data.channel_no;
    empty = channel_data.empty;
    baseline = channel_data.baseline;
    rms = channel_data.rms;
    n_peaks = channel_data.peaks.size();
    mean_peak_amplitude = channel_data.meanPeakHeight();
  }
};

}

#endif
