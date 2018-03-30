#include <vector>
#include <array>
#include <numeric>
#include <math.h> 
#include <stdlib.h>
#include <iostream>

#include "Noise.hh"

daqAnalysis::NoiseSample::NoiseSample(std::vector<PeakFinder::Peak>& peaks, double baseline, unsigned wvfm_size) {
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

double daqAnalysis::NoiseSample::CalcRMS(std::vector<double> &wvfm_self, std::vector<std::array<unsigned,2>> &ranges, double baseline) {
  unsigned n_samples = 0;
  double ret = 0;
  for (auto &range: ranges) {
    //std::cout << "RANGE: " << range[0] << " " << range[1] << std::endl;
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      ret += (wvfm_self[i] - baseline) * (wvfm_self[i] - baseline);
      //std::cout << "VAL: " << wvfm_self[i] << " " << baseline << " " << (wvfm_self[i] - baseline) * (wvfm_self[i] - baseline) << std::endl;
    }
  }
  //std::cout << "NSAMPLES: " << n_samples << std::endl;
  return sqrt(ret / n_samples);
}

daqAnalysis::NoiseSample daqAnalysis::NoiseSample::Intersection(daqAnalysis::NoiseSample &other) {
  std::vector<std::array<unsigned, 2>> ranges;
  unsigned self_ind = 0;
  unsigned other_ind = 0;
  while (self_ind < _ranges.size() && other_ind < other._ranges.size()) {
    // determine if there is a valid intersection
    bool is_intersection = (_ranges[self_ind][1] >= other._ranges[other_ind][0]) &&
                           (_ranges[self_ind][0] <= other._ranges[other_ind][1]);
    if (is_intersection) {
      unsigned intersection_lo = std::max(_ranges[self_ind][0], other._ranges[other_ind][0]);
      unsigned intersection_hi = std::min(_ranges[self_ind][1], other._ranges[other_ind][1]);
      ranges.emplace_back( std::array<unsigned,2>{intersection_lo, intersection_hi} );
    }

    // determine which ind to incl
    if (_ranges[self_ind][1] < other._ranges[other_ind][1]) self_ind ++;
    else other_ind ++;

  }

  return daqAnalysis::NoiseSample(ranges, _baseline);
}

double daqAnalysis::NoiseSample::Covariance(std::vector<double> &wvfm_self, daqAnalysis::NoiseSample &other, std::vector<double> &wvfm_other) {
  daqAnalysis::NoiseSample joint = Intersection(other);
  unsigned n_samples = 0;
  double ret = 0;
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
  return ret / n_samples;
}

double daqAnalysis::NoiseSample::Correlation(std::vector<double> &wvfm_self, daqAnalysis::NoiseSample &other, std::vector<double> &wvfm_other) {
  daqAnalysis::NoiseSample joint = Intersection(other);
  double scaling = CalcRMS(wvfm_self, joint._ranges, _baseline) * CalcRMS(wvfm_other, joint._ranges, other._baseline);
  return Covariance(wvfm_self, other, wvfm_other) / scaling;
}

double daqAnalysis::NoiseSample::SumRMS(std::vector<double> &wvfm_self, daqAnalysis::NoiseSample &other, std::vector<double> &wvfm_other) {
  daqAnalysis::NoiseSample joint = Intersection(other);
  unsigned n_samples = 0;
  double ret = 0;
  for (auto &range: joint._ranges) {
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      double val = wvfm_self[i] - _baseline + wvfm_other[i] - other._baseline;
      ret += val * val;
    }
  }
  return sqrt(ret / n_samples);
}

