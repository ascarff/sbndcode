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
  Redis();
  ~Redis();
  void SendChannelData(std::vector<daqAnalysis::ChannelData> *per_channel_data);
  void SendHeaderData(std::vector<daqAnalysis::HeaderData> *header_data);
  void UpdateTime();

protected:
  redisContext *context;
  std::time_t _now;

};
#endif /* Redis_h */
