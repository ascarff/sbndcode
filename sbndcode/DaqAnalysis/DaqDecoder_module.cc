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

DEFINE_ART_MODULE(daq::DaqDecoder)


daq::DaqDecoder::DaqDecoder(fhicl::ParameterSet const & param)
  : _tag("daq","NEVISTPC")
{
  double wait_time = param.get<double>("wait_time", -1 /* units of seconds */);
  _wait_sec = (int) wait_time;
  _wait_usec = (int) (wait_time / 1000000);
  
  // produce stuff
  produces<std::vector<raw::RawDigit>>();
}

void daq::DaqDecoder::produce(art::Event & event)
{
  if (_wait_sec >= 0) {
    std::this_thread::sleep_for(std::chrono::seconds(_wait_sec) + std::chrono::microseconds(_wait_usec));
  }
  auto const& daq_handle = event.getValidHandle<std::vector<artdaq::Fragment>>(_tag);

  std::unique_ptr<std::vector<raw::RawDigit>> product_collection(new std::vector<raw::RawDigit>);
  for (auto const &rawfrag: *daq_handle) {
    process_fragment(rawfrag, product_collection);
  }
  event.put(std::move(product_collection));
}

void daq::DaqDecoder::process_fragment(const artdaq::Fragment &frag, 
  std::unique_ptr<std::vector<raw::RawDigit>> &product_collection) {

  sbnddaq::NevisTPCFragment fragment(frag);

  validate_header(fragment.header());

  std::unordered_map<uint16_t,sbnddaq::NevisTPC_Data_t> waveform_map;
  size_t n_waveforms = fragment.decode_data(waveform_map);
  (void)n_waveforms;

  for (auto waveform: waveform_map) {
    std::vector<int16_t> raw_digits_waveform;
    // TODO: is this too expensive an operation?
    for (auto digit: waveform.second) {
      raw_digits_waveform.push_back( (int16_t) digit);
    }  
    product_collection->emplace_back(waveform.first, raw_digits_waveform.size(), raw_digits_waveform);
  }
}
void daq::DaqDecoder::validate_header(const sbnddaq::NevisTPCHeader *header) {
  // TODO: implement
  return; 
}


 

