#ifndef _sbnddaq_analysis_Noise
#define _sbnddaq_analysis_Noise
#include <vector>
#include <array>

#include "PeakFinder.hh"

// keeps track of which regions of a waveform are suitable for noise calculations (i.e. don't contain signal)
namespace daqAnalysis {
class NoiseSample {
public:
  // construct sample from peaks (signals)
  NoiseSample(std::vector<PeakFinder::Peak>& peaks, int16_t baseline, unsigned wvfm_size);
  // construct from a list of ranges that don't have signal
  NoiseSample(std::vector<std::array<unsigned, 2>> ranges, int16_t baseline): _ranges(ranges), _baseline(baseline) {}
  // zero initialize
  NoiseSample(): _baseline(0) {}

  // calculate the intersect of ranges with another sample
  NoiseSample Intersection(NoiseSample &other) { return DoIntersection(*this, other, _baseline); }

  float RMS(std::vector<int16_t> &wvfm_self) { return CalcRMS(wvfm_self, _ranges, _baseline); } 

  // Functions for quantifying coherent noise:
  float Covariance(std::vector<int16_t> &wvfm_self, NoiseSample &other, std::vector<int16_t> &wvfm_other);
  float Correlation(std::vector<int16_t> &wvfm_self, NoiseSample &other, std::vector<int16_t> &wvfm_other);
  // the "Sum RMS" of a sample with another sample
  float SumRMS(std::vector<int16_t> &wvfm_self, NoiseSample &other, std::vector<int16_t> &wvfm_other);
  // the "Sum RMS" of n samples
  static float ScaledSumRMS(std::vector<NoiseSample *>& other, std::vector<std::vector<int16_t> *>& wvfm_other);
  // "DNoise" with another sample
  float DNoise(std::vector<int16_t> &wvfm_self, NoiseSample &other, std::vector<int16_t> &wvfm_other);

  // get access to the ranges
  std::vector<std::array<unsigned, 2>> *Ranges() { return &_ranges; }
private:
  static float CalcRMS(std::vector<int16_t> &wvfm_self, std::vector<std::array<unsigned,2>> &ranges, int16_t baseline);
  static NoiseSample DoIntersection(NoiseSample &me, NoiseSample &other, int16_t baseline=0.);

  std::vector<std::array<unsigned, 2>> _ranges;
  int16_t _baseline;
};
} // namespace daqAnalysis
#endif
