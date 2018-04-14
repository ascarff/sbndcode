#ifndef _sbnddaq_analysis_Noise
#define _sbnddaq_analysis_Noise
#include <vector>
#include <array>

#include "PeakFinder.hh"

// Holds a sample of a given waveform to be used to calculate noise metrics
namespace daqAnalysis {
class NoiseSample {
public:
  NoiseSample(std::vector<PeakFinder::Peak>& peaks, int16_t baseline, unsigned wvfm_size);
  NoiseSample(std::vector<std::array<unsigned, 2>> ranges, int16_t baseline): _ranges(ranges), _baseline(baseline) {}
  // zero initialize
  NoiseSample(): _baseline(0) {}

  NoiseSample Intersection(NoiseSample &other) { return DoIntersection(*this, other, _baseline); }

  float RMS(std::vector<int16_t> &wvfm_self) { return CalcRMS(wvfm_self, _ranges, _baseline); } 

  float Covariance(std::vector<int16_t> &wvfm_self, NoiseSample &other, std::vector<int16_t> &wvfm_other);
  float Correlation(std::vector<int16_t> &wvfm_self, NoiseSample &other, std::vector<int16_t> &wvfm_other);
  float SumRMS(std::vector<int16_t> &wvfm_self, NoiseSample &other, std::vector<int16_t> &wvfm_other);
  static float ScaledSumRMS(std::vector<NoiseSample *>& other, std::vector<std::vector<int16_t> *>& wvfm_other);
  float DNoise(std::vector<int16_t> &wvfm_self, NoiseSample &other, std::vector<int16_t> &wvfm_other);

  std::vector<std::array<unsigned, 2>> *Ranges() { return &_ranges; }
private:
  static float CalcRMS(std::vector<int16_t> &wvfm_self, std::vector<std::array<unsigned,2>> &ranges, int16_t baseline);
  static NoiseSample DoIntersection(NoiseSample &me, NoiseSample &other, int16_t baseline=0.);

  std::vector<std::array<unsigned, 2>> _ranges;
  int16_t _baseline;
};
} // namespace daqAnalysis
#endif
