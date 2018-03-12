#ifndef _sbnddaq_analysis_HeaderData
#define _sbnddaq_analysis_HeaderData

#include "sbnddaq-datatypes/Overlays/NevisTPCFragment.hh"

// TODO: This should probably have a better name

namespace daqAnalysis {
// Header Data associated witha NevisTPCHeader
class HeaderData {
  public:
  uint32_t event_number;
  uint32_t frame_number;
  uint32_t checksum;
  uint32_t adc_word_count;
  uint32_t trig_frame_number;
  double frame_time;
  double trig_frame_time;

  HeaderData(const sbnddaq::NevisTPCHeader *raw_header, double frame_to_dt) {
    event_number = raw_header->getEventNum();
    frame_number = raw_header->getFrameNum();
    checksum = raw_header->getChecksum();
    adc_word_count = raw_header->getADCWordCount();
    trig_frame_number = raw_header->getTrigFrame();

    frame_time = frame_number * frame_to_dt;
    trig_frame_time = trig_frame_number * frame_to_dt;
  }
  HeaderData() {}

};
}

#endif
