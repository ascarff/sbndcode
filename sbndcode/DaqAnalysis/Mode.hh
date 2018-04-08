#ifndef DAQANALYSIS_MODE_HH
#define DAQANALYSIS_MODE_HH

#include <vector>
#include <array>

#include "Mode.hh"

// Calculate the mode to find a baseline of the passed in waveform.
// Mode finding algorithm from: http://erikdemaine.org/papers/NetworkStats_ESA2002/paper.pdf (Algorithm FREQUENT)
int16_t Mode(const std::vector<int16_t> &adcs);

#endif
