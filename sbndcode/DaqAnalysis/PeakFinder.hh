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
    unsigned peak_index;
    unsigned start_tight;
    unsigned start_loose;
    unsigned end_tight;
    unsigned end_loose;

    Peak() {
      reset();
    }
    void reset() {
      amplitude = -DBL_MAX;
      peak_index = 0;
      start_tight = 0;
      start_loose = 0;
      end_tight = 0;
      end_loose = 0;
    }
  };

  PeakFinder(std::vector<double> waveform, double baseline, unsigned n_smoothing_samples=1, double threshold_hi=100., double threshold_lo=-1);
  inline std::vector<Peak> *Peaks() { return &_peaks; }
private:
  Peak FinishPeak(Peak peak, unsigned n_smoothing_samples, double baseline, bool up_peak, unsigned index);
  std::vector<double> _smoothed_waveform;
  std::vector<Peak> _peaks;

};
#endif
