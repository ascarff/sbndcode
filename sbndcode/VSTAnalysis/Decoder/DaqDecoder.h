#ifndef DaqDecoder_h
#define DaqDecoder_h
////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoder
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoder.h
//
// Generated at Thu Feb  8 16:41:18 2018 by Gray Putnam using cetskelgen
// from cetlib version v3_01_03.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "lardataobj/RawData/RawDigit.h"
#include "artdaq-core/Data/Fragment.hh"
#include "canvas/Utilities/InputTag.h"

#include "sbnddaq-datatypes/Overlays/NevisTPCFragment.hh"

#include "../HeaderData.hh"

/*
  * The Decoder module takes as input "NevisTPCFragments" and
  * outputs raw::RawDigits. It also handles in and all issues
  * with the passed in header and fragments (or at least it will).
*/

namespace daq {
  class DaqDecoder;
}


class daq::DaqDecoder : public art::EDProducer {
public:
  explicit DaqDecoder(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  DaqDecoder(DaqDecoder const &) = delete;
  DaqDecoder(DaqDecoder &&) = delete;
  DaqDecoder & operator = (DaqDecoder const &) = delete;
  DaqDecoder & operator = (DaqDecoder &&) = delete;

  // Required functions.
  void produce(art::Event & e) override;

  // get checksum from a Nevis fragment
  static uint32_t compute_checksum(sbnddaq::NevisTPCFragment &fragment);

private:
  // process an individual fragment inside an art event
  void process_fragment(const artdaq::Fragment &frag,
    std::unique_ptr<std::vector<raw::RawDigit>> &product_collection,
    std::unique_ptr<std::vector<daqAnalysis::HeaderData>> &header_collection);

  // validate Nevis header
  // TODO: report errors in header
  void validate_header(const sbnddaq::NevisTPCHeader *header); 

  // Gets the WIRE ID of the channel. This wire id can be then passed
  // to the Lariat geometry.
  raw::ChannelID_t get_wire_id(const sbnddaq::NevisTPCHeader *header, uint16_t nevis_channel_id);
  art::InputTag _tag;
  int _wait_sec;
  int _wait_usec;
  bool _produce_header;
  bool _calc_baseline;
};

#endif /* DaqDecoder_h */
