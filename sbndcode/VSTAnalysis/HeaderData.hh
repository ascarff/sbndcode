#ifndef _sbnddaq_analysis_HeaderData
#define _sbnddaq_analysis_HeaderData

// TODO: This should probably have a better name

namespace daqAnalysis {
// Header Data associated with a NevisTPCHeader
class HeaderData {
  public:
  uint8_t fem_id;
  uint8_t slot_id;
  uint32_t event_number;
  uint32_t frame_number;
  uint32_t checksum;
  uint32_t adc_word_count;
  uint32_t trig_frame_number;
  double frame_time;
  double trig_frame_time;

  HeaderData() {}
  // Returns the logical index of the header
  // TODO @INSTALLATION: implement
  size_t Ind() const {
    return 0;
  }

};
}

#endif
