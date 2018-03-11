#ifndef Redis_h
#define Redis_h

#include <vector>

#include <hiredis/hiredis.h>

#include "ChannelData.hh"

namespace daqAnalysis {
  class Redis;
}

class daqAnalysis::Redis {
public:
  // The Event definition as seen by consumers of this analysis outout (i.e. Redis)
  struct EventDef {
    std::vector<daqAnalysis::ChannelData> *per_channel_data;
  };

  Redis();
  ~Redis();
  void Send(EventDef &event);

protected:
  redisContext *context;

};
#endif /* Redis_h */
