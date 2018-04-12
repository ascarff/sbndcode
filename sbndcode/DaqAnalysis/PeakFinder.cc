#include <vector>
#include <numeric>
#include <cassert>
#include <float.h>
#include <algorithm>
#include <iostream>

#include "TH1D.h"
#include "TF1.h"

#include "PeakFinder.hh"
#include "Noise.hh"

inline bool fitDownPeak(unsigned plane_type) {
  return plane_type < 2;
}

inline bool doMatchPeaks(unsigned plane_type) {
  return plane_type == 1;
}

// plane_type == 0 means fit up and down peaks and don't match (i.e. debug mode)
// plane_type == 1 means fit up and down peaks and match (induction planes)
// plane_type == 2 means fit up peaks only (collection planes)
PeakFinder::PeakFinder(std::vector<int16_t> &waveform, int16_t baseline, int16_t threshold, unsigned n_smoothing_samples, unsigned plane_type ) {
  // number of smoothing samples must be odd to make sense
  assert(n_smoothing_samples % 2 == 1);

  // smooth out input waveform
  unsigned smoothing_per_direction = n_smoothing_samples / 2;
  for (unsigned i = smoothing_per_direction; i < waveform.size() - smoothing_per_direction; i++) {
    unsigned begin = i - smoothing_per_direction;
    unsigned end = i + smoothing_per_direction + 1;
    int16_t average = std::accumulate(waveform.begin() + begin, waveform.begin() + end, 0.0) / n_smoothing_samples;
   
    _smoothed_waveform.push_back(average); 
  }

  // iterate through smoothed samples
  bool inside_peak = false;
  // whether peak points up or down
  bool up_peak = false;
  PeakFinder::Peak peak;
  for (unsigned i = 0; i < _smoothed_waveform.size(); i++) {
    int16_t dat = _smoothed_waveform[i];
    // detect a new peak, or continue on the current one

    // up-peak
    if (dat > baseline + threshold) {
      // it's a new peak!
      if (!inside_peak) {
        peak.start_tight = i;
        inside_peak = true;
        up_peak = true;
        peak.is_up = true;
      }
      else assert(up_peak);

      // use original waveform to determine amplitude
      if (waveform[i] > peak.amplitude) {
        peak.amplitude = waveform[i];
        peak.peak_index = i;
      }
    }
    // down-peak
    else if (fitDownPeak(plane_type) && dat < baseline - threshold) {
      if (!inside_peak) {
        peak.amplitude = INT16_MAX;
        peak.start_tight = i;
        inside_peak = true;
        up_peak = false;
        peak.is_up = false;
      }
      else assert(!up_peak);

      // use original waveform to determine amplitude
      if (waveform[i] < peak.amplitude) {
        peak.amplitude = waveform[i];
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

  // match peaks
  if (doMatchPeaks(plane_type)) {
    // set matching distance based on smoothing samples (for now)
    matchPeaks(n_smoothing_samples*2);
  }
}


PeakFinder::Peak PeakFinder::FinishPeak(PeakFinder::Peak peak, unsigned n_smoothing_samples, int16_t baseline, bool up_peak, unsigned index) {
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

// match up peak - down peak pairs for collection planes
void PeakFinder::matchPeaks(unsigned match_range) {
  std::vector<PeakFinder::Peak> pruned_peaks;

  bool last_was_up_peak = false;
  for (unsigned i = 0; i < _peaks.size(); i++) {
    // look for down peak that immediately follows up peak
    if (!_peaks[i].is_up && last_was_up_peak) {
      // make sure start of down peak is within range of end of up peak
      if ((i > 0) &&
          ((_peaks[i].start_loose < _peaks[i-1].end_loose) ||
           (_peaks[i].start_loose - _peaks[i-1].end_loose < match_range))) {
        pruned_peaks.push_back(_peaks[i-1]);
        pruned_peaks.push_back(_peaks[i]);
      }
    }
    last_was_up_peak = _peaks[i].is_up;
  }
  // set the peaks to the pruned version
  _peaks = pruned_peaks;
}


Threshold::Threshold(std::vector<int16_t> &waveform, int16_t baseline, float n_sigma, bool verbose) {
  int16_t min = *std::min_element(waveform.begin(), waveform.end());
  int16_t max = *std::max_element(waveform.begin(), waveform.end());
  size_t length = waveform.size();

  TH1D hist("waveform", "waveform", length/100, min, max);
  for (int16_t dat: waveform) {
    hist.Fill(dat);
  }
  if (verbose) {
    std::cout << "MEAN: " << hist.GetMean() << std::endl;
    std::cout << "RMS: " << hist.GetRMS() << std::endl;
  }

  TF1 fit("fit", "gaus");
  fit.SetParameter(0, baseline);
  fit.SetParameter(1, hist.GetMean());
  fit.SetParameter(2, hist.GetRMS());

  fit.SetRange( hist.GetMean() - n_sigma*hist.GetRMS(), hist.GetMean() + n_sigma*hist.GetRMS() );

  if (verbose) {
    hist.Fit(&fit, "R");
  }
  else {
    hist.Fit(&fit, "RQ");
  }

  // set thresholds at n_sigma
  _threshold = baseline + n_sigma*fit.GetParameter(2);
}

float rawRMS(std::vector<int16_t> &waveform, int16_t baseline) {
  daqAnalysis::NoiseSample temp({{0, (unsigned)waveform.size() -1}}, baseline);
  return temp.RMS(waveform);
}

int16_t RunningThreshold::Threshold(std::vector<int16_t> &waveform, int16_t baseline, float n_sigma) {
  if (_n_past_rms == 0) {
    // 2x penalty since rawRMS will overestimate the true RMS
    return rawRMS(waveform, baseline) * n_sigma / 2;
  }
  else {
    float rms = 0;
    for (unsigned i = 0; i < _n_past_rms; i++) {
      rms += _past_rms[i];
    }
    rms = rms / _n_past_rms;
    return rms * n_sigma;
  }
}

void RunningThreshold::AddRMS(float rms) {
  if (_n_past_rms < 10) _n_past_rms ++;
  _past_rms[_rms_ind] = rms;
  _rms_ind = (_rms_ind + 1 ) % _past_rms.size();
}

