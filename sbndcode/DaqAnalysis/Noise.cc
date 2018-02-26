#include <vector>
#include <numeric>
#include <math.h> 


#include "Noise.hh"

Noise::Noise(std::vector<double> &inp_wvfm_1, std::vector<double> &inp_wvfm_2, unsigned n_baseline_samples) {
  for (unsigned i = 0; i < n_baseline_samples; i++) {
    wvfm_1.push_back( inp_wvfm_1[i] );
    wvfm_2.push_back( inp_wvfm_2[i] );
    wvfm_diff.push_back( inp_wvfm_1[i] - inp_wvfm_2[i] );
  } 

  double wvfm_1_baseline = 
    std::accumulate(wvfm_1.begin(), wvfm_1.end(), 0.0) / wvfm_1.size();
  double wvfm_2_baseline = 
    std::accumulate(wvfm_2.begin(), wvfm_2.end(), 0.0) / wvfm_1.size();
  double wvfm_diff_baseline = wvfm_1_baseline - wvfm_2_baseline;

  for (unsigned i = 0; i < n_baseline_samples; i++) {
    wvfm_1[i] -= wvfm_1_baseline;
    wvfm_2[i] -= wvfm_2_baseline;
    wvfm_diff[i] -= wvfm_diff_baseline;
  }

}

double Noise::CalcCov(std::vector<double> &wvfm_1, std::vector<double> &wvfm_2) {
  double ret = 0;
  for (unsigned i = 0; i < wvfm_1.size(); i++) {
    ret += wvfm_1[i] * wvfm_2[i];
  }
  ret /= wvfm_1.size();
  return ret;
}

double Noise::CalcCor(std::vector<double> &wvfm_1, std::vector<double> &wvfm_2) {
  return CalcCov(wvfm_1, wvfm_2) / CalcRMS(wvfm_1) / CalcRMS(wvfm_2);
}

double Noise::CalcRMS(std::vector<double> &wvfm) {
  double ret = 0;
  for (unsigned i = 0; i < wvfm.size(); i++) {
    ret += wvfm[i] * wvfm[i];
  }
  ret /= wvfm.size();
  return sqrt(ret);
  
}
