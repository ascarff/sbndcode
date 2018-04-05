#ifndef Redis_h
#define Redis_h

#include <vector>
#include <ctime>

#include <hiredis/hiredis.h>

#include "ChannelData.hh"
#include "HeaderData.hh"

namespace daqAnalysis {
  class Redis;
  class StreamDataCache;
}

class daqAnalysis::Redis {
public:
  Redis(std::vector<unsigned> &stream_take, std::vector<unsigned> &stream_expire, int snapshot_time = -1);
  ~Redis();
  void SendChannelData(std::vector<daqAnalysis::ChannelData> *per_channel_data);
  void SendHeaderData(std::vector<daqAnalysis::HeaderData> *header_data);
  void StartSend();
  void FinishSend();

protected:
  void SendChannel(unsigned stream_index);
  void SendFem(unsigned stream_index);
  void Snapshot(std::vector<ChannelData> *per_channel_data);

  redisContext *context;
  std::time_t _now;
  std::time_t _last;
  std::time_t _start;
  int _snapshot_time;
  std::vector<unsigned> _stream_take;
  std::vector<unsigned> _stream_expire;

  std::vector<daqAnalysis::StreamDataCache> _channel_rms;
  std::vector<daqAnalysis::StreamDataCache> _channel_baseline;
  std::vector<daqAnalysis::StreamDataCache> _channel_hit_occupancy;

  std::vector<daqAnalysis::StreamDataCache> _fem_rms;
  std::vector<daqAnalysis::StreamDataCache> _fem_baseline;
  std::vector<daqAnalysis::StreamDataCache> _fem_hit_occupancy;
};

class daqAnalysis::StreamDataCache {
public:
  StreamDataCache(unsigned n_data): _data(n_data, 0.), _n_values(0) {}

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

#endif /* Redis_h */
