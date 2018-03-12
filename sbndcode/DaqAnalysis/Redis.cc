#include <stdlib.h>
#include <stdio.h>
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
  std::time_t now = std::time(nullptr);
  void *reply;
  for (auto &channel: *event.per_channel_data) {
    // TODO: just send json blob

    // Send in some stuff 
    reply = redisCommand(context, "SET stream/1:%i:baseline:%i %f", now, channel.channel_no, channel.baseline);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:baseline:%i %i", now, channel.channel_no, 600);
    freeReplyObject(reply);
    reply = redisCommand(context, "SET stream/1:%i:rms:%i %f", now, channel.channel_no, channel.rms);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:rms:%i %i", now, channel.channel_no, 600);
    freeReplyObject(reply);
    reply = redisCommand(context, "SET stream/1:%i:max:%i %f", now, channel.channel_no, channel.max);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:max:%i %i", now, channel.channel_no, 600);
    freeReplyObject(reply);

    // add in noise sample to do calculations on front end
    for (auto &dat: channel.noise_sample) {
      reply = redisCommand(context, "RPUSH stream/1:%i:noise_sample:%i %f", now, channel.channel_no, dat);
      freeReplyObject(reply);
    }
    reply = redisCommand(context, "EXPIRE stream/1:%i:noise_sample:%i %i", now, channel.channel_no, 600);
    freeReplyObject(reply);
    
    // also push the channel data as a json blob
    reply = redisCommand(context, "Set stream/1:%i:channel_data:%i %s", now, channel.channel_no, channel.Jsonify().c_str());
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:channel_data:%i %i", now, channel.channel_no, 600);
    freeReplyObject(reply);
  }
}

