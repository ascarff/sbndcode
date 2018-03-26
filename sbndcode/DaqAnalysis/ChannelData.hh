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
  double baseline;
  double max;
  double min;
  double rms;
  // TEMPORARY: save the correlation
  // between "adjacent" channels for debugging purposes
  double last_channel_correlation;
  double next_channel_correlation;
  // and their sum-rms
  double last_channel_sum_rms;
  double next_channel_sum_rms;
  std::vector<double> waveform;
  std::vector<double> fft_real;
  std::vector<double> fft_imag;
  std::vector<PeakFinder::Peak> peaks;
  std::vector<std::array<unsigned, 2>> noise_ranges;

  Json::Value GetJson(); 
  std::string Jsonify();
  std::string JsonifyPretty();
  
};

}

#endif
