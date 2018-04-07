#include <vector>
#include <numeric>
#include <cassert>
#include <float.h>

#include "PeakFinder.hh"


PeakFinder::PeakFinder(std::vector<double> waveform, double baseline, unsigned n_smoothing_samples, double threshold_up, double threshold_down) {
  // number of smoothing samples must be odd to make sense
  assert(n_smoothing_samples % 2 == 1);

  // smooth out input waveform
  unsigned smoothing_per_direction = n_smoothing_samples / 2;
  for (unsigned i = smoothing_per_direction; i < waveform.size() - smoothing_per_direction; i++) {
    unsigned begin = i - smoothing_per_direction;
    unsigned end = i + smoothing_per_direction + 1;
    double average = std::accumulate(waveform.begin() + begin, waveform.begin() + end, 0.0) / n_smoothing_samples;
   
    _smoothed_waveform.push_back(average); 
  }

  // iterate through smoothed samples
  bool inside_peak = false;
  // whether peak points up or down
  bool up_peak = false;
  PeakFinder::Peak peak;
  for (unsigned i = 0; i < _smoothed_waveform.size(); i++) {
    double dat = _smoothed_waveform[i];
    // detect a new peak, or continue on the current one

    // up-peak
    if (dat > baseline + threshold_up) {
      // it's a new peak!
      if (!inside_peak) {
        peak.start_tight = i;
        inside_peak = true;
        up_peak = true;
        peak.is_up = true;
      }
      else assert(up_peak);

      if (dat > peak.amplitude) {
        peak.amplitude = dat;
        peak.peak_index = i;
      }
    }
    // down-peak
    else if (threshold_down > 0 && dat < baseline - threshold_down) {
      if (!inside_peak) {
        peak.amplitude = DBL_MAX;
        peak.start_tight = i;
        inside_peak = true;
        up_peak = false;
        peak.is_up = false;
      }
      else assert(!up_peak);

      if (dat < peak.amplitude) {
        peak.amplitude = dat;
        peak.peak_index = i;
      }
    }
    // this means we're at the end of a peak. 
    // process it and set up the next one
    else if (inside_peak) {
      inside_peak = false;
      peak = FinishPeak(peak, n_smoothing_samples, baseline, up_peak, i);
      _peaks.emplace_back(peak);
      peak = PeakFinder::Peak();
    }
    else { /* not inside peak, not at end of peak, nothing to do */ }
  }
  // finish peak if we're inside one at the end
  if (inside_peak) {
    peak = FinishPeak(peak, n_smoothing_samples, baseline, up_peak, _smoothed_waveform.size()-1);
    _peaks.emplace_back(peak);
  }
}


PeakFinder::Peak PeakFinder::FinishPeak(PeakFinder::Peak peak, unsigned n_smoothing_samples, double baseline, bool up_peak, unsigned index) {
  peak.end_tight = index;
  // find the upper and lower bounds to determine the max width
  peak.start_loose = peak.start_tight;
  unsigned n_at_baseline = 0;
  while (peak.start_loose > 0) {
    if ((up_peak && _smoothed_waveform[peak.start_loose] < baseline) ||
       (!up_peak && _smoothed_waveform[peak.start_loose] > baseline)) {

      n_at_baseline ++;
    }
    // define the lower bound to be where the point where the waveform
    // has gone under the baseline twice
    if (n_at_baseline >= 2) {
      break;
    }
    peak.start_loose --;
  }
  // set start_loose such that it isn't under the influence of any points inside peak
  peak.start_loose = (peak.start_loose >= n_smoothing_samples/2) ? (peak.start_loose - n_smoothing_samples/2) : 0;
  
  // now find upper bound on end
  peak.end_loose = peak.end_tight;
  n_at_baseline = 0;
  while (peak.end_loose < _smoothed_waveform.size()-1) {
    if ((up_peak && _smoothed_waveform[peak.end_loose] < baseline) ||
       (!up_peak && _smoothed_waveform[peak.end_loose] > baseline)) {
      n_at_baseline ++;
    }
    if (n_at_baseline > 2) {
      break;
    }
    peak.end_loose ++;
  }
  // set end_loose such that it isn't under the influence of any points inside peak
  peak.end_loose = std::min(peak.end_loose + n_smoothing_samples/2, (unsigned)_smoothed_waveform.size()-1);
  return peak;
}

