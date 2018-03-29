#ifndef Redis_h
#define Redis_h

#include <vector>
#include <ctime>

#include <hiredis/hiredis.h>

#include "ChannelData.hh"
#include "HeaderData.hh"

namespace daqAnalysis {
  class Redis;
}

class daqAnalysis::Redis {
public:
  Redis(std::vector<unsigned> &stream_take, std::vector<unsigned> &stream_expire, int snapshot_time = -1);
  ~Redis();
  void SendChannelData(std::vector<daqAnalysis::ChannelData> *per_channel_data);
  void SendHeaderData(std::vector<daqAnalysis::HeaderData> *header_data);
  void StartSend();
  void FinishSend();
  bool ReadyToSend();

protected:
  redisContext *context;
  std::time_t _now;
  std::time_t _last;
  std::time_t _start;
  int _snapshot_time;
  std::vector<unsigned> _stream_take;
  std::vector<unsigned> _stream_expire;
};
#endif /* Redis_h */
