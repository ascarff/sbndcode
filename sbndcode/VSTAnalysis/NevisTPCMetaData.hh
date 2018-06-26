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
    uint32_t two_mhzsample;
    uint32_t sub_run_no;

  // print the data -- for debugging
  std::string Print() const {
    std::stringstream buffer;
    buffer << "slot: " << slot << std::endl;
    buffer << "frame no: " << frame_number << std::endl;
    buffer << "trigger frame no: " << trig_frame_number << std::endl;
    buffer << "two mhz sample: " << two_mhzsample << std::endl;
    buffer << "sub_run_no: " << sub_run_no << std::endl;
    return buffer.str();
  }

  NevisTPCMetaData() {}

  explicit NevisTPCMetaData(daqAnalysis::HeaderData const& header) {
    slot = header.slot;
    trig_frame_number = header.trig_frame_number;
    frame_number = header.frame_number;
    two_mhzsample = header.two_mhzsample;
    sub_run_no = header.sub_run_no;
  }
};
}
#endif
