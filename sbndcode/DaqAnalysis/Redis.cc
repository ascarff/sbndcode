#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <ctime>

#include <hiredis/hiredis.h>

#include "Redis.hh"
#include "ChannelData.hh"
#include "HeaderData.hh"
#include "Noise.hh"
#include "Analysis.h"

using namespace daqAnalysis;
using namespace std;

Redis::Redis(std::vector<unsigned> &stream_take, std::vector<unsigned> &stream_expire, int snapshot_time): 
  _snapshot_time(snapshot_time),
  _stream_take(stream_take),
  _stream_expire(stream_expire)
{
  context = redisConnect("127.0.0.1", 6379);
  std::cout << "REDIS CONTEXT: " << context << std::endl;
  if (context != NULL && context->err) {
    std::cerr << "Redis error: " <<  context->errstr << std::endl;
    exit(1);
  }
  _start = 0;
}

Redis::~Redis() {
  redisFree(context);
}

void Redis::StartSend() {
  _now = std::time(nullptr);
  if (_start == 0) _start = _now;
}

void Redis::FinishSend() {
  _last = _now;
}

bool Redis::ReadyToSend() {
  return _now != _last;
}

void Redis::SendHeaderData(vector<HeaderData> *header_data) {
  // if there's already data for this time, don't send
  if (_now == _last) return;

  void *reply;
  for (auto &header: *header_data) {
    for (size_t i = 0; i < _stream_take.size(); i++) {
      if ( (_now - _start) % _stream_take[i] == 0) {
	reply = redisCommand(context, "SET stream/%i:%i:checksum:fem%i:slot%i %i", _stream_take[i], _now, header.fem_id, header.slot_id, header.checksum);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:checksum:fem%i:slot%i %i", _stream_take[i], _now, header.fem_id, header.slot_id, _stream_expire[i]);
	freeReplyObject(reply);
	reply = redisCommand(context, "SET stream/%i:%i:frameno:fem%i:slot%i %i", _stream_take[i], _now, header.fem_id, header.slot_id, header.frame_number);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:frameno:fem%i:slot%i %i", _stream_take[i], _now, header.fem_id, header.slot_id, _stream_expire[i]);
	freeReplyObject(reply);
	reply = redisCommand(context, "SET stream/%i:%i:eventno:fem%i:slot%i %i", _stream_take[i], _now, header.fem_id, header.slot_id, header.event_number);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:eventno:fem%i:slot%i %i", _stream_take[i], _now, header.fem_id, header.slot_id, _stream_expire[i]);
	freeReplyObject(reply);
	reply = redisCommand(context, "SET stream/%i:%i:trigframeno:fem%i:slot%i %i", _stream_take[i], _now, header.fem_id, header.slot_id, header.trig_frame_number);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:trigframeno:fem%i:slot%i %i", _stream_take[i], _now, header.fem_id, header.slot_id, _stream_expire[i]);
	freeReplyObject(reply);
      }
    }
  }
}

void Redis::SendChannelData(vector<ChannelData> *per_channel_data) {
  // if there's already data for this time, don't send
  if (_now == _last) return;

  bool take_snapshot = _snapshot_time != 0 && (_now - _start) % _snapshot_time == 0;
  // TODO: Report failures
  void *reply;
 
  // if you're taking a snapshot, also record the time and event
  if (take_snapshot) {
    reply = redisCommand(context, "SET snapshot:time %i", _now);
    freeReplyObject(reply);
  }

  // loop over all channels and set streams for important information
  for (auto &channel: *per_channel_data) {
    // loop over streams
    for (size_t i = 0; i < _stream_take.size(); i++) {
      if ( (_now - _start) % _stream_take[i] == 0) {
	// Send in some stuff 
	reply = redisCommand(context, "SET stream/%i:%i:baseline:wire:%i %f", _stream_take[i], _now, channel.channel_no, channel.baseline);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:baseline:wire:%i %i", _stream_take[i], _now, channel.channel_no, _stream_expire[i]);
	freeReplyObject(reply);
	reply = redisCommand(context, "SET stream/%i:%i:rms:wire:%i %f", _stream_take[i], _now, channel.channel_no, channel.rms);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:rms:wire:%i %i", _stream_take[i], _now, channel.channel_no, _stream_expire[i]);
	freeReplyObject(reply);
	reply = redisCommand(context, "SET stream/%i:%i:max:wire:%i %f", _stream_take[i], _now, channel.channel_no, channel.max);
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:max:wire:%i %i", _stream_take[i], _now, channel.channel_no, _stream_expire[i]);
	freeReplyObject(reply);
	
	// TODO: Is this too slow on front end?
	// also push the channel data as a json blob
	reply = redisCommand(context, "SET stream/%i:%i:channel_data:wire:%i %s", _stream_take[i], _now, channel.channel_no, channel.Jsonify().c_str());
	freeReplyObject(reply);
	reply = redisCommand(context, "EXPIRE stream/%i:%i:channel_data:wire:%i %i", _stream_take[i], _now, channel.channel_no, _stream_expire[i]);
	freeReplyObject(reply);
      }
    }

    // snapshots
    if (take_snapshot) {
      // store the waveform and fft's
      // also delete old lists
      reply = redisCommand(context, "DEL snapshot:waveform:wire:%i", channel.channel_no);
      freeReplyObject(reply);
      for (auto dat: channel.waveform) {
        reply = redisCommand(context, "RPUSH snapshot:waveform:wire:%i %f", channel.channel_no, dat);
        freeReplyObject(reply);
      }
      // norm of fft
      reply = redisCommand(context, "DEL snapshot:fft:wire:%i", channel.channel_no);
      freeReplyObject(reply);
      for (size_t i = 0; i < channel.fft_real.size(); i++) {
        double dat = channel.fft_real[i]*channel.fft_real[i] + channel.fft_imag[i]*channel.fft_imag[i];
        reply = redisCommand(context, "RPUSH snapshot:fft:wire:%i %f", channel.channel_no, dat);
        freeReplyObject(reply);
      }
    }
  }
  // also store the noise correlation matrix if taking a snapshot
  if (take_snapshot) {
    // re-build the noise objects
    std::vector<daqAnalysis::NoiseSample> noise;
    for (size_t i = 0; i < per_channel_data->size(); i++) {
      noise.emplace_back((*per_channel_data)[i].noise_ranges, (*per_channel_data)[i].baseline);
    }
    reply = redisCommand(context, "DEL snapshot:correlation");
    freeReplyObject(reply);

    // Only calculate the upper-right half of the matrix since it is symmetric

    // The index 'k' into the list of the i-th sample with the j-th sample (where i <= j) is:
    // k = ((n+1)*n/2) - (n-i+1)*(n-i)/2 + j - i
    for (size_t i = 0; i < noise.size(); i++) {
      for (size_t j = i; j < noise.size(); j++) {
        double correlation = noise[i].Correlation((*per_channel_data)[i].waveform, noise[j], (*per_channel_data)[j].waveform);
        reply = redisCommand(context, "RPUSH snapshot:correlation %f",  correlation);
        freeReplyObject(reply);
      }
    }
  }
}

