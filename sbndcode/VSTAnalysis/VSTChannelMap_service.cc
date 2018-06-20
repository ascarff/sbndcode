////////////////////////////////////////////////////////////////////////
// Class:       VSTChannelMap
// Plugin Type: service (art v2_07_03)
// File:        VSTChannelMap_service.cc
//
// Generated at Mon Jun 18 12:05:37 2018 by Gray Putnam using cetskelgen
// from cetlib version v3_00_01.
////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>
#include <stdlib.h>
#include <cassert>

#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "fhiclcpp/ParameterSet.h"

#include "VSTChannelMap.hh"
#include "HeaderData.hh"

using namespace std;

daqAnalysis::VSTChannelMap::VSTChannelMap(fhicl::ParameterSet const & p, art::ActivityRegistry & areg):
  _n_channels(p.get<unsigned>("n_channels")),
  _n_fem(p.get<unsigned>("n_fem")),
  _slot_offset(p.get<unsigned>("slot_offset", 0)),
  _crate_id(p.get<unsigned>("crate_id", 0)),
  _channel_to_wire(_n_channels, 0),
  _wire_to_channel(_n_channels, 0),
  _channel_per_fem(_n_fem, 0)
{
  unsigned n_channels = p.get<unsigned>("n_channels");

  // Setup channel map

  // read channel map from file if defined
  if (p.has_key("map_file_name")) {
    fstream input(p.get<string>("map_file_name"), fstream::in);

    unsigned line_no = 0;
    string line;
    unsigned channel;
    unsigned wire;
    while (getline(input, line)) {
      istringstream sline(line);
      // checks if line is formatted properly
      if (!(sline >> channel >> wire >> std::ws) && sline.peek() != EOF) {
        cerr << "ERROR: misformatted line " << line_no << endl;
        exit(1);
      }
      _channel_to_wire[channel] = wire;
      _wire_to_channel[wire] = channel;
      line_no ++;
    }
    // checks if file has correct number of lines
    if (line_no != n_channels) {
      cerr << "ERROR: incorrect number of lines defined in file " << line_no << " lines for " << n_channels << " channels." << std::endl;
      exit(1);
    }
  }
  else {
    for (unsigned i = 0; i < _n_channels; i++) {
      _channel_to_wire[i] = i;
      _wire_to_channel[i] = i;
    }
  }

  // Setup channel per FEM
  if (p.has_key("channel_per_fem")) {
    _channel_per_fem = p.get<std::vector<unsigned>>("channel_per_fem");
    if (_channel_per_fem.size() != _n_fem) {
      cerr << "ERROR: incorrect vector size " << _channel_per_fem.size() << " for number of FEM " << _n_fem << std::endl;
      exit(1);
    }
  }
  else {
    // if no vector defined, assume channels are spread equally over FEM
    unsigned channel_per_each_fem = (n_channels / _n_fem) + ((n_channels % _n_fem == 0) ? 0 : 1);
    for (unsigned i = 0; i < _n_fem - 1; i ++) {
      _channel_per_fem[i] = channel_per_each_fem;
    }
    // except for the last one, whcih only gets the number of remaining channels
    _channel_per_fem[_n_fem - 1] = _n_channels - (_n_fem - 1) * channel_per_each_fem;

  }
}

unsigned daqAnalysis::VSTChannelMap::NFEM() const { return _n_fem; } 
unsigned daqAnalysis::VSTChannelMap::NChannels() const { return _n_channels; }
unsigned daqAnalysis::VSTChannelMap::Channel2Wire(unsigned i) const { return _channel_to_wire[i]; }
unsigned daqAnalysis::VSTChannelMap::Wire2Channel(unsigned i) const { return _wire_to_channel[i]; }
unsigned daqAnalysis::VSTChannelMap::NSlotChannels(unsigned i) const { return _channel_per_fem[i]; }

unsigned daqAnalysis::VSTChannelMap::Channel2Wire(unsigned channel, unsigned slot, unsigned crate, bool add_offset) const {
  daqAnalysis::ReadoutChannel readout;
  readout.channel_ind = channel;
  readout.slot = slot + (add_offset ? _slot_offset : 0);
  readout.crate = crate;
  return Channel2Wire(readout);
}

// TODO: FIX?
unsigned daqAnalysis::VSTChannelMap::Channel2Wire(daqAnalysis::ReadoutChannel channel) const {
  unsigned slot = channel.slot - _slot_offset;
  unsigned channel_base = std::accumulate(_channel_per_fem.begin(), _channel_per_fem.begin() + slot, 0);
  unsigned channel_ind = channel_base + channel.channel_ind;
  return Channel2Wire(channel_ind);
}

// TODO: FIX?
daqAnalysis::ReadoutChannel daqAnalysis::VSTChannelMap::Ind2ReadoutChannel(unsigned channel_no) const {
  assert(channel_no < _n_channels);

  daqAnalysis::ReadoutChannel ret;
  ret.crate = _crate_id;
  
  unsigned channel_base = 0;
  for (unsigned i = 0; i < _n_fem; i ++) {
    channel_base += _channel_per_fem[i]; 
    if (channel_base > channel_no) {
      ret.slot = _slot_offset + i; 
      ret.channel_ind = channel_no - (channel_base - _channel_per_fem[i]);
      break;
    }
  }
  return ret;
}

unsigned daqAnalysis::VSTChannelMap::ReadoutChannel2Ind(unsigned channel, unsigned slot, unsigned crate, bool add_offset) const {
  daqAnalysis::ReadoutChannel readout;
  readout.channel_ind = channel;
  readout.slot = slot + (add_offset ? _slot_offset : 0);
  readout.crate = crate;
  return ReadoutChannel2Ind(readout);
}

unsigned daqAnalysis::VSTChannelMap::ReadoutChannel2Ind(daqAnalysis::ReadoutChannel channel) const {
  return std::accumulate(_channel_per_fem.begin(), _channel_per_fem.begin() + (channel.slot - _slot_offset), channel.channel_ind);
}


unsigned daqAnalysis::VSTChannelMap::SlotIndex(daqAnalysis::HeaderData header) const {
  return header.slot - _slot_offset;
}

unsigned daqAnalysis::VSTChannelMap::SlotIndex(daqAnalysis::ReadoutChannel channel) const {
  return channel.slot - _slot_offset;
}

unsigned daqAnalysis::VSTChannelMap::SlotIndex(daqAnalysis::NevisTPCMetaData metadata) const {
  return metadata.slot - _slot_offset;
}


bool daqAnalysis::VSTChannelMap::IsGoodSlot(unsigned slot) const {
  // negative overflow will wrap around and also be large than n_fem_per_crate,
  // so this covers both the case where the slot id is too big and too small
  return (slot - _slot_offset) < _n_fem;
}

DEFINE_ART_SERVICE(daqAnalysis::VSTChannelMap)

