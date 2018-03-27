#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <ctime>

#include <hiredis/hiredis.h>

#include "Redis.hh"
#include "ChannelData.hh"
#include "HeaderData.hh"
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
  UpdateTime();
}

Redis::~Redis() {
  redisFree(context);
}

void Redis::UpdateTime() {
  _now = std::time(nullptr);
}

void Redis::SendHeaderData(vector<HeaderData> *header_data) {
  void *reply;
  for (auto &header: *header_data) {
    reply = redisCommand(context, "SET stream/1:%i:checksum:fem%i:slot%i %i", _now, header.fem_id, header.slot_id, header.checksum);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:checksum:fem%i:slot%i %i", _now, header.fem_id, header.slot_id, 600);
    freeReplyObject(reply);
    reply = redisCommand(context, "SET stream/1:%i:frameno:fem%i:slot%i %i", _now, header.fem_id, header.slot_id, header.frame_number);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:frameno:fem%i:slot%i %i", _now, header.fem_id, header.slot_id, 600);
    freeReplyObject(reply);
    reply = redisCommand(context, "SET stream/1:%i:eventno:fem%i:slot%i %i", _now, header.fem_id, header.slot_id, header.event_number);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:eventno:fem%i:slot%i %i", _now, header.fem_id, header.slot_id, 600);
    freeReplyObject(reply);
    reply = redisCommand(context, "SET stream/1:%i:trigframeno:fem%i:slot%i %i", _now, header.fem_id, header.slot_id, header.trig_frame_number);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:trigframeno:fem%i:slot%i %i", _now, header.fem_id, header.slot_id, 600);
    freeReplyObject(reply);
  }
}

void Redis::SendChannelData(vector<ChannelData> *per_channel_data) {
  // TODO: Report failures
  // loop over all channels and set streams for important information
  void *reply;
  for (auto &channel: *per_channel_data) {
    // Send in some stuff 
    reply = redisCommand(context, "SET stream/1:%i:baseline:%i %f", _now, channel.channel_no, channel.baseline);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:baseline:%i %i", _now, channel.channel_no, 600);
    freeReplyObject(reply);
    reply = redisCommand(context, "SET stream/1:%i:rms:%i %f", _now, channel.channel_no, channel.rms);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:rms:%i %i", _now, channel.channel_no, 600);
    freeReplyObject(reply);
    reply = redisCommand(context, "SET stream/1:%i:max:%i %f", _now, channel.channel_no, channel.max);
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:max:%i %i", _now, channel.channel_no, 600);
    freeReplyObject(reply);

    // TODO: Is this to slow on front end?
    // also push the channel data as a json blob
    reply = redisCommand(context, "Set stream/1:%i:channel_data:%i %s", _now, channel.channel_no, channel.Jsonify().c_str());
    freeReplyObject(reply);
    reply = redisCommand(context, "EXPIRE stream/1:%i:channel_data:%i %i", _now, channel.channel_no, 600);
    freeReplyObject(reply);
  }
}

