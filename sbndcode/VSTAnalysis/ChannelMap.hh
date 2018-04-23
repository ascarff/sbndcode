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
  struct board_channel {
    size_t slot_no;
    size_t fem_no;
    size_t channel_ind;
  };

  // TODO @INSTALLATION: Implement for VST
  static board_channel Wire2Channel(wire_id_t wire) {
    // TEMPORARY IMPLEMENTATION FOR TEST ON LARIAT DATA

    // slots count up from 0 -> 480. 64 channels per board. 8 boards total.
    size_t slot_no = wire / (ChannelMap::n_fem_per_board * ChannelMap::n_channel_per_fem);
    // fems count up from 0 -> 480. 16 channels per fem. 4 fem per board.
    size_t fem_no = (wire / ChannelMap::n_channel_per_fem) % ChannelMap::n_fem_per_board;
    // channel ind counts up from 0 -> 480. 64 channels per reset (board)
    size_t channel_ind = wire % (ChannelMap::n_fem_per_board * ChannelMap::n_channel_per_fem);

    return board_channel {slot_no, fem_no, channel_ind};
  }

  //TODO @INSTALLATION: Imeplement
  static wire_id_t Channel2Wire(board_channel channel) {
    // TEMPORARY IMPLEMENTATION FOR TEST ON LARIAT DATA
    wire_id_t wire = channel.slot_no * ChannelMap::n_fem_per_board * ChannelMap::n_channel_per_fem + channel.fem_no * ChannelMap::n_channel_per_fem + channel.channel_ind;
    return wire;
  }

  static wire_id_t Channel2Wire(size_t card_no, size_t fem_no, size_t channel_id) {
    board_channel channel {card_no, fem_no, channel_id};
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
  static const size_t n_boards = 8;
  static const size_t n_fem_per_board = 4;
  static const size_t n_channel_per_fem = 16;
  static const size_t n_wire = 480;
};
#endif

