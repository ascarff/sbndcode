#include <vector>
#include <numeric>
#include <cassert>

#include "PeakFinder.hh"


PeakFinder::PeakFinder(std::vector<double> waveform, double baseline, unsigned n_smoothing_samples, double threshold) {
  assert(n_smoothing_samples % 2 == 1);

  unsigned smoothing_per_direction = n_smoothing_samples / 2;
  for (unsigned i = smoothing_per_direction; i < waveform.size() - smoothing_per_direction; i++) {
    unsigned begin = i - smoothing_per_direction;
    unsigned end = i + smoothing_per_direction + 1;
    double average = std::accumulate(waveform.begin() + begin, waveform.begin() + end, 0.0) / n_smoothing_samples;
   
    _smoothed_waveform.push_back(average); 
  }

  bool inside_peak = false;
  PeakFinder::Peak peak;
  for (unsigned i = 0; i < _smoothed_waveform.size(); i++) {
    double dat = _smoothed_waveform[i];
    if (dat > baseline + threshold) {
      if (!inside_peak) {
        peak.start = i;
        inside_peak = true;
      }
      peak.width ++;
      if (dat > peak.amplitude) {
        peak.amplitude = dat;
      }
    }
    else if (inside_peak) {
      inside_peak = false;
      int baseline_left_ind = std::max(0u, i - 1 - peak.width - smoothing_per_direction*2);
      double baseline_left = _smoothed_waveform[baseline_left_ind];
      int baseline_right_ind = std::min((unsigned)_smoothed_waveform.size()-1, i - 1 + smoothing_per_direction*2);
      double baseline_right = _smoothed_waveform[baseline_right_ind];
      peak.baseline = (baseline_left + baseline_right) / 2.;
      _peaks.emplace_back(peak);
      peak = PeakFinder::Peak();
    }
  }
}

