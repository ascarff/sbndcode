#ifndef Redis_h
#define Redis_h

#include <vector>
#include <ctime>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "ChannelData.hh"
#include "HeaderData.hh"
#include "Noise.hh"
#include "FFT.hh"

namespace daqAnalysis {
  class Redis;
  class RedisTiming;
  class StreamDataMean;
  class StreamDataVariableMean;
  class StreamDataMax;
}

// keep track of timing information
class daqAnalysis::RedisTiming {
public:
  clock_t start;
  float copy_channel_data;
  float board_and_fem_data;
  float send_channel_data;
  float send_fem_data;
  float send_board_data;
  float send_header_data;
  float send_waveform;
  float send_fft;
  float correlation;
  float clear_pipeline;

  RedisTiming():
    copy_channel_data(0),
    board_and_fem_data(0),
    send_channel_data(0),
    send_fem_data(0),
    send_board_data(0),
    send_header_data(0),
    send_waveform(0),
    send_fft(0),
    correlation(0),
    clear_pipeline(0)
  {}
  
  void StartTime();
  void EndTime(float *field);

  void Print();
};

class daqAnalysis::Redis {
public:
  Redis(std::vector<unsigned> &stream_take, std::vector<unsigned> &stream_expire, int snapshot_time = -1, int static_waveform_size = -1, bool timing=false);
  ~Redis();
  // send info associated w/ ChannelData
  void SendChannelData(std::vector<daqAnalysis::ChannelData> *per_channel_data, std::vector<daqAnalysis::NoiseSample> *noise_samples);
  // send info associated w/ HeaderData
  void SendHeaderData(std::vector<daqAnalysis::HeaderData> *header_data);
  // must be called before calling Send functions
  void StartSend();
  // must be called after calling Send functions
  void FinishSend();

protected:
  // actually send per-channel data to Redis
  void SendChannel(unsigned stream_index);
  // per-fem data to Redis
  void SendFem(unsigned stream_index);
  // per-board data to Redis
  void SendBoard(unsigned stream_index);
  // per-header (each associated w/ an fem) data to Redis
  void SendHeader(unsigned stream_index);
  // snapshot stuff
  void Snapshot(std::vector<ChannelData> *per_channel_data, std::vector<daqAnalysis::NoiseSample> *noise);
  // clear out a pipeline of n_commands commands
  void FinishPipeline(size_t n_commands);

  redisContext *context;
  std::time_t _now;
  std::time_t _start;
  int _snapshot_time;
  // vector of time deltas for each stream
  // the i-th stream with _stram_take[i] = dt will send info to redis every dt seconds
  std::vector<unsigned> _stream_take;
  // vector of the storage time for each stream
  // the i-th stream with _stream_expire[i] = dt will keep data in redis for dt seconds
  std::vector<unsigned> _stream_expire;
  // last time each stream sent something to redis
  std::vector<std::time_t> _stream_last;
  // whether, this time around, the i-th stream will send to redis. Calculated in StartSend()
  std::vector<bool> _stream_send;
  // last time a snapshot was sent
  std::time_t _last_snapshot;

  // running averates of Redis metrics per stream
  std::vector<daqAnalysis::StreamDataMean> _channel_rms;
  std::vector<daqAnalysis::StreamDataMean> _channel_baseline;
  std::vector<daqAnalysis::StreamDataMean> _channel_hit_occupancy;
  std::vector<daqAnalysis::StreamDataVariableMean> _channel_pulse_height;

  std::vector<daqAnalysis::StreamDataMean> _fem_rms;
  std::vector<daqAnalysis::StreamDataMean> _fem_scaled_sum_rms;
  std::vector<daqAnalysis::StreamDataMean> _fem_baseline;
  std::vector<daqAnalysis::StreamDataMean> _fem_hit_occupancy;
  std::vector<daqAnalysis::StreamDataVariableMean> _fem_pulse_height;

  std::vector<daqAnalysis::StreamDataMean> _board_rms;
  std::vector<daqAnalysis::StreamDataMean> _board_baseline;
  std::vector<daqAnalysis::StreamDataMean> _board_hit_occupancy;
  std::vector<daqAnalysis::StreamDataVariableMean> _board_pulse_height;

  std::vector<daqAnalysis::StreamDataMax> _frame_no;
  std::vector<daqAnalysis::StreamDataMax> _trigframe_no;
  std::vector<daqAnalysis::StreamDataMax> _event_no;

  // FFT's for snapshots
  FFTManager _fft_manager;

  bool _do_timing;
  daqAnalysis::RedisTiming _timing;
};

// keeps a running mean of a metric w/ n_data instances
class daqAnalysis::StreamDataMean {
public:
  StreamDataMean(unsigned n_data): _data(n_data, 0.), _n_values(0) {}

  // add in a new value
  void Add(unsigned index, float dat);
  // incl the number of values
  void Incl();
  // clear the number of values
  void Clear();
  // take the data value and reset it
  float Take(unsigned index);
  // just take a peek at the data value
  float Peak(unsigned index) { return _data[index]; }
  // returns n_data
  unsigned Size() { return _data.size(); }

protected:
  // internal data
  std::vector<float> _data;
  // number of values averaged together in each data point
  unsigned _n_values;

};

// keeps a running mean of a metric w/ n_data instances where each metric may have a different
// number of entries
class daqAnalysis::StreamDataVariableMean {
public:
  StreamDataVariableMean(unsigned n_data): _data(n_data, 0.), _n_values(n_data, 0) {}

  // add in a new value
  void Add(unsigned index, float dat);
  // take the data value and reset it
  float Take(unsigned index);
  // just take a peek at the data value
  float Peak(unsigned index) { return _data[index]; }
  // returns n_data
  unsigned Size() { return _data.size(); }

protected:
  // internal data
  std::vector<float> _data;
  // number of values averaged together in each data point
  std::vector<unsigned> _n_values;
};

// keeps running max value of a metric w/ n instances
class daqAnalysis::StreamDataMax {
public:
  StreamDataMax(unsigned n_data): _data(n_data, 0.) {}

  void Add(unsigned index, float dat);
  float Take(unsigned index);
  float Peek(unsigned index) { return _data[index]; }
  unsigned Size() { return _data.size(); }

protected:
  std::vector<float> _data;
};

#endif /* Redis_h */
