#include <vector>
#include <array>
#include <numeric>
#include <math.h> 
#include <stdlib.h>
#include <iostream>

#include "Noise.hh"

daqAnalysis::NoiseSample::NoiseSample(std::vector<PeakFinder::Peak>& peaks, int16_t baseline, unsigned wvfm_size) {
  // we assume here that the vector of peaks are "sorted"
  // that peak[i].start_loose <= peak[i+1].start_loose and
  // that peak[i].end_loose <= peak[i+1].end_loose
  unsigned min = 0;
  unsigned peak_ind = 0;
  // noise samples are where the peaks aren't
  while (peak_ind < peaks.size()) {
    if (min < peaks[peak_ind].start_loose) {
      _ranges.emplace_back( std::array<unsigned,2>{min, peaks[peak_ind].start_loose-1});
    }
    min = peaks[peak_ind].end_loose+1;
    peak_ind ++;
  }
  if (min < wvfm_size-1) {
    _ranges.emplace_back( std::array<unsigned,2>{min, wvfm_size - 1} );
  } 
  _baseline = baseline;
}

float daqAnalysis::NoiseSample::CalcRMS(std::vector<int16_t> &wvfm_self, std::vector<std::array<unsigned,2>> &ranges, int16_t baseline) {
  unsigned n_samples = 0;
  int ret = 0;
  for (auto &range: ranges) {
    //std::cout << "RANGE: " << range[0] << " " << range[1] << std::endl;
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      ret += (wvfm_self[i] - baseline) * (wvfm_self[i] - baseline);
      //std::cout << "VAL: " << wvfm_self[i] << " " << baseline << " " << (wvfm_self[i] - baseline) * (wvfm_self[i] - baseline) << std::endl;
    }
  }
  //std::cout << "NSAMPLES: " << n_samples << std::endl;
  return sqrt((float)ret / n_samples);
}

daqAnalysis::NoiseSample daqAnalysis::NoiseSample::DoIntersection(daqAnalysis::NoiseSample &me, daqAnalysis::NoiseSample &other, int16_t baseline) {
  std::vector<std::array<unsigned, 2>> ranges;
  unsigned self_ind = 0;
  unsigned other_ind = 0;
  while (self_ind < me._ranges.size() && other_ind < other._ranges.size()) {
    // determine if there is a valid intersection
    bool is_intersection = (me._ranges[self_ind][1] >= other._ranges[other_ind][0]) &&
                           (me._ranges[self_ind][0] <= other._ranges[other_ind][1]);
    if (is_intersection) {
      unsigned intersection_lo = std::max(me._ranges[self_ind][0], other._ranges[other_ind][0]);
      unsigned intersection_hi = std::min(me._ranges[self_ind][1], other._ranges[other_ind][1]);
      ranges.emplace_back( std::array<unsigned,2>{intersection_lo, intersection_hi} );
    }

    // determine which ind to incl
    if (me._ranges[self_ind][1] < other._ranges[other_ind][1]) self_ind ++;
    else other_ind ++;

  }

  return daqAnalysis::NoiseSample(ranges, baseline);
}

float daqAnalysis::NoiseSample::Covariance(std::vector<int16_t> &wvfm_self, daqAnalysis::NoiseSample &other, std::vector<int16_t> &wvfm_other) {
  daqAnalysis::NoiseSample joint = Intersection(other);
  unsigned n_samples = 0;
  int ret = 0;
  for (auto &range: joint._ranges) {
    //std::cout << "RANGE: " << range[0] << " " << range[1] << std::endl;
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      ret += (wvfm_self[i] - _baseline) * (wvfm_other[i] - other._baseline);
      //std::cout << "VAL: " << wvfm_self[i] << " " << _baseline << " " << (wvfm_self[i] - _baseline) * (wvfm_other[i] - other._baseline) << std::endl;
      //std::cout << "VAL: " << wvfm_other[i] << " " << other._baseline << " " << (wvfm_self[i] - _baseline) * (wvfm_other[i] - other._baseline) << std::endl;
    }
  }
  //std::cout << "NSAMPLES: " << n_samples << std::endl;
  return ((float)ret) / n_samples;
}

float daqAnalysis::NoiseSample::Correlation(std::vector<int16_t> &wvfm_self, daqAnalysis::NoiseSample &other, std::vector<int16_t> &wvfm_other) {
  daqAnalysis::NoiseSample joint = Intersection(other);
  float scaling = CalcRMS(wvfm_self, joint._ranges, _baseline) * CalcRMS(wvfm_other, joint._ranges, other._baseline);
  return Covariance(wvfm_self, other, wvfm_other) / scaling;
}

float daqAnalysis::NoiseSample::SumRMS(std::vector<int16_t> &wvfm_self, daqAnalysis::NoiseSample &other, std::vector<int16_t> &wvfm_other) {
  daqAnalysis::NoiseSample joint = Intersection(other);
  unsigned n_samples = 0;
  int ret = 0;
  for (auto &range: joint._ranges) {
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      int16_t val = wvfm_self[i] - _baseline + wvfm_other[i] - other._baseline;
      ret += val * val;
    }
  }
  return sqrt(((float)ret) / n_samples);
}

float daqAnalysis::NoiseSample::ScaledSumRMS(std::vector<daqAnalysis::NoiseSample *>& noises, std::vector<std::vector<int16_t> *>& waveforms) {
  daqAnalysis::NoiseSample joint = DoIntersection(*noises[0], *noises[1]);
  for (unsigned i = 2; i < noises.size(); i++) {
    joint = DoIntersection(joint, *noises[i]);
  }
  unsigned n_samples = 0;
  int ret = 0;
  for (auto &range: joint._ranges) {
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      int sample = 0;
      for (unsigned wvfm_ind = 0; wvfm_ind < noises.size(); wvfm_ind++) {
        sample += (*waveforms[wvfm_ind])[i] - noises[wvfm_ind]->_baseline;
      }
      ret += sample * sample;
    }
  }
  float sum_rms = ((float)ret) / n_samples; 

  float rms_all = 0;
  for (unsigned wvfm_ind = 0; wvfm_ind < noises.size(); wvfm_ind++) {
    rms_all += CalcRMS(*waveforms[wvfm_ind], joint._ranges, noises[wvfm_ind]->_baseline);
  }
  // send uncorrelated sum-rms value to 0
  float scale_sub = rms_all * sqrt((float) noises.size());
  // and send fully correlated sum-rms value to 1
  float scale_div = rms_all * noises.size() - scale_sub;

  //std::cout << "UNSCALED: " << ret<< std::endl;
  //std::cout << "SACLE SUB: " << scale_sub << std::endl;
  //std::cout << "SACLE DIV: " << scale_div << std::endl;

  return (sum_rms - scale_sub) / scale_div; 
}



