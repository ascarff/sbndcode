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
    bool is_up;

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

  PeakFinder(std::vector<double> &waveform, double baseline, double threshold, unsigned n_smoothing_samples=1, unsigned plane_type=0);
  inline std::vector<Peak> *Peaks() { return &_peaks; }
private:
  Peak FinishPeak(Peak peak, unsigned n_smoothing_samples, double baseline, bool up_peak, unsigned index);
  void matchPeaks(unsigned match_range);
  std::vector<double> _smoothed_waveform;
  std::vector<Peak> _peaks;
};

class Threshold {
public:
  Threshold(std::vector<double> &waveform, double baseline, double n_sigma=5., bool verbose=true);

  inline double Val() { return _threshold; }
private:
  double _threshold;
};

class RunningThreshold {
public:
  RunningThreshold(): _rms_ind(0), _n_past_rms(0) { std::fill(_past_rms.begin(), _past_rms.end(), 0); }

  double Threshold(std::vector<double> &waveform, double baseline, double n_sigma);
  void AddRMS(double rms);

private:
  std::array<double, 10> _past_rms;
  unsigned _rms_ind;
  unsigned _n_past_rms;
};

#endif
