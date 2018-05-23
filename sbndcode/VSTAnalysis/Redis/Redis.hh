#ifndef Redis_h
#define Redis_h

#include <vector>
#include <ctime>
#include <numeric>
#include <chrono>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "art/Framework/Principal/Handle.h"

#include "lardataobj/RawData/RawDigit.h"

#include "../ChannelData.hh"
#include "../HeaderData.hh"
#include "../Noise.hh"
#include "../FFT.hh"

#include "RedisData.hh"

namespace daqAnalysis {
  class Redis;
  class RedisTiming;

}
// keep track of timing information
class daqAnalysis::RedisTiming {
public:
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  float copy_data;
  float send_metrics;
  float send_header_data;
  float send_waveform;
  float send_fft;
  float correlation;
  float clear_pipeline;
  float fem_waveforms;

  RedisTiming():
    copy_data(0),
    send_metrics(0),
    send_header_data(0),
    send_waveform(0),
    send_fft(0),
    correlation(0),
    clear_pipeline(0),
    fem_waveforms(0)
  {}
  
  void StartTime();
  void EndTime(float *field);

  void Print();
};

class daqAnalysis::Redis {
public:
  class Config {
    public:
    const char *hostname;
    std::vector<unsigned> stream_take;
    std::vector<unsigned> stream_expire;
    bool sub_run_stream;
    unsigned sub_run_stream_expire;
    unsigned first_subrun;
    int snapshot_time;
    int waveform_input_size;
    bool timing;
    Config(): 
      hostname("127.0.0.1"),
      sub_run_stream(false),
      sub_run_stream_expire(0),
      first_subrun(0),
      snapshot_time(-1),
      waveform_input_size(-1),
      timing(false) 
    {}
    unsigned NStreams() { return stream_take.size() + (sub_run_stream ? 1:0); }
  };

  explicit Redis(Config &config);
  ~Redis();
  // send info associated w/ ChannelData
  void ChannelData(std::vector<daqAnalysis::ChannelData> *per_channel_data, std::vector<daqAnalysis::NoiseSample> *noise_samples, 
      std::vector<std::vector<int>> *fem_summed_waveforms, std::vector<std::vector<double>> *fem_summed_fft,
      const art::ValidHandle<std::vector<raw::RawDigit>> &digits, const std::vector<unsigned> &channel_to_index);
  // send info associated w/ HeaderData
  void HeaderData(std::vector<daqAnalysis::HeaderData> *header_data);
  // must be called before calling Send functions
  void StartSend(unsigned sub_run=0);
  // must be called after calling Send functions
  void FinishSend();
  // whether the code will call Snapshot() on ChannelData
  bool WillTakeSnapshot();

protected:
  // per-channel data to redis
  void SendChannelData();
  void FillChannelData(std::vector<daqAnalysis::ChannelData> *per_channel_data);
  // send info associated w/ HeaderData
  void SendHeaderData();
  void FillHeaderData(std::vector<daqAnalysis::HeaderData> *header_data);
  // snapshot stuff
  void Snapshot(std::vector<daqAnalysis::ChannelData> *per_channel_data, std::vector<daqAnalysis::NoiseSample> *noise, 
    std::vector<std::vector<int>> *fem_summed_waveforms, std::vector<std::vector<double>> *fem_summed_fft,
    const art::ValidHandle<std::vector<raw::RawDigit>> &digits, const std::vector<unsigned> &channel_to_index);
  // clear out a pipeline of n_commands commands
  void FinishPipeline(size_t n_commands);

  redisContext *context;
  std::time_t _now;
  std::time_t _start;
  int _snapshot_time;
  // vector of time deltas for each stream
  // the i-th stream with _stram_take[i] = dt will send info to redis every dt seconds
  std::vector<unsigned> _stream_take;
  // vector of the storage time for each stream
  // the i-th stream with _stream_expire[i] = dt will keep data in redis for dt seconds
  std::vector<unsigned> _stream_expire;
  // last time each stream sent something to redis
  std::vector<std::time_t> _stream_last;
  // whether, this time around, the i-th stream will send to redis. Calculated in StartSend()
  std::vector<bool> _stream_send;
  // whether there is a sub run stream
  bool _sub_run_stream;
  // expire time on sub run stream
  unsigned _sub_run_stream_expire;
  // total number of streams
  unsigned _n_streams;
  // current subrun analyzed
  unsigned _this_subrun;
  // last subrun analyzed
  unsigned _last_subrun;
  // last time a snapshot was sent
  std::time_t _last_snapshot;
  // whether this is the first run
  bool _first_run;

  // running averates of Redis metrics per stream
  std::vector<daqAnalysis::RedisRMS> _rms;
  std::vector<daqAnalysis::RedisBaseline> _baseline;
  std::vector<daqAnalysis::RedisDNoise> _dnoise;
  std::vector<daqAnalysis::RedisPulseHeight> _pulse_height;
  std::vector<daqAnalysis::RedisOccupancy> _occupancy;
  std::vector<daqAnalysis::RedisRawHitPulseHeight> _rawhit_pulse_height;
  std::vector<daqAnalysis::RedisRawHitOccupancy> _rawhit_occupancy;

  // header info
  std::vector<daqAnalysis::RedisFrameNo> _frame_no;
  std::vector<daqAnalysis::RedisTrigFrameNo> _trig_frame_no;
  std::vector<daqAnalysis::RedisEventNo> _event_no;
  std::vector<daqAnalysis::RedisBlocks> _blocks;

  // FFT's for snapshots
  FFTManager _fft_manager;

  bool _do_timing;
  daqAnalysis::RedisTiming _timing;
};

#endif /* Redis_h */
