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

  _channel_rms(stream_take.size(), StreamDataMean(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _channel_baseline(stream_take.size(), StreamDataMean(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _channel_hit_occupancy(stream_take.size(), StreamDataMean(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),

  _fem_rms(stream_take.size(), StreamDataMean(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fem_baseline(stream_take.size(), StreamDataMean(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fem_hit_occupancy(stream_take.size(), StreamDataMean(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),

  _board_rms(stream_take.size(), StreamDataMean(ChannelMap::n_boards)),
  _board_baseline(stream_take.size(), StreamDataMean(ChannelMap::n_boards)),
  _board_hit_occupancy(stream_take.size(), StreamDataMean(ChannelMap::n_boards)),

  _frame_no(stream_take.size(), StreamDataMax(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _trigframe_no(stream_take.size(), StreamDataMax(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _event_no(stream_take.size(), StreamDataMax(ChannelMap::n_fem_per_board* ChannelMap::n_boards))
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
  for (auto &header: *header_data) {
    for (size_t i = 0; i < _stream_take.size(); i++) {
      // TODO: Change
      // index into the fem data cache
      //unsigned fem_ind = header.slot_id * ChannelMap::n_fem_per_board + header.fem_id;
      unsigned fem_ind = 0;
      _event_no[i].Add(fem_ind, header.event_number);
      _frame_no[i].Add(fem_ind, header.frame_number);
      _trigframe_no[i].Add(fem_ind, header.trig_frame_number);
    }
  }
  for (size_t i = 0; i < _stream_take.size(); i++) {
    if ( (_now - _start) % _stream_take[i] == 0) {
      SendHeader(i);
    }
  }
}

void Redis::SendHeader(unsigned stream_index) {
  void *reply;
  for (size_t fem_ind = 0; fem_ind < ChannelMap::n_fem_per_board* ChannelMap::n_boards; fem_ind++) {
    unsigned fem = fem_ind % ChannelMap::n_fem_per_board;
    unsigned board = fem_ind / ChannelMap::n_fem_per_board;

    reply = redisCommand(context, "SET stream/%i:%i:frameno:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _frame_no[stream_index].Take(fem_ind));
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/%i:%i:frameno:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _stream_expire[stream_index]);
    freeReplyObject(reply);

    reply = redisCommand(context, "SET stream/%i:%i:eventno:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _event_no[stream_index].Take(fem_ind));
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/%i:%i:eventno:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _stream_expire[stream_index]);
    freeReplyObject(reply);

    reply = redisCommand(context, "SET stream/%i:%i:trigframeno:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _trigframe_no[stream_index].Take(fem_ind));
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/%i:%i:trigframeno:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _stream_expire[stream_index]);
    freeReplyObject(reply);
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
  for (unsigned fem_ind = 0; fem_ind < _fem_rms[stream_index].Size(); fem_ind++) {
    unsigned fem = fem_ind % ChannelMap::n_fem_per_board;
    unsigned board = ChannelMap::Board(fem_ind);
    reply = redisCommand(context, "SET stream/%i:%i:rms:board:%i:fem:%i %f", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _fem_rms[stream_index].Take(fem_ind));
    freeReplyObject(reply);       
    reply = redisCommand(context, "EXPIRE stream/%i:%i:rms:board:%i:fem:%i %i", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _stream_expire[stream_index]);
    freeReplyObject(reply);       
    reply = redisCommand(context, "SET stream/%i:%i:baseline:board:%i:fem:%i %f", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _fem_baseline[stream_index].Take(fem_ind));
    freeReplyObject(reply);       
    reply = redisCommand(context, "EXPIRE stream/%i:%i:baseline:board:%i:fem:%i %i", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _stream_expire[stream_index]);
    freeReplyObject(reply);       
    reply = redisCommand(context, "SET stream/%i:%i:hit_occupancy:board:%i:fem:%i %f", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _fem_hit_occupancy[stream_index].Take(fem_ind));
    freeReplyObject(reply);       
    reply = redisCommand(context, "EXPIRE stream/%i:%i:hit_occupancy:board:%i:fem:%i %i", _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _stream_expire[stream_index]);
    freeReplyObject(reply);       
  }
}

void Redis::SendBoard(unsigned stream_index) {
  // TODO: Report failures
  void *reply;
  for (unsigned board = 0; board < _board_rms[stream_index].Size(); board++) {
    reply = redisCommand(context, "SET stream/%i:%i:rms:board:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, _board_rms[stream_index].Take(board));
    freeReplyObject(reply);       
    reply = redisCommand(context, "EXPIRE stream/%i:%i:rms:board:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, _stream_expire[stream_index]);
    freeReplyObject(reply);       

    reply = redisCommand(context, "SET stream/%i:%i:baseline:board:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, _board_baseline[stream_index].Take(board));
    freeReplyObject(reply);       
    reply = redisCommand(context, "EXPIRE stream/%i:%i:baseline:board:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, _stream_expire[stream_index]);
    freeReplyObject(reply);       

    reply = redisCommand(context, "SET stream/%i:%i:hit_occupancy:board:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, _board_hit_occupancy[stream_index].Take(board));
    freeReplyObject(reply);       
    reply = redisCommand(context, "EXPIRE stream/%i:%i:hit_occupancy:board:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, _stream_expire[stream_index]);
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
    double board_rms = 0;
    double board_baseline = 0;
    double board_hit_occupancy = 0;

    double rms = 0;
    double baseline = 0;
    double hit_occupancy = 0;
    // fem average
    for (unsigned fem = 0; fem < daqAnalysis::ChannelMap::n_fem_per_board; fem++) {
      // Calculate average of metrics and store them
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
      // board average
      board_rms += rms;
      board_baseline += baseline;
      board_hit_occupancy += hit_occupancy; 
    }

    board_rms = board_rms / daqAnalysis::ChannelMap::n_boards;
    board_baseline = board_baseline / daqAnalysis::ChannelMap::n_boards;
    board_hit_occupancy = board_hit_occupancy / daqAnalysis::ChannelMap::n_boards;
    // update board streams
    for (size_t i = 0; i < _stream_take.size(); i++) {
      _board_rms[i].Add(board, board_rms);
      _board_baseline[i].Add(board, board_baseline);
      _board_hit_occupancy[i].Add(board, board_hit_occupancy);
    }
  }

  for (size_t i = 0; i < _stream_take.size(); i++) {
    // send stuff (maybe)
    if ( (_now - _start) % _stream_take[i] == 0) {
      SendChannel(i);
      SendFem(i);
      SendBoard(i);
      // clear data caches
      _channel_rms[i].Clear();
      _channel_baseline[i].Clear();
      _channel_hit_occupancy[i].Clear();
      _fem_rms[i].Clear();
      _fem_baseline[i].Clear();
      _fem_hit_occupancy[i].Clear();
      _board_rms[i].Clear();
      _board_baseline[i].Clear();
      _board_hit_occupancy[i].Clear();
    }
    else {
      // increment data caches
      _channel_rms[i].Incl();
      _channel_baseline[i].Incl();
      _channel_hit_occupancy[i].Incl();
      _fem_rms[i].Incl();
      _fem_baseline[i].Incl();
      _fem_hit_occupancy[i].Incl();
      _board_rms[i].Incl();
      _board_baseline[i].Incl();
      _board_hit_occupancy[i].Incl();
    }
  }

  // snapshots
  bool take_snapshot = _snapshot_time != 0 && (_now - _start) % _snapshot_time == 0;
  if (take_snapshot) {
    Snapshot(per_channel_data);
  }
}

void daqAnalysis::StreamDataMean::Add(unsigned index, double dat) {
  _data[index] = (_n_values * _data[index] + dat) / (_n_values + 1);
}

void daqAnalysis::StreamDataMean::Incl() {
  _n_values ++;
}

void daqAnalysis::StreamDataMean::Clear() {
  _n_values = 0;
}

double daqAnalysis::StreamDataMean::Take(unsigned index) {
  double ret = _data[index];
  _data[index] = 0;
  return ret;
}

void daqAnalysis::StreamDataMax::Add(unsigned index, double dat) {
  if (_data[index] < dat) _data[index] = dat;
}

double daqAnalysis::StreamDataMax::Take(unsigned index) {
  double ret = _data[index];
  _data[index] = 0;
  return ret;
}

