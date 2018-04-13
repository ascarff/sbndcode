#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <ctime>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "Redis.hh"
#include "ChannelData.hh"
#include "HeaderData.hh"
#include "Noise.hh"
#include "Analysis.h"
#include "ChannelMap.hh"
#include "FFT.hh"

using namespace daqAnalysis;
using namespace std;

Redis::Redis(std::vector<unsigned> &stream_take, std::vector<unsigned> &stream_expire, int snapshot_time, int waveform_input_size, bool timing): 
  _snapshot_time(snapshot_time),
  _stream_take(stream_take),
  _stream_expire(stream_expire),
  _stream_last(stream_take.size(), 0),
  _stream_send(stream_take.size(), false),
  _last_snapshot(0),

  _channel_rms(stream_take.size(), StreamDataMean(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _channel_baseline(stream_take.size(), StreamDataMean(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _channel_hit_occupancy(stream_take.size(), StreamDataMean(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _channel_pulse_height(stream_take.size(), StreamDataVariableMean(ChannelMap::n_channel_per_fem * ChannelMap::n_fem_per_board* ChannelMap::n_boards)),

  _fem_rms(stream_take.size(), StreamDataMean(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fem_scaled_sum_rms(stream_take.size(), StreamDataMean(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fem_baseline(stream_take.size(), StreamDataMean(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fem_hit_occupancy(stream_take.size(), StreamDataMean(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fem_pulse_height(stream_take.size(), StreamDataVariableMean(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),

  _board_rms(stream_take.size(), StreamDataMean(ChannelMap::n_boards)),
  _board_baseline(stream_take.size(), StreamDataMean(ChannelMap::n_boards)),
  _board_hit_occupancy(stream_take.size(), StreamDataMean(ChannelMap::n_boards)),
  _board_pulse_height(stream_take.size(), StreamDataVariableMean(ChannelMap::n_boards)),

  _frame_no(stream_take.size(), StreamDataMax(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _trigframe_no(stream_take.size(), StreamDataMax(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _event_no(stream_take.size(), StreamDataMax(ChannelMap::n_fem_per_board* ChannelMap::n_boards)),
  _fft_manager((waveform_input_size > 0) ? waveform_input_size: 0),
  _do_timing(timing)
{
  //context = redisAsyncConnect("127.0.0.1", 6379);
  context = redisConnect("127.0.0.1", 6379);
  std::cout << "REDIS CONTEXT: " << context << std::endl;
  if (context != NULL && context->err) {
    std::cerr << "Redis error: " <<  context->errstr << std::endl;
    exit(1);
  }
  _start = 0;
}

Redis::~Redis() {
  //redisAsyncDisconnect(context);
  redisFree(context);
}

void Redis::StartSend() {
  _now = std::time(nullptr);
  for (size_t i = 0; i < _stream_send.size(); i++) {
    _stream_send[i] = _stream_last[i] != _now && (_now - _start) % _stream_take[i] == 0;
    if (_stream_send[i]) {
      _stream_last[i] = _now;
    }
  }
  // first time startup
  if (_start == 0) _start = _now;
}

void Redis::FinishSend() {
  if (_do_timing) {
    _timing.Print();
  }
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
    if (_stream_send[i]) {
      SendHeader(i);
    }
  }
}

void Redis::SendHeader(unsigned stream_index) {
  void *reply; 
  if (_do_timing) {
    _timing.StartTime();
  }
  for (size_t fem_ind = 0; fem_ind < ChannelMap::n_fem_per_board* ChannelMap::n_boards; fem_ind++) {
    unsigned fem = fem_ind % ChannelMap::n_fem_per_board;
    unsigned board = fem_ind / ChannelMap::n_fem_per_board;

    reply = redisCommand(context, "SET stream/%i:%i:frame_no:board:%i:fem:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _frame_no[stream_index].Take(fem_ind));
    freeReplyObject(reply);
    
    reply = redisCommand(context, "EXPIRE stream/%i:%i:frame_no:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _stream_expire[stream_index]);
    freeReplyObject(reply);
    

    reply = redisCommand(context, "SET stream/%i:%i:event_no:board:%i:fem:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _event_no[stream_index].Take(fem_ind));
    freeReplyObject(reply);
    
    reply = redisCommand(context, "EXPIRE stream/%i:%i:event_no:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _stream_expire[stream_index]);
    freeReplyObject(reply);
    

    reply = redisCommand(context, "SET stream/%i:%i:trigframe_no:board:%i:fem:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _trigframe_no[stream_index].Take(fem_ind));
    freeReplyObject(reply);
    
    reply = redisCommand(context, "EXPIRE stream/%i:%i:trigframe_no:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], fem, board, _stream_expire[stream_index]);
    freeReplyObject(reply);
    
    if (_do_timing) {
      _timing.EndTime(&_timing.send_header_data);
    }
  }
}

void Redis::SendChannel(unsigned stream_index) {
  void *reply;
  // send everything in channel streams
  if (_do_timing) {
    _timing.StartTime();
  }
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
    

    reply = redisCommand(context, "SET stream/%i:%i:pulse_height:wire:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], i, _channel_pulse_height[stream_index].Take(i));
    freeReplyObject(reply);
    
    reply = redisCommand(context, "EXPIRE stream/%i:%i:pulse_height:wire:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], i, _stream_expire[stream_index]);
    freeReplyObject(reply);
  } 
  if (_do_timing) {
    _timing.EndTime(&_timing.send_channel_data);
  }
}

void Redis::SendFem(unsigned stream_index) {
  // TODO: Report failures

  void *reply;
  if (_do_timing) {
    _timing.StartTime();
  }
  for (unsigned fem_ind = 0; fem_ind < _fem_rms[stream_index].Size(); fem_ind++) {
    unsigned fem = fem_ind % ChannelMap::n_fem_per_board;
    unsigned board = fem_ind / ChannelMap::n_fem_per_board;
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
           

    reply = redisCommand(context, "SET stream/%i:%i:scaled_sum_rms:board:%i:fem:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _fem_scaled_sum_rms[stream_index].Take(fem_ind));
    freeReplyObject(reply);
           
    reply = redisCommand(context, "EXPIRE stream/%i:%i:scaled_sum_rms:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _stream_expire[stream_index]);
    freeReplyObject(reply);
           

    reply = redisCommand(context, "SET stream/%i:%i:pulse_height:board:%i:fem:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _fem_pulse_height[stream_index].Take(fem_ind));
    freeReplyObject(reply);
           
    reply = redisCommand(context, "EXPIRE stream/%i:%i:pulse_height:board:%i:fem:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, fem, _stream_expire[stream_index]);
    freeReplyObject(reply);
           
  }
  if (_do_timing) {
    _timing.EndTime(&_timing.send_fem_data);
  }
}

void Redis::SendBoard(unsigned stream_index) {
  // TODO: Report failures
  
  void *reply;
  if (_do_timing) {
    _timing.StartTime();
  }
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
           

    reply = redisCommand(context, "SET stream/%i:%i:pulse_height:board:%i %f", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, _board_pulse_height[stream_index].Take(board));
    freeReplyObject(reply);
           
    reply = redisCommand(context, "EXPIRE stream/%i:%i:pulse_height:board:%i %i", 
      _stream_take[stream_index], _now/_stream_take[stream_index], board, _stream_expire[stream_index]);
    freeReplyObject(reply);
  }
  if (_do_timing) {
    _timing.EndTime(&_timing.send_board_data);
  }
}

inline void pushFFTDat(redisContext *context, unsigned channel, float re, float im) {
  float dat = re * im;
  redisAppendCommand(context, "RPUSH snapshot:fft:wire:%i %f", channel, dat);
}

void Redis::Snapshot(vector<ChannelData> *per_channel_data, vector<NoiseSample> *noise) {
  size_t n_commands = 0;

  // record the time for reference
  redisAppendCommand(context, "SET snapshot:time %i", _now);
  n_commands ++;

  // stuff per channel
  for (auto &channel: *per_channel_data) { 
    if (_do_timing) {
      _timing.StartTime();
    }
    // store the waveform and fft's
    // also delete old lists
    redisAppendCommand(context, "DEL snapshot:waveform:wire:%i", channel.channel_no);
    
    for (auto dat: channel.waveform) {
      redisAppendCommand(context, "RPUSH snapshot:waveform:wire:%i %i", channel.channel_no, dat);
    }
    n_commands += channel.waveform.size() + 1;

    if (_do_timing) {
      _timing.EndTime(&_timing.send_waveform);
    }

    if (_do_timing) {
      _timing.StartTime();
    }

    redisAppendCommand(context, "DEL snapshot:fft:wire:%i", channel.channel_no);
    // use already calculated FFT if there
    if (channel.fft_real.size() != 0) {
      for (size_t i = 0; i < channel.fft_real.size(); i++) {
        pushFFTDat(context, channel.channel_no, channel.fft_real[i], channel.fft_imag[i]);
      }
      n_commands += channel.fft_real.size() + 1;
    }
    // otherwise calculate it yourself
    else {
      // re-allocate if necessary
      if (_fft_manager.InputSize() != channel.waveform.size()) {
        _fft_manager.Set(channel.waveform.size());
      }
      for (size_t i = 0; i < channel.waveform.size(); i++) {
        double *input = _fft_manager.InputAt(i);
        *input = (double) channel.waveform[i];
      }
      _fft_manager.Execute();
      int adc_fft_size = _fft_manager.OutputSize();
      for (int i = 0; i < adc_fft_size; i++) {
        pushFFTDat(context, channel.channel_no, _fft_manager.ReOutputAt(i), _fft_manager.ImOutputAt(i));
      }
      n_commands += adc_fft_size + 1;
    }
    if (_do_timing) {
      _timing.EndTime(&_timing.send_fft);
    }
  }

  if (_do_timing) {
    _timing.StartTime();
  }
  // also store the noise correlation matrix if taking a snapshot
  redisAppendCommand(context, "DEL snapshot:correlation");
  
  // Only calculate the upper-right half of the matrix since it is symmetric
  // The index 'k' into the list of the i-th sample with the j-th sample (where i <= j) is:
  // k = ((n+1)*n/2) - (n-i+1)*(n-i)/2 + j - i
  for (size_t i = 0; i < noise->size(); i++) {
    for (size_t j = i; j < noise->size(); j++) {
      float correlation = (*noise)[i].Correlation((*per_channel_data)[i].waveform, (*noise)[j], (*per_channel_data)[j].waveform);
      redisAppendCommand(context, "RPUSH snapshot:correlation %f",  correlation);
    }
  }
  n_commands += 1 +(noise->size() + 1) * noise->size() / 2;
  if (_do_timing) {
    _timing.EndTime(&_timing.correlation);
  }

  if (_do_timing) {
    _timing.StartTime();
  }
  FinishPipeline(n_commands);
  if (_do_timing) {
    _timing.EndTime(&_timing.clear_pipeline);
  }
  
}

void Redis::SendChannelData(vector<ChannelData> *per_channel_data, vector<NoiseSample> *noise_samples) {
  if (_do_timing) {
    _timing.StartTime();
  }
  // loop over all channels and set streams for important information
  for (auto &channel: *per_channel_data) {
    // loop over streams
    for (size_t i = 0; i < _stream_take.size(); i++) {
      _channel_rms[i].Add(channel.channel_no, channel.rms);
      _channel_baseline[i].Add(channel.channel_no, (float)channel.baseline);
      _channel_hit_occupancy[i].Add(channel.channel_no, (float)channel.peaks.size());
      float pulse_height = channel.meanPeakHeight();
      if (pulse_height > 1e-4) {
          _channel_pulse_height[i].Add(channel.channel_no, pulse_height);
      }
    }
  }
  if (_do_timing) {
    _timing.EndTime(&_timing.copy_channel_data);
  }

  if (_do_timing) {
    _timing.StartTime();
  }
  // also store averages over fem, board data
  for (unsigned board = 0; board < daqAnalysis::ChannelMap::n_boards; board++) {
    float board_rms = 0;
    float board_baseline = 0;
    float board_hit_occupancy = 0;
    float board_pulse_height = 0;
    unsigned n_board_pulse_height = 0;

    float rms = 0;
    float baseline = 0;
    float hit_occupancy = 0;
    float pulse_height = 0;
    float ssum_rms;

    unsigned n_pulse_height = 0;
    // fem average
    for (unsigned fem = 0; fem < daqAnalysis::ChannelMap::n_fem_per_board; fem++) {
      std::vector<std::vector<int16_t> *> waveforms {};
      std::vector<daqAnalysis::NoiseSample *> this_noise_samples {};

      // Calculate average of metrics and store them
      for (unsigned channel = 0; channel < daqAnalysis::ChannelMap::n_channel_per_fem; channel++) {
	daqAnalysis::ChannelMap::board_channel board_channel {board, fem, channel};
	auto wire = daqAnalysis::ChannelMap::Channel2Wire(board_channel);

	rms += (*per_channel_data)[wire].rms;
	baseline += (*per_channel_data)[wire].baseline;
	hit_occupancy += (*per_channel_data)[wire].Occupancy();
        float this_pulse_height = (*per_channel_data)[wire].meanPeakHeight();
        if (this_pulse_height > 1e-4) {
            pulse_height += this_pulse_height;
            n_pulse_height ++;
        }

        // collect waveforms for sum rms calculation
        waveforms.push_back(&(*per_channel_data)[wire].waveform);
        this_noise_samples.push_back(&(*noise_samples)[wire]);
      }
      rms = rms / daqAnalysis::ChannelMap::n_channel_per_fem;
      baseline = baseline / daqAnalysis::ChannelMap::n_channel_per_fem;
      hit_occupancy = hit_occupancy / daqAnalysis::ChannelMap::n_channel_per_fem;
      pulse_height = pulse_height / n_pulse_height;

      // sum rms!
      ssum_rms = NoiseSample::ScaledSumRMS(this_noise_samples, waveforms); 
      // and then clear out containers
      waveforms.clear();
      this_noise_samples.clear();

      // index into the fem data cache
      unsigned fem_ind = board * ChannelMap::n_fem_per_board + fem;

      // update all the streams
      for (size_t i = 0; i < _stream_take.size(); i++) {
        _fem_rms[i].Add(fem_ind, rms);
        _fem_baseline[i].Add(fem_ind, baseline);
        _fem_hit_occupancy[i].Add(fem_ind, hit_occupancy);
        _fem_scaled_sum_rms[i].Add(fem_ind, ssum_rms);
        if (pulse_height > 1e-4) {
          _fem_pulse_height[i].Add(fem_ind, pulse_height);
        }
      }
      // board average
      board_rms += rms;
      board_baseline += baseline;
      board_hit_occupancy += hit_occupancy; 
      if (pulse_height > 1e-4) {
          board_pulse_height += pulse_height;
          n_board_pulse_height ++;
      }   
    }

    board_rms = board_rms / daqAnalysis::ChannelMap::n_boards;
    board_baseline = board_baseline / daqAnalysis::ChannelMap::n_boards;
    board_hit_occupancy = board_hit_occupancy / daqAnalysis::ChannelMap::n_boards;
    board_pulse_height = board_pulse_height / n_board_pulse_height;

    // update board streams
    for (size_t i = 0; i < _stream_take.size(); i++) {
      _board_rms[i].Add(board, board_rms);
      _board_baseline[i].Add(board, board_baseline);
      _board_hit_occupancy[i].Add(board, board_hit_occupancy);
      if (board_pulse_height > 1e-4) {
        _board_pulse_height[i].Add(board, board_pulse_height);
      }
    }
  }
  if (_do_timing) {
    _timing.EndTime(&_timing.board_and_fem_data);
  }

  for (size_t i = 0; i < _stream_take.size(); i++) {
    // send stuff (maybe)
    if (_stream_send[i]) {
      // let redis use as much memory as it needs
      //context->c.reader->maxbuf = 0;
      SendChannel(i);
      SendFem(i);
      SendBoard(i);
      // turn back on memory limiting
      //context->c.reader->maxbuf = REDIS_READER_MAX_BUF;
      // clear data caches
      _channel_rms[i].Clear();
      _channel_baseline[i].Clear();
      _channel_hit_occupancy[i].Clear();
      _fem_rms[i].Clear();
      _fem_baseline[i].Clear();
      _fem_hit_occupancy[i].Clear();
      _fem_scaled_sum_rms[i].Clear();
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
      _fem_scaled_sum_rms[i].Incl();
      _board_rms[i].Incl();
      _board_baseline[i].Incl();
      _board_hit_occupancy[i].Incl();
    }
  }

  if (_do_timing) {
    _timing.StartTime();
  }
  // snapshots
  bool take_snapshot = _snapshot_time > 0 && (_now - _start) % _snapshot_time == 0 && _last_snapshot != _now;
  if (take_snapshot) {
    Snapshot(per_channel_data, noise_samples);
    _last_snapshot = _now;
  }
}

// clear out a sequence of Append Commands
void Redis::FinishPipeline(size_t n_commands) {
  // TODO: Error Handling
  void *reply;
  for (size_t i =0; i <n_commands; i++) {
    redisGetReply(context, &reply); 
    freeReplyObject(reply);
  }
}

void daqAnalysis::StreamDataMean::Add(unsigned index, float dat) {
  _data[index] = (_n_values * _data[index] + dat) / (_n_values + 1);
}

void daqAnalysis::StreamDataMean::Incl() {
  _n_values ++;
}

void daqAnalysis::StreamDataMean::Clear() {
  _n_values = 0;
}

float daqAnalysis::StreamDataMean::Take(unsigned index) {
  float ret = _data[index];
  _data[index] = 0;
  return ret;
}

void daqAnalysis::StreamDataVariableMean::Add(unsigned index, float dat) {
  _data[index] = (_n_values[index] * _data[index] + dat) / (_n_values[index] + 1);
  _n_values[index] ++;
}

float daqAnalysis::StreamDataVariableMean::Take(unsigned index) {
  float ret = _data[index];
  _data[index] = 0;
  _n_values[index] = 0;
  return ret;
}

void daqAnalysis::StreamDataMax::Add(unsigned index, float dat) {
  if (_data[index] < dat) _data[index] = dat;
}

float daqAnalysis::StreamDataMax::Take(unsigned index) {
  float ret = _data[index];
  _data[index] = 0;
  return ret;
}

void RedisTiming::StartTime() {
  start = clock();
}
void RedisTiming::EndTime(float *field) {
  *field += float(clock() - start)/CLOCKS_PER_SEC;
}
void RedisTiming::Print() {
  std::cout << "COPY CHANNEL: " << copy_channel_data << std::endl;
  std::cout << "BOARD + FEM : " << board_and_fem_data << std::endl;
  std::cout << "SEND CHANNEL: " << send_channel_data << std::endl;
  std::cout << "SEND FEM    : " << send_fem_data << std::endl;
  std::cout << "SEND BOARD  : " << send_board_data << std::endl;
  std::cout << "SEND HEADER : " << send_header_data << std::endl;
  std::cout << "SEND WAVEFORM " << send_waveform << std::endl;
  std::cout << "SEND FFT    : " << send_fft << std::endl;
  std::cout << "CORRELATION : " << correlation << std::endl;
  std::cout << "CLEAR PIPE  : " << clear_pipeline << std::endl;
}



