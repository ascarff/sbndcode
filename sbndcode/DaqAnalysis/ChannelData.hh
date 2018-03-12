#ifndef _sbnddaq_analysis_WaveformData
#define _sbnddaq_analysis_WaveformData

#include <vector>
#include <string>

#include "PeakFinder.hh"

namespace daqAnalysis {

class ChannelData {
public:
  unsigned channel_no;
  double baseline;
  double max;
  double rms;
  // TEMPORARY: save the correlation
  // between "adjacent" channels for debugging purposes
  double last_channel_correlation;
  double next_channel_correlation;
  // includes the first `n_baseline_samples` with
  // the mean subtracted for noise calculations
  std::vector<double> noise_sample;
  std::vector<double> waveform;
  std::vector<double> fft_real;
  std::vector<double> fft_imag;
  std::vector<PeakFinder::Peak> peaks;

  std::string Jsonify();
  
};

}

#endif
