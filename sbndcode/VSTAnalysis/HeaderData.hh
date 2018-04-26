#ifndef _sbnddaq_analysis_HeaderData
#define _sbnddaq_analysis_HeaderData

#include "ChannelMap.hh"

namespace daqAnalysis {
// Header Data associated with a NevisTPCHeader
class HeaderData {
  public:
  uint8_t crate;
  uint8_t slot;
  uint32_t event_number;
  uint32_t frame_number;
  uint32_t checksum;
  uint32_t computed_checksum;
  uint32_t adc_word_count;
  uint32_t trig_frame_number;
  double frame_time;
  double trig_frame_time;

  HeaderData() {}
  // Returns the logical index of the header
  // @VST This is ok because crate will always be 0
  size_t Ind() const {
    return crate * ChannelMap::n_fem_per_crate + slot;
  }

};
}

#endif
