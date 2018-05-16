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

// TODO: Implement these
// get run id from fragment
inline unsigned RunNo(const artdaq::Fragment &frag) {
  return 0;
}
// get sub-run id from fragment
inline unsigned SubRunNo(const artdaq::Fragment &frag) {
  static unsigned ind = 0;
  return ind++;
}


// constructs a header data object from a nevis header
// construct from a nevis header
daqAnalysis::HeaderData Fragment2HeaderData(const artdaq::Fragment &frag, double frame_to_dt=1.0, bool calc_checksum=false) {
  sbnddaq::NevisTPCFragment fragment(frag);

  const sbnddaq::NevisTPCHeader *raw_header = fragment.header();
  daqAnalysis::HeaderData ret;

  ret.crate = raw_header->getFEMID();
  ret.slot = raw_header->getSlot();
  ret.event_number = raw_header->getEventNum();
  ret.frame_number = raw_header->getFrameNum();
  ret.checksum = raw_header->getChecksum();

  if (calc_checksum) {
    ret.computed_checksum = daq::DaqDecoder::compute_checksum(fragment);
  }
  else {
    ret.computed_checksum = 0;
  }

  ret.adc_word_count = raw_header->getADCWordCount();
  ret.trig_frame_number = raw_header->getTrigFrame();
  
  ret.frame_time = ret.frame_number * frame_to_dt;
  ret.trig_frame_time = ret.trig_frame_number * frame_to_dt;

  // run id stuff
  ret.run_no = RunNo(frag); 
  ret.sub_run_no = SubRunNo(frag);

  // raw header stuff
  ret.id_and_slot_word = raw_header->id_and_slot;
  ret.word_count_word = raw_header->word_count;
  ret.event_num_word = raw_header->event_num;
  ret.frame_num_word = raw_header->frame_num;
  ret.checksum_word = raw_header->checksum;
  ret.trig_frame_sample_word = raw_header->trig_frame_sample;

  return ret;
}

daq::DaqDecoder::DaqDecoder(fhicl::ParameterSet const & param): 
  _tag(param.get<std::string>("raw_data_label", "daq"),param.get<std::string>("fragment_type_label", "NEVISTPC")),
  _config(param),
  _last_event_number(0),
  _last_trig_frame_number(0)
{
  
  // produce stuff
  produces<std::vector<raw::RawDigit>>();
  if (_config.produce_header) {
    produces<std::vector<daqAnalysis::HeaderData>>();
  }
}

daq::DaqDecoder::Config::Config(fhicl::ParameterSet const & param) {
  // amount of time to wait in between processing events
  // useful for debugging redis
  double wait_time = param.get<double>("wait_time", -1 /* units of seconds */);
  wait_sec = (int) wait_time;
  wait_usec = (int) (wait_time / 1000000.);
  // whether to calcualte the pedestal (and set it in SetPedestal())
  baseline_calc = param.get<bool>("baseline_calc", false);
  // whether to put HeaderInfo in the art root file
  produce_header = param.get<bool>("produce_header", false);
  // whether to check if Header looks good and print out error info
  validate_header = param.get<bool>("validate_header", false);
  // how many adc values to skip in mode/pedestal finding
  n_mode_skip = param.get<unsigned>("n_mode_skip", 1);
  // whether to verify checksum
  calc_checksum = param.get<bool>("calc_checksum", false);
}

void daq::DaqDecoder::produce(art::Event & event)
{
  if (_config.wait_sec >= 0) {
    std::this_thread::sleep_for(std::chrono::seconds(_config.wait_sec) + std::chrono::microseconds(_config.wait_usec));
  }
  auto const& daq_handle = event.getValidHandle<artdaq::Fragments>(_tag);
  
  // storage for waveform
  std::unique_ptr<std::vector<raw::RawDigit>> product_collection(new std::vector<raw::RawDigit>);
  // storage for header info
  std::unique_ptr<std::vector<daqAnalysis::HeaderData>> header_collection(new std::vector<daqAnalysis::HeaderData>);
  for (auto const &rawfrag: *daq_handle) {
    process_fragment(rawfrag, product_collection, header_collection);
  }

  event.put(std::move(product_collection));
  if (_config.produce_header) {
    event.put(std::move(header_collection));
  }
}


raw::ChannelID_t daq::DaqDecoder::get_wire_id(const sbnddaq::NevisTPCHeader *header, uint16_t nevis_channel_id) {
 daqAnalysis::ChannelMap::readout_channel channel {header->getSlot(), header->getFEMID(), nevis_channel_id };
 // rely on ChannelMap for implementation
 return daqAnalysis::ChannelMap::Channel2Wire(channel);
}

void daq::DaqDecoder::process_fragment(const artdaq::Fragment &frag, 
  std::unique_ptr<std::vector<raw::RawDigit>> &product_collection,
  std::unique_ptr<std::vector<daqAnalysis::HeaderData>> &header_collection) {

  // convert fragment to Nevis fragment
  sbnddaq::NevisTPCFragment fragment(frag);


  std::unordered_map<uint16_t,sbnddaq::NevisTPC_Data_t> waveform_map;
  size_t n_waveforms = fragment.decode_data(waveform_map);
  (void)n_waveforms;

  if (_config.produce_header || _config.validate_header) {
    auto header_data = Fragment2HeaderData(frag, 1.0, _config.calc_checksum);
    if (_config.produce_header) {
      // Construct HeaderData from the Nevis Header and throw it in the collection
      header_collection->push_back(header_data);
    }
    if (_config.validate_header) {
      validate_header(header_data);
    }
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
    if (_config.baseline_calc) {
      (*product_collection)[product_collection->size() - 1].SetPedestal( Mode(raw_digits_waveform, _config.n_mode_skip) ); 
    }
  }
}
void daq::DaqDecoder::validate_header(const daqAnalysis::HeaderData &header) {
  bool printed = false;
  if (_config.calc_checksum && header.checksum != header.computed_checksum) {
   unsigned checksum = header.checksum;
   unsigned computed_checksum = header.computed_checksum;
    mf::LogError("Bad Header") << std::hex << "computed checksum " << 
      checksum << " does not match firmware checksum " << computed_checksum ;
    printed = true;
  }
  if (header.slot >= daqAnalysis::ChannelMap::n_fem_per_crate) {
    unsigned n_fem_per_crate = daqAnalysis::ChannelMap::n_fem_per_crate;
    unsigned slot = header.slot;
    mf::LogError("Bad Header") <<  "Slot index of FEM (" << slot << 
      ") mismatch with number of FEM slots per crate (" << n_fem_per_crate << ")" ;
    printed = true;
  }
  if (header.crate >= daqAnalysis::ChannelMap::n_crate) {
    unsigned crate = header.crate;
    unsigned n_crate = daqAnalysis::ChannelMap::n_crate;
    mf::LogError("Bad Header") << "Crate id (" << crate << 
      ") too large for total number of crates (" << n_crate << ")" ;
    printed = true;
  }
  if (header.Ind() >= daqAnalysis::ChannelMap::NFEM()) {
    unsigned ind = header.Ind();
    unsigned n_fem = daqAnalysis::ChannelMap::NFEM();
    mf::LogError("Bad Header") << "Global index of FEM (" << ind << 
      ") too large for total number of FEM's (" << n_fem << ")" ;
    printed = true;
  }
  if (header.adc_word_count == 0) {
    unsigned fem_ind = header.Ind();
    unsigned crate = header.crate;
    unsigned slot = header.slot;
    mf::LogWarning("Bad Header") << "ADC Word Count in crate " << crate << ", slot " << 
      slot << ", fem ID " << fem_ind << "is 0" ;
    printed = true;
  }
  if (header.event_number < _last_event_number) {
    unsigned event_number = header.event_number;
    mf::LogError("Bad Header") << "Non incrementing event numbers. Last event number: " << 
      _last_event_number << ". This event number: " << event_number ;
    printed = true;
  }
  if (header.trig_frame_number < _last_trig_frame_number) {
    unsigned trig_frame_number = header.trig_frame_number;
    mf::LogError("Bad Header") << "Non incrementing trig frame numbers. Last trig frame: " << 
      _last_trig_frame_number << " This trig frame: " << trig_frame_number ;
    printed = true;
  }
  if (printed) {
     mf::LogInfo("Bad Header") << "Header Info:\n" <<  header.Print() ;
  }
  // store numbers for next time
  _last_event_number = header.event_number;
  _last_trig_frame_number = header.trig_frame_number;
  return; 
}

// Computes the checksum, given a nevis tpc header
// Ideally this would be in sbnddaq-datatypes, but it's not and I can't
// make changes to it, so put it here for now
uint32_t daq::DaqDecoder::compute_checksum(sbnddaq::NevisTPCFragment &fragment) {
  uint32_t checksum = 0;

  const sbnddaq::NevisTPC_ADC_t* data_ptr = fragment.data();
  // RETURN VALUE OF getADCWordCount IS OFF BY 1
  size_t n_words = fragment.header()->getADCWordCount() + 1;

  for (size_t word_ind = 0; word_ind < n_words; word_ind++) {
    const sbnddaq::NevisTPC_ADC_t* word_ptr = data_ptr + word_ind;
    checksum += *word_ptr;
  }
  // only first 6 bytes of checksum are used
  return checksum & 0xFFFFFF;

}



 

