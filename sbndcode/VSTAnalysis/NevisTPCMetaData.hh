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
    uint64_t time;
    uint16_t slot;
    uint32_t two_mhzsample;
    uint32_t sub_run_no;

  // print the data -- for debugging
  std::string Print() const {
    std::stringstream buffer;
    buffer << "slot: " << slot << std::endl;
    buffer << "two mhz sample: " << two_mhzsample << std::endl;
    buffer << "sub_run_no: " << sub_run_no << std::endl;
    buffer << "time: " << time << std::endl;
    return buffer.str();
  }

  std::string PrintRaw() const {
    std::stringstream buffer;
    buffer << "slot: " << slot << std::endl;
    buffer << "time: " << std::hex << time << std::endl;
    return buffer.str();
  }

  NevisTPCMetaData():
  time(0),  
  slot(0)
  {}

  explicit NevisTPCMetaData(daqAnalysis::HeaderData const& header) {
    slot = header.slot;
    time = header.frame_time_stamp;
  }
};
}
#endif
