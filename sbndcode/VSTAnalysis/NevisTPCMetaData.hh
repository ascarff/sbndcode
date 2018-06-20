#ifndef _sbnddaq_analysis_NevisTPCMetaData
#define _sbnddaq_analysis_NevisTPCMetaData

#include <string>
#include <iostream>
#include <sstream> 
#include <stdlib.h>

#include "HeaderData.hh"

namespace daqAnalysis {
// Metadata associated with a NevisTPCHeader
class NevisTPCMetaData {
  public:
    uint32_t trig_frame_number;
    uint32_t frame_number;
    uint16_t slot;

  // print the data -- for debugging
  std::string Print() const {
    std::stringstream buffer;
    buffer << "slot: " << slot << std::endl;
    buffer << "frame no: " << frame_number << std::endl;
    buffer << "trigger frame no: " << trig_frame_number << std::endl;
    return buffer.str();
  }

  NevisTPCMetaData() {}

  explicit NevisTPCMetaData(daqAnalysis::HeaderData const& header) {
    slot = header.slot;
    trig_frame_number = header.trig_frame_number;
    frame_number = header.frame_number;
  }
};
}
#endif
