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
  void SendChannelData(std::vector<daqAnalysis::ChannelData> *per_channel_data, std::vector<daqAnalysis::NoiseSample> *noise_samples);
  void SendHeaderData(std::vector<daqAnalysis::HeaderData> *header_data);
  void StartSend();
  void FinishSend();

protected:
  void SendChannel(unsigned stream_index);
  void SendFem(unsigned stream_index);
  void SendBoard(unsigned stream_index);
  void SendHeader(unsigned stream_index);
  void Snapshot(std::vector<ChannelData> *per_channel_data, std::vector<daqAnalysis::NoiseSample> *noise);
  void FinishPipeline(size_t n_commands);

  redisContext *context;
  std::time_t _now;
  std::time_t _start;
  int _snapshot_time;
  std::vector<unsigned> _stream_take;
  std::vector<unsigned> _stream_expire;
  std::vector<std::time_t> _stream_last;
  std::vector<bool> _stream_send;
  std::time_t _last_snapshot;

  std::vector<daqAnalysis::StreamDataMean> _channel_rms;
  std::vector<daqAnalysis::StreamDataMean> _channel_baseline;
  std::vector<daqAnalysis::StreamDataMean> _channel_hit_occupancy;
  std::vector<daqAnalysis::StreamDataMean> _channel_pulse_height;

  std::vector<daqAnalysis::StreamDataMean> _fem_rms;
  std::vector<daqAnalysis::StreamDataMean> _fem_scaled_sum_rms;
  std::vector<daqAnalysis::StreamDataMean> _fem_baseline;
  std::vector<daqAnalysis::StreamDataMean> _fem_hit_occupancy;
  std::vector<daqAnalysis::StreamDataMean> _fem_pulse_height;

  std::vector<daqAnalysis::StreamDataMean> _board_rms;
  std::vector<daqAnalysis::StreamDataMean> _board_baseline;
  std::vector<daqAnalysis::StreamDataMean> _board_hit_occupancy;
  std::vector<daqAnalysis::StreamDataMean> _board_pulse_height;

  std::vector<daqAnalysis::StreamDataMax> _frame_no;
  std::vector<daqAnalysis::StreamDataMax> _trigframe_no;
  std::vector<daqAnalysis::StreamDataMax> _event_no;

  FFTManager _fft_manager;

  bool _do_timing;
  daqAnalysis::RedisTiming _timing;
};

class daqAnalysis::StreamDataMean {
public:
  StreamDataMean(unsigned n_data): _data(n_data, 0.), _n_values(0) {}

  void Add(unsigned index, float dat);
  void Incl();
  void Clear();
  float Take(unsigned index);
  float Peak(unsigned index) { return _data[index]; }
  unsigned Size() { return _data.size(); }

protected:
  std::vector<float> _data;
  unsigned _n_values;

};

class daqAnalysis::StreamDataMax {
public:
  StreamDataMax(unsigned n_data): _data(n_data, 0.) {}

  void Add(unsigned index, float dat);
  float Take(unsigned index);
  float Peak(unsigned index) { return _data[index]; }
  unsigned Size() { return _data.size(); }

protected:
  std::vector<float> _data;
};

#endif /* Redis_h */
