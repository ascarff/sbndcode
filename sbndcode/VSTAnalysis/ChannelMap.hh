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
    uint32_t crate;
    uint32_t slot;
    uint32_t channel_ind;
    uint32_t slot_offset;
  };

  // TODO @INSTALLATION: Implement for VST
  static readout_channel Wire2Channel(wire_id_t wire, uint32_t slot_offset=0) {
    // TEMPORARY IMPLEMENTATION FOR TEST ON LARIAT DATA

    uint32_t crate = wire / (ChannelMap::n_fem_per_crate * ChannelMap::n_channel_per_fem);
    uint32_t slot = (wire / ChannelMap::n_channel_per_fem) % ChannelMap::n_fem_per_crate + slot_offset;
    // channel ind counts up from 0 -> N. 64 channels per fem.
    uint32_t channel_ind = wire % (ChannelMap::n_fem_per_crate * ChannelMap::n_channel_per_fem);

    return readout_channel {crate, slot, channel_ind};
  }

  //TODO @INSTALLATION: Imeplement
  static wire_id_t Channel2Wire(readout_channel channel) {
    // TEMPORARY IMPLEMENTATION FOR TEST ON LARIAT DATA AND NEVIST TEST STAND DATA
    wire_id_t wire = (channel.slot - channel.slot_offset) * ChannelMap::n_channel_per_fem + channel.channel_ind;
    return wire;
  }

  static wire_id_t Channel2Wire(uint32_t crate, uint32_t fem, uint32_t channel_id, uint32_t slot_offset=0) {
    readout_channel channel {crate, fem, channel_id, slot_offset};
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
  // TEMPORARY IMPLEMENTATION FOR TEST ON NEVIS TEST STAND DATA
  static const uint32_t n_crate = 1;
  static const uint32_t n_fem_per_crate = 10;
  static const uint32_t n_channel_per_fem = 64;
  static const uint32_t n_wire = 640;
};
#endif

