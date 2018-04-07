#ifndef Redis_h
#define Redis_h

#include <vector>
#include <ctime>

#include <hiredis/hiredis.h>

#include "ChannelData.hh"
#include "HeaderData.hh"
#include "Noise.hh"

namespace daqAnalysis {
  class Redis;
  class StreamDataMean;
  class StreamDataMax;
}

class daqAnalysis::Redis {
public:
  Redis(std::vector<unsigned> &stream_take, std::vector<unsigned> &stream_expire, int snapshot_time = -1);
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

  redisContext *context;
  std::time_t _now;
  std::time_t _last;
  std::time_t _start;
  int _snapshot_time;
  std::vector<unsigned> _stream_take;
  std::vector<unsigned> _stream_expire;

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
  
};

class daqAnalysis::StreamDataMean {
public:
  StreamDataMean(unsigned n_data): _data(n_data, 0.), _n_values(0) {}

  void Add(unsigned index, double dat);
  void Incl();
  void Clear();
  double Take(unsigned index);
  double Peak(unsigned index) { return _data[index]; }
  unsigned Size() { return _data.size(); }

protected:
  std::vector<double> _data;
  unsigned _n_values;

};

class daqAnalysis::StreamDataMax {
public:
  StreamDataMax(unsigned n_data): _data(n_data, 0.) {}

  void Add(unsigned index, double dat);
  double Take(unsigned index);
  double Peak(unsigned index) { return _data[index]; }
  unsigned Size() { return _data.size(); }

protected:
  std::vector<double> _data;
};

#endif /* Redis_h */
