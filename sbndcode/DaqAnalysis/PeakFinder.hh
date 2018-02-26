#ifndef _sbnddaq_analysis_PeakFinder
#define _sbnddaq_analysis_PeakFinder
#include <vector>
#include <float.h>

// Reinventing the wheel: search for a bunch of peaks in a set of data
// 
// Implementation: searches for points above some threshold (requiring a 
// good basline) and tries to make peaks for them.
class PeakFinder {
public:
  class Peak {
  public:
    double amplitude;
    unsigned width;
    unsigned start;
    double baseline;

    Peak() {
      amplitude = -DBL_MAX;
      width = 0;      
      baseline = 0;
    }
    void reset() {
      amplitude = -DBL_MAX;
      width = 0;
      baseline = 0;
    }
  };

  PeakFinder(std::vector<double> waveform, double baseline, unsigned n_smoothing_samples=1, double threshold=100.);
  inline std::vector<Peak> *Peaks() { return &_peaks; }
private:
  std::vector<double> _smoothed_waveform;
  std::vector<Peak> _peaks;

};
#endif
