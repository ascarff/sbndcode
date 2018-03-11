#include <stdlib.h>
#include <cassert>
#include <ctime>

#include <hiredis/hiredis.h>

#include "Redis.hh"
#include "Analysis.h"

using namespace daqAnalysis;
using namespace std;

Redis::Redis() {
  context = redisConnect("127.0.0.1", 6379);
  std::cout << "REDIS CONTEXT: " << context << std::endl;
  if (context != NULL && context->err) {
    std::cerr << "Redis error: " <<  context->errstr << std::endl;
    exit(1);
  }
}

Redis::~Redis() {
  redisFree(context);
}

void Redis::Send(Redis::EventDef &event) {
  // TODO: Report failures
  // loop over all channels and set streams for important information
  std::cout << "PER CHANNEL DATA: " << event.per_channel_data << std::endl;
  std::time_t now = std::time(nullptr);
  for (auto &channel: *event.per_channel_data) {
    redisCommand(context, "SET stream/1:%i:baseline:%i %f", now, channel.channel_no, channel.baseline);
    redisCommand(context, "EXPIRE stream/1:%i:baseline:%i %i", now, channel.channel_no, 600);
    redisCommand(context, "SET stream/1:%i:rms:%i %f", now, channel.channel_no, channel.rms);
    redisCommand(context, "EXPIRE stream/1:%i:rms:%i %i", now, channel.channel_no, 600);
    redisCommand(context, "SET stream/1:%i:max:%i %f", now, channel.channel_no, channel.max);
    redisCommand(context, "EXPIRE stream/1:%i:max:%i %i", now, channel.channel_no, 600);
  }
}

