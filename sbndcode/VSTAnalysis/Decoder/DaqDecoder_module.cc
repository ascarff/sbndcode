////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoder
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoder.cxx
//
// Generated at Thu Feb  8 16:41:18 2018 by Gray Putnam using cetskelgen
// from cetlib version v3_01_03.
////////////////////////////////////////////////////////////////////////

#include "DaqDecoder.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>
#include <iostream>
#include <stdlib.h>
#include <chrono>
#include <thread>

#include "art/Framework/Core/ModuleMacros.h"

#include "artdaq-core/Data/Fragment.hh"

#include "lardataobj/RawData/RawDigit.h"

#include "sbnddaq-datatypes/Overlays/NevisTPCFragment.hh"
#include "sbnddaq-datatypes/NevisTPC/NevisTPCTypes.hh"
#include "sbnddaq-datatypes/NevisTPC/NevisTPCUtilities.hh"

#include "../HeaderData.hh"
#include "../ChannelMap.hh"
#include "../Mode.hh"

DEFINE_ART_MODULE(daq::DaqDecoder)

// constructs a header data object from a nevis header
// construct from a nevis header
daqAnalysis::HeaderData Nevis2HeaderData(sbnddaq::NevisTPCFragment fragment, double frame_to_dt=1.0) {

  const sbnddaq::NevisTPCHeader *raw_header = fragment.header();
  daqAnalysis::HeaderData ret;

  ret.crate = raw_header->getFEMID();
  ret.slot = raw_header->getSlot();
  ret.event_number = raw_header->getEventNum();
  ret.frame_number = raw_header->getFrameNum();
  ret.checksum = raw_header->getChecksum();
  ret.computed_checksum = daq::DaqDecoder::compute_checksum(fragment);
  ret.adc_word_count = raw_header->getADCWordCount();
  ret.trig_frame_number = raw_header->getTrigFrame();
  
  ret.frame_time = ret.frame_number * frame_to_dt;
  ret.trig_frame_time = ret.trig_frame_number * frame_to_dt;

  // draw header stuff
  ret.id_and_slot_word = raw_header->id_and_slot;
  ret.word_count_word = raw_header->word_count;
  ret.event_num_word = raw_header->event_num;
  ret.frame_num_word = raw_header->frame_num;
  ret.checksum_word = raw_header->checksum;
  ret.trig_frame_sample_word = raw_header->trig_frame_sample;

  return ret;
}

daq::DaqDecoder::DaqDecoder(fhicl::ParameterSet const & param)
  : _tag("daq","NEVISTPC")
{
  // amount of time to wait in between processing events
  // useful for debugging redis
  double wait_time = param.get<double>("wait_time", -1 /* units of seconds */);
  _wait_sec = (int) wait_time;
  _wait_usec = (int) (wait_time / 1000000);
  
  // produce stuff
  produces<std::vector<raw::RawDigit>>();
  _produce_header = param.get<bool>("produce_header", false);
  if (_produce_header) {
    produces<std::vector<daqAnalysis::HeaderData>>();
  }
}

void daq::DaqDecoder::produce(art::Event & event)
{
  if (_wait_sec >= 0) {
    std::this_thread::sleep_for(std::chrono::seconds(_wait_sec) + std::chrono::microseconds(_wait_usec));
  }
  auto const& daq_handle = event.getValidHandle<std::vector<artdaq::Fragment>>(_tag);

  // storage for waveform
  std::unique_ptr<std::vector<raw::RawDigit>> product_collection(new std::vector<raw::RawDigit>);
  // storage for header info
  std::unique_ptr<std::vector<daqAnalysis::HeaderData>> header_collection(new std::vector<daqAnalysis::HeaderData>);
  for (auto const &rawfrag: *daq_handle) {
    process_fragment(rawfrag, product_collection, header_collection);
  }

  event.put(std::move(product_collection));
  if (_produce_header) {
    event.put(std::move(header_collection));
  }
}


raw::ChannelID_t daq::DaqDecoder::get_wire_id(const sbnddaq::NevisTPCHeader *header, uint16_t nevis_channel_id) {
 daqAnalysis::ChannelMap::readout_channel channel {header->getSlot(), header->getFEMID(), (size_t) nevis_channel_id };
 // rely on ChannelMap for implementation
 return daqAnalysis::ChannelMap::Channel2Wire(channel);
}

void daq::DaqDecoder::process_fragment(const artdaq::Fragment &frag, 
  std::unique_ptr<std::vector<raw::RawDigit>> &product_collection,
  std::unique_ptr<std::vector<daqAnalysis::HeaderData>> &header_collection) {

  // convert fragment to Nevis fragment
  sbnddaq::NevisTPCFragment fragment(frag);

  validate_header(fragment.header());

  std::unordered_map<uint16_t,sbnddaq::NevisTPC_Data_t> waveform_map;
  size_t n_waveforms = fragment.decode_data(waveform_map);
  (void)n_waveforms;

  if (_produce_header) {
    // Construct HeaderData from the Nevis Header and throw it in the collection
    header_collection->push_back(Nevis2HeaderData(fragment));
  }

  for (auto waveform: waveform_map) {
    std::vector<int16_t> raw_digits_waveform;
    raw::ChannelID_t wire_id = get_wire_id(fragment.header(), waveform.first);
    // TODO: is this too expensive an operation?
    for (auto digit: waveform.second) {
      raw_digits_waveform.push_back( (int16_t) digit);
    }  
    // construct the next RawDigit object
    product_collection->emplace_back(wire_id, raw_digits_waveform.size(), raw_digits_waveform);
    // calculate the mode and set it as the pedestal
    (*product_collection)[product_collection->size() - 1].SetPedestal( Mode(raw_digits_waveform) ); 
  }
}
void daq::DaqDecoder::validate_header(const sbnddaq::NevisTPCHeader *header) {
  // TODO: implement
  (void) header;
  return; 
}

// just copied from sbnddaq-datatypes
// we should tale this out at some point
sbnddaq::NevisTPCWordType_t get_word_type( uint16_t word )
{
  if( (word & 0xF000) == 0x4000 ) return sbnddaq::NevisTPCWordType_t::kChannelHeader;
  if( (word & 0xF000) == 0x0000 ) return sbnddaq::NevisTPCWordType_t::kADC;
  if( (word & 0xC000) == 0x8000 ) return sbnddaq::NevisTPCWordType_t::kADCHuffman; // Look for headers first
  if( (word & 0xF000) == 0x5000 ) return sbnddaq::NevisTPCWordType_t::kChannelEnding;
  else
    return sbnddaq::NevisTPCWordType_t::kUnknown;
}

// Computes the checksum, given a nevis tpc header
// Ideally this would be in sbnddaq-datatypes, but it's not and I can't
// make changes to it, so put it here for now
uint32_t daq::DaqDecoder::compute_checksum(sbnddaq::NevisTPCFragment &fragment) {
  uint32_t checksum = 0;

  const sbnddaq::NevisTPC_ADC_t* data_ptr = fragment.data();
  size_t n_words = fragment.header()->getADCWordCount();

  for (size_t word_ind = 0; word_ind < n_words; word_ind++) {
    const sbnddaq::NevisTPC_ADC_t* word_ptr = data_ptr + word_ind;
    switch (get_word_type(*word_ptr)) {
      // checksum gets incremented for each of these cases
      case sbnddaq::NevisTPCWordType_t::kChannelHeader:
      case sbnddaq::NevisTPCWordType_t::kADC:
      case sbnddaq::NevisTPCWordType_t::kADCHuffman:
      case sbnddaq::NevisTPCWordType_t::kChannelEnding:
      case sbnddaq::NevisTPCWordType_t::kUnknown:
        checksum += *word_ptr;
        break;
    }
  }
  // only first 6 bytes of checksum are used
  return checksum & 0xFFFFFF;

}



 

