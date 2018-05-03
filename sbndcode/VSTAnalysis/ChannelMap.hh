#ifndef ChannelMap_h
#define ChannelMap_h

#include "lardataobj/RawData/RawDigit.h"

namespace daqAnalysis {
  class ChannelMap;
}

// maps wire id's to and from card no, fem no, channel index
class daqAnalysis::ChannelMap {
public:
  typedef uint16_t wire_id_t;
  struct readout_channel {
    size_t crate;
    size_t slot;
    size_t channel_ind;
  };

  // TODO @INSTALLATION: Implement for VST
  static readout_channel Wire2Channel(wire_id_t wire) {
    // TEMPORARY IMPLEMENTATION FOR TEST ON LARIAT DATA

    // 480 total channels. 64 channels per fem, 8 fem per crate. 1 crate total.
    size_t crate = wire / (ChannelMap::n_fem_per_crate * ChannelMap::n_channel_per_fem);
    // 480 total channels. 64 channels per fem. 8 fem per crate.
    size_t slot = (wire / ChannelMap::n_channel_per_fem) % ChannelMap::n_fem_per_crate;
    // channel ind counts up from 0 -> 480. 64 channels per fem.
    size_t channel_ind = wire % (ChannelMap::n_fem_per_crate * ChannelMap::n_channel_per_fem);

    return readout_channel {crate, slot, channel_ind};
  }

  //TODO @INSTALLATION: Imeplement
  static wire_id_t Channel2Wire(readout_channel channel) {
    // TEMPORARY IMPLEMENTATION FOR TEST ON LARIAT DATA
    //wire_id_t wire = channel.crate * ChannelMap::n_fem_per_crate * ChannelMap::n_channel_per_fem + channel.slot * ChannelMap::n_channel_per_fem + channel.channel_ind;
    // TEMPORARY IMPLEMENTATION FOR TEST ON NEVIS TEST STAND DATA
    wire_id_t wire = channel.channel_ind;
    return wire;
  }

  static wire_id_t Channel2Wire(size_t card_no, size_t fem, size_t channel_id) {
    readout_channel channel {card_no, fem, channel_id};
    return Channel2Wire(channel);
  }

  // TODO @INSTALLATION: Implement
  // 1 == induction plane
  // 2 == collection plane
  static unsigned PlaneType(unsigned wire_no) {
    // TEMPORARY IMPLEMENTATION FOR TEST ON LARIAT DATA
    bool is_induction = wire_no < 240;
    if (is_induction) {
      return 1;
    }
    else {
      return 2;
    }
  }

  // TODO @INSTALLATION: Implement
  // TEMPORARY IMPLEMENTATION FOR TEST ON LARIAT DATA
  inline static constexpr unsigned NFEM() {
    return (n_wire / n_channel_per_fem) + ((n_wire % n_channel_per_fem == 0) ? 0:1);
  }

  // TODO @INSTALLATION: Implement
  // TEMPORARY IMPLEMENTATION FOR TEST ON LARIAT DATA
  /*
  static const size_t n_crate = 1;
  static const size_t n_fem_per_crate = 8;
  static const size_t n_channel_per_fem = 64;
  static const size_t n_wire = 480;
  // TEMPORARY IMPLEMENTATION FOR TEST ON NEVIS TEST STAND DATA
  */
  static const size_t n_crate = 1;
  static const size_t n_fem_per_crate = 8;
  static const size_t n_channel_per_fem = 64;
  static const size_t n_wire = 64;
};
#endif

