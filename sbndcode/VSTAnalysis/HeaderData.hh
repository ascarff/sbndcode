#ifndef _sbnddaq_analysis_HeaderData
#define _sbnddaq_analysis_HeaderData

#include <string>
#include <iostream>
#include <sstream> 
#include <stdlib.h>
#include <ctime>

namespace daqAnalysis {
// Header Data associated with a NevisTPCHeader
class HeaderData {
  public:
  // derived quantities
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

  // raw header
  uint32_t id_and_slot_word;
  uint32_t word_count_word;
  uint32_t event_num_word;
  uint32_t frame_num_word;
  uint32_t checksum_word;
  uint32_t trig_frame_sample_word;

  // timing info
  uint32_t run_no;
  uint32_t sub_run_no;
  uint32_t art_event_no;

  uint32_t time_stamp_hi;
  uint32_t time_stamp_lo;

  // by default make words noticable
  // Nevis uses DEADBEEF as a default, so distinguish from
  // that use BEEFDEAD
  HeaderData():
    crate(0xFF),
    slot(0xFF),
    event_number(0xBEEFDEAD),
    frame_number(0xBEEFDEAD),
    checksum(0xBEEFDEAD),
    computed_checksum(0xBEEFDEAD),
    adc_word_count(0xBEEFDEAD),
    trig_frame_number(0xBEEFDEAD),
    id_and_slot_word(0xBEEFDEAD),
    word_count_word(0xBEEFDEAD),
    event_num_word(0xBEEFDEAD),
    frame_num_word(0xBEEFDEAD),
    checksum_word(0xBEEFDEAD),
    trig_frame_sample_word(0xBEEFDEAD),
    run_no(0),
    sub_run_no(0)
  {}

  // print the data -- for debugging
  std::string Print() const {
    std::stringstream buffer;
    buffer << "run id: " << run_no << std::endl;
    buffer << "sub run id: " << sub_run_no << std::endl;

    buffer << "crate: " << ((unsigned)crate) << std::endl;
    buffer << "slot: " << ((unsigned)slot) << std::endl;
    buffer << "event no: " << event_number << std::endl;
    buffer << "frame no: " << frame_number << std::endl;
    buffer << "checksum: " << checksum << std::endl;
    buffer << "computed checksum: " << computed_checksum << std::endl;
    buffer << "adc word count: " << adc_word_count << std::endl;
    buffer << "trigger frame no: " << trig_frame_number << std::endl;
    buffer << "frame time: " << frame_time << std::endl;
    buffer << "trigger frame time: " << trig_frame_time << std::endl;
    buffer << "ART event time lo: " << time_stamp_lo << std::endl;
    buffer << "ART event time hi: " << time_stamp_hi << std::endl;
    buffer << std::hex << "checksum: " << checksum << std::endl;
    buffer << std::hex << "computed checksum: " << computed_checksum << std::endl;

    return buffer.str();
  }

  // print raw data -- for debugging
  std::string PrintRaw() const {
    std::stringstream buffer;
    buffer << std::hex << "id_and_slot_word: " << id_and_slot_word << std::endl;
    buffer << std::hex << "word_count_word:  " << word_count_word << std::endl;
    buffer << std::hex << "event_num_word:   " << event_num_word << std::endl;
    buffer << std::hex << "frame_num_word:   " << frame_num_word << std::endl;
    buffer << std::hex << "checksum_word:    " << checksum_word << std::endl;
    buffer << std::hex << "trig_frame_sample_word: " << trig_frame_sample_word << std::endl;

    return buffer.str();
  }
};
}

#endif
