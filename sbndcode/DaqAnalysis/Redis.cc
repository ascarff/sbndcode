#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <ctime>

#include <hiredis/hiredis.h>

#include "Redis.hh"
#include "ChannelData.hh"
#include "HeaderData.hh"
#include "Noise.hh"
#include "Analysis.h"
#include "ChannelMap.hh"

using namespace daqAnalysis;
using namespace std;

Redis::Redis(std::vector<unsigned> &stream_take, std::vector<unsigned> &stream_expire, int snapshot_time): 
  _snapshot_time(snapshot_time),
  _stream_take(stream_take),
  _stream_expire(stream_expire),
  _channel_rms(stream_take.size(), StreamDataCache(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _channel_baseline(stream_take.size(), StreamDataCache(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _channel_hit_occupancy(stream_take.size(), StreamDataCache(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fem_rms(stream_take.size(), StreamDataCache(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fem_baseline(stream_take.size(), StreamDataCache(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fem_hit_occupancy(stream_take.size(), StreamDataCache(ChannelMap::n_fem_per_board* ChannelMap::n_boards))
{
  context = redisConnect("127.0.0.1", 6379);
  std::cout << "REDIS CONTEXT: " << context << std::endl;
  if (context != NULL && context->err) {
    std::cerr << "Redis error: " <<  context->errstr << std::endl;
    exit(1);
  }
  _start = 0;
}

Redis::~Redis() {
  redisFree(context);
}

void Redis::StartSend() {
  _now = std::time(nullptr);
  // first time startup
  if (_start == 0) _start = _now;
}

void Redis::FinishSend() {
  _last = _now;
}

void Redis::SendHeaderData(vector<HeaderData> *header_data) {
  void *reply;
  for (auto &header: *header_data) {
    for (size_t i = 0; i < _stream_take.size(); i++) {
      if ( (_now - _start) % _stream_take[i] == 0) {
	reply = redisCommand(context, "SET stream/%i:%i:checksum:fem%i:slot%i %i", _stream_take[i], _now/_stream_take[i], header.fem_id, header.slot_id, header.checksum);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:checksum:fem%i:slot%i %i", _stream_take[i], _now/_stream_take[i], header.fem_id, header.slot_id, _stream_expire[i]);
	freeReplyObject(reply);
	reply = redisCommand(context, "SET stream/%i:%i:frameno:fem%i:slot%i %i", _stream_take[i], _now/_stream_take[i], header.fem_id, header.slot_id, header.frame_number);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:frameno:fem%i:slot%i %i", _stream_take[i], _now/_stream_take[i], header.fem_id, header.slot_id, _stream_expire[i]);
	freeReplyObject(reply);
	reply = redisCommand(context, "SET stream/%i:%i:eventno:fem%i:slot%i %i", _stream_take[i], _now/_stream_take[i], header.fem_id, header.slot_id, header.event_number);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:eventno:fem%i:slot%i %i", _stream_take[i], _now/_stream_take[i], header.fem_id, header.slot_id, _stream_expire[i]);
	freeReplyObject(reply);
	reply = redisCommand(context, "SET stream/%i:%i:trigframeno:fem%i:slot%i %i", _stream_take[i], _now/_stream_take[i], header.fem_id, header.slot_id, header.trig_frame_number);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:trigframeno:fem%i:slot%i %i", _stream_take[i], _now/_stream_take[i], header.fem_id, header.slot_id, _stream_expire[i]);
	freeReplyObject(reply);
      }
    }
  }
}

void Redis::SendChannel(unsigned stream_index) {
  // send everything in channel streams

  // TODO: Report failures
  void *reply;
 
  for (unsigned i = 0; i < _channel_rms[stream_index].Size(); i++) {  
    reply = redisCommand(context, "SET stream/%i:%i:baseline:wire:%i %f", _stream_take[stream_index], _now/_stream_take[stream_index], i, _channel_baseline[stream_index].Take(i));
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/%i:%i:baseline:wire:%i %i", _stream_take[stream_index], _now/_stream_take[stream_index], i, _stream_expire[stream_index]);
    freeReplyObject(reply);
    reply = redisCommand(context, "SET stream/%i:%i:rms:wire:%i %f", _stream_take[stream_index], _now/_stream_take[stream_index], i, _channel_rms[stream_index].Take(i));
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/%i:%i:rms:wire:%i %i", _stream_take[stream_index], _now/_stream_take[stream_index], i, _stream_expire[stream_index]);
    freeReplyObject(reply);
    reply = redisCommand(context, "SET stream/%i:%i:hit_occupancy:wire:%i %f", _stream_take[stream_index], _now/_stream_take[stream_index], i, _channel_hit_occupancy[stream_index].Take(i));
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/%i:%i:hit_occupancy:wire:%i %i", _stream_take[stream_index], _now/_stream_take[stream_index], i, _stream_expire[stream_index]);
    freeReplyObject(reply);
  } 
}

void Redis::SendFem(unsigned stream_index) {
  // TODO: Report failures
  void *reply;
  for (unsigned i = 0; i < _fem_rms[stream_index].Size(); i++) {
    unsigned fem = i;
    unsigned board = ChannelMap::Board(fem);
    reply = redisCommand(context, "SET stream/%i:%i:rms:board:%i:fem:%i %f", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _fem_rms[stream_index].Take(fem));
    freeReplyObject(reply);       
    reply = redisCommand(context, "EXPIRE stream/%i:%i:rms:board:%i:fem:%i %i", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _stream_expire[stream_index]);
    freeReplyObject(reply);       
    reply = redisCommand(context, "SET stream/%i:%i:baseline:board:%i:fem:%i %f", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _fem_baseline[stream_index].Take(fem));
    freeReplyObject(reply);       
    reply = redisCommand(context, "EXPIRE stream/%i:%i:baseline:board:%i:fem:%i %i", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _stream_expire[stream_index]);
    freeReplyObject(reply);       
    reply = redisCommand(context, "SET stream/%i:%i:hit_occupancy:board:%i:fem:%i %f", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _fem_hit_occupancy[stream_index].Take(fem));
    freeReplyObject(reply);       
    reply = redisCommand(context, "EXPIRE stream/%i:%i:hit_occupancy:board:%i:fem:%i %i", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _stream_expire[stream_index]);
    freeReplyObject(reply);       
  }
}

void Redis::Snapshot(vector<ChannelData> *per_channel_data) {
  void *reply;
   
  // record the time for reference
  reply = redisCommand(context, "SET snapshot:time %i", _now);
  freeReplyObject(reply);

  // stuff per channel
  for (auto &channel: *per_channel_data) { 
    // store the waveform and fft's
    // also delete old lists
    reply = redisCommand(context, "DEL snapshot:waveform:wire:%i", channel.channel_no);
    freeReplyObject(reply);
    for (auto dat: channel.waveform) {
      reply = redisCommand(context, "RPUSH snapshot:waveform:wire:%i %f", channel.channel_no, dat);
      freeReplyObject(reply);
    }

    reply = redisCommand(context, "DEL snapshot:fft:wire:%i", channel.channel_no);
    freeReplyObject(reply);
    for (size_t i = 0; i < channel.fft_real.size(); i++) {
      double dat = channel.fft_real[i]*channel.fft_real[i] + channel.fft_imag[i]*channel.fft_imag[i];
      reply = redisCommand(context, "RPUSH snapshot:fft:wire:%i %f", channel.channel_no, dat);
      freeReplyObject(reply);
    }
  }

  // also store the noise correlation matrix if taking a snapshot
  // re-build the noise objects
  std::vector<daqAnalysis::NoiseSample> noise;
  for (size_t i = 0; i < per_channel_data->size(); i++) {
    noise.emplace_back((*per_channel_data)[i].noise_ranges, (*per_channel_data)[i].baseline);
  }
  reply = redisCommand(context, "DEL snapshot:correlation");
  freeReplyObject(reply);
  
  // Only calculate the upper-right half of the matrix since it is symmetric
  
  // The index 'k' into the list of the i-th sample with the j-th sample (where i <= j) is:
  // k = ((n+1)*n/2) - (n-i+1)*(n-i)/2 + j - i
  for (size_t i = 0; i < noise.size(); i++) {
    for (size_t j = i; j < noise.size(); j++) {
      double correlation = noise[i].Correlation((*per_channel_data)[i].waveform, noise[j], (*per_channel_data)[j].waveform);
      reply = redisCommand(context, "RPUSH snapshot:correlation %f",  correlation);
      freeReplyObject(reply);
    }
  }
}

void Redis::SendChannelData(vector<ChannelData> *per_channel_data) {
  // loop over all channels and set streams for important information
  for (auto &channel: *per_channel_data) {
    // loop over streams
    for (size_t i = 0; i < _stream_take.size(); i++) {
      _channel_rms[i].Add(channel.channel_no, channel.rms);
      _channel_baseline[i].Add(channel.channel_no, (double)channel.baseline);
      _channel_hit_occupancy[i].Add(channel.channel_no, (double)channel.peaks.size());
    }
  }

  // also store averages over fem, board data
  for (unsigned board = 0; board < daqAnalysis::ChannelMap::n_boards; board++) {
    for (unsigned fem = 0; fem < daqAnalysis::ChannelMap::n_fem_per_board; fem++) {
      // Calculate average of metrics and store them
      double rms = 0;
      double baseline = 0;
      double hit_occupancy = 0;
      for (unsigned channel = 0; channel < daqAnalysis::ChannelMap::n_channel_per_fem; channel++) {
	daqAnalysis::ChannelMap::board_channel board_channel {board, fem, channel};
	auto wire = daqAnalysis::ChannelMap::Channel2Wire(board_channel);
	rms += (*per_channel_data)[wire].rms;
	baseline += (*per_channel_data)[wire].baseline;
	hit_occupancy += (*per_channel_data)[wire].peaks.size();
      }
      rms = rms / daqAnalysis::ChannelMap::n_channel_per_fem;
      baseline = baseline / daqAnalysis::ChannelMap::n_channel_per_fem;
      hit_occupancy = hit_occupancy / daqAnalysis::ChannelMap::n_channel_per_fem;

      // index into the fem data cache
      unsigned fem_ind = board * ChannelMap::n_fem_per_board + fem;

      // update all the streams
      for (size_t i = 0; i < _stream_take.size(); i++) {
        _fem_rms[i].Add(fem_ind, rms);
        _fem_baseline[i].Add(fem_ind, baseline);
        _fem_hit_occupancy[i].Add(fem_ind, hit_occupancy);
      }
    }
  }

  for (size_t i = 0; i < _stream_take.size(); i++) {
    // send stuff (maybe)
    if ( (_now - _start) % _stream_take[i] == 0) {
        SendChannel(i);
        SendFem(i);
      // clear data caches
      _channel_rms[i].Clear();
      _channel_baseline[i].Clear();
      _channel_hit_occupancy[i].Clear();
      _fem_rms[i].Clear();
      _fem_baseline[i].Clear();
      _fem_hit_occupancy[i].Clear();
    }
    else {
      // increment data caches
      _channel_rms[i].Incl();
      _channel_baseline[i].Incl();
      _channel_hit_occupancy[i].Incl();
      _fem_rms[i].Incl();
      _fem_baseline[i].Incl();
      _fem_hit_occupancy[i].Incl();
    }
  }

  // snapshots
  bool take_snapshot = _snapshot_time != 0 && (_now - _start) % _snapshot_time == 0;
  if (take_snapshot) {
    Snapshot(per_channel_data);
  }
}

void daqAnalysis::StreamDataCache::Add(unsigned index, double dat) {
  _data[index] = (_n_values * _data[index] + dat) / (_n_values + 1);
}

void daqAnalysis::StreamDataCache::Incl() {
  _n_values ++;
}

void daqAnalysis::StreamDataCache::Clear() {
  _n_values = 0;
}

double daqAnalysis::StreamDataCache::Take(unsigned index) {
  double ret = _data[index];
  _data[index] = 0;
  return ret;
}


