#ifndef _sbnddaq_analysis_FFT
#define _sbnddaq_analysis_FFT
#include <vector>

#include "fftw3.h"

// Computes the Discrete Fourier Transform of the _real_ input data.
// Output has a size 2 *(n/2 + 1) where n is the size of the input data.
// Returned array must be free'd with fftw_free
class FFT {
public:
  FFT(std::vector<double>& input);
  ~FFT();
  // returns the size of the output array
  int size();
  fftw_complex *data();

protected:
  int _fft_size;
  fftw_complex *_fft_array;
  fftw_plan _plan;
};

#endif
