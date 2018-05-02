#ifndef Redis_h
#define Redis_h

#include <vector>
#include <ctime>
#include <numeric>
#include <chrono>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "../ChannelData.hh"
#include "../HeaderData.hh"
#include "../Noise.hh"
#include "../FFT.hh"

#include "RedisData.hh"

namespace daqAnalysis {
  class Redis;
  class RedisTiming;

}
// keep track of timing information
class daqAnalysis::RedisTiming {
public:
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  float copy_data;
  float send_metrics;
  float send_header_data;
  float send_waveform;
  float send_fft;
  float correlation;
  float clear_pipeline;
  float fem_waveforms;

  RedisTiming():
    copy_data(0),
    send_metrics(0),
    send_header_data(0),
    send_waveform(0),
    send_fft(0),
    correlation(0),
    clear_pipeline(0),
    fem_waveforms(0)
  {}
  
  void StartTime();
  void EndTime(float *field);

  void Print();
};

class daqAnalysis::Redis {
public:
  Redis(const char *hostname, std::vector<unsigned> &stream_take, std::vector<unsigned> &stream_expire, int snapshot_time = -1, int static_waveform_size = -1, bool timing=false);
  ~Redis();
  // send info associated w/ ChannelData
  void SendChannelData(std::vector<daqAnalysis::ChannelData> *per_channel_data, std::vector<daqAnalysis::NoiseSample> *noise_samples, 
      std::vector<std::vector<short>> *fem_summed_waveforms);
  // send info associated w/ HeaderData
  void SendHeaderData(std::vector<daqAnalysis::HeaderData> *header_data);
  // must be called before calling Send functions
  void StartSend();
  // must be called after calling Send functions
  void FinishSend();

protected:
  // per-header (each associated w/ an fem) data to Redis
  void SendHeader(unsigned stream_index);
  // snapshot stuff
  void Snapshot(std::vector<ChannelData> *per_channel_data, std::vector<daqAnalysis::NoiseSample> *noise, std::vector<std::vector<short>> *fem_summed_waveforms);
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
  std::vector<daqAnalysis::RedisRMS> _rms;
  std::vector<daqAnalysis::RedisBaseline> _baseline;
  std::vector<daqAnalysis::RedisDNoise> _dnoise;
  std::vector<daqAnalysis::RedisPulseHeight> _pulse_height;
  std::vector<daqAnalysis::RedisOccupancy> _occupancy;

  // header info
  std::vector<daqAnalysis::StreamDataMax> _frame_no;
  std::vector<daqAnalysis::StreamDataMax> _trigframe_no;
  std::vector<daqAnalysis::StreamDataMax> _event_no;

  // FFT's for snapshots
  FFTManager _fft_manager;

  bool _do_timing;
  daqAnalysis::RedisTiming _timing;
};

#endif /* Redis_h */
