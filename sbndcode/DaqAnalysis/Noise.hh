#ifndef _sbnddaq_analysis_Noise
#define _sbnddaq_analysis_Noise
#include <vector>

// Calculate the correlated noise of two waveforms
class Noise {
public:
  Noise(std::vector<double> &wvfm_1, std::vector<double> &wvfm_2, unsigned n_baseline_samples);
  inline double RMS1() { return CalcRMS(wvfm_1); }
  inline double RMS2() { return CalcRMS(wvfm_2); }
  inline double CorrelatedRMS() { return CalcRMS(wvfm_diff); }
  inline double Covariance() { return CalcCov(wvfm_1, wvfm_2); }
  inline double Correlation() { return CalcCor(wvfm_1, wvfm_2); }
private:
  static double CalcRMS(std::vector<double> &wvfm);
  static double CalcCov(std::vector<double> &wvfm_1, std::vector<double> &wvfm_2);
  static double CalcCor(std::vector<double> &wvfm_1, std::vector<double> &wvfm_2);

  std::vector<double> wvfm_1;
  std::vector<double> wvfm_2;
  std::vector<double> wvfm_diff;
};
#endif
