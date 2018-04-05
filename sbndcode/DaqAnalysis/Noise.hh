#ifndef _sbnddaq_analysis_Noise
#define _sbnddaq_analysis_Noise
#include <vector>
#include <array>

#include "PeakFinder.hh"

// Holds a sample of a given waveform to be used to calculate noise metrics
namespace daqAnalysis {
class NoiseSample {
public:
  NoiseSample(std::vector<PeakFinder::Peak>& peaks, double baseline, unsigned wvfm_size);
  NoiseSample(std::vector<std::array<unsigned, 2>> ranges, double baseline): _ranges(ranges), _baseline(baseline) {}
  // zero initialize
  NoiseSample(): _baseline(0) {}

  NoiseSample Intersection(NoiseSample &other);

  double RMS(std::vector<double> &wvfm_self) { return CalcRMS(wvfm_self, _ranges, _baseline); } 

  double Covariance(std::vector<double> &wvfm_self, NoiseSample &other, std::vector<double> &wvfm_other);
  double Correlation(std::vector<double> &wvfm_self, NoiseSample &other, std::vector<double> &wvfm_other);
  double SumRMS(std::vector<double> &wvfm_self, NoiseSample &other, std::vector<double> &wvfm_other);
  //double SumRMS(std::vector<double> &wvfm_self, std::vector<std::pair<NoiseSample *, std::vector<double> *>>& other);

  std::vector<std::array<unsigned, 2>> *Ranges() { return &_ranges; }
private:
  static double CalcRMS(std::vector<double> &wvfm_self, std::vector<std::array<unsigned,2>> &ranges, double baseline);

  std::vector<std::array<unsigned, 2>> _ranges;
  double _baseline;
};
} // namespace daqAnalysis
#endif
