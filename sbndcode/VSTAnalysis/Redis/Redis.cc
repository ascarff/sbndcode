#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <ctime>
#include <numeric>
#include <chrono>
#include <iostream>
#include <string> 

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "art/Framework/Principal/Handle.h"

#include "lardataobj/RawData/RawDigit.h"

#include "../ChannelData.hh"
#include "../HeaderData.hh"
#include "../Noise.hh"
#include "../ChannelMap.hh"
#include "../FFT.hh"

#include "Redis.hh"
#include "RedisData.hh"

using namespace daqAnalysis;
using namespace std;

Redis::Redis(Redis::Config &config):
  context(redisConnect(config.hostname, 6379)),
  _now(std::time(nullptr)),
  _start(std::time(nullptr)),
  _snapshot_time(config.snapshot_time),
  _stream_take(config.stream_take),
  _stream_expire(config.stream_expire),
  _stream_last(config.NStreams(), _now),
  _stream_send(config.NStreams(), false),
  _sub_run_stream(config.sub_run_stream),
  _sub_run_stream_expire(config.sub_run_stream_expire),
  _n_streams(config.NStreams()),
  _this_subrun(config.first_subrun),
  _last_subrun(config.first_subrun),
  _last_snapshot(0),
  _first_run(true),

  // allocate and zero-initalize all of the metrics
  _rms(config.NStreams(), RedisRMS()),
  _baseline(config.NStreams(), RedisBaseline()),
  _dnoise(config.NStreams(), RedisDNoise()),
  _pulse_height(config.NStreams(), RedisPulseHeight()),
  _occupancy(config.NStreams(), RedisOccupancy()),

  // and the header stuff
  _frame_no(config.NStreams(), RedisFrameNo()),
  _trig_frame_no(config.NStreams(), RedisTrigFrameNo()),
  _event_no(config.NStreams(), RedisEventNo()),
  _blocks(config.NStreams(), RedisBlocks()),

  _fft_manager((config.waveform_input_size > 0) ? config.waveform_input_size: 0),
  _do_timing(config.timing)
{
  // set the subrun stream last correctly
  if (config.sub_run_stream) {
    _stream_last[_n_streams - 1] = config.first_subrun;
  }

  if (context != NULL && context->err) {
    std::cerr << "Redis error: " <<  context->errstr << std::endl;
    exit(1);
  }
}

Redis::~Redis() {
  //redisAsyncDisconnect(context);
  redisFree(context);
}

void Redis::StartSend(unsigned sub_run) {
  _now = std::time(nullptr);

  for (size_t i = 0; i < _stream_take.size(); i++) {
    // calculate whether each stream is sending to redis on this event
    _stream_send[i] = _stream_last[i] != _now && (_now - _start) % _stream_take[i] == 0;
    if (_stream_send[i]) {
      _stream_last[i] = _now;
    }
  }
  // special-case the sub-run stream
  // this one is sent if we're on a new sub-run
  if (_sub_run_stream) {
    unsigned sub_run_ind = _n_streams - 1;  
    _stream_send[sub_run_ind] = _stream_last[sub_run_ind] != sub_run; 
    if (_stream_send[sub_run_ind]) {
      _stream_last[sub_run_ind] = sub_run;
    }
  }
  _this_subrun = sub_run;
}

void Redis::FinishSend() {
  if (_do_timing) {
    _timing.Print();
  }
  // if a new subrun, set the value in redis
  if (_this_subrun != _last_subrun) {
    redisCommand(context, "SET last_subrun_no %u", _last_subrun);
  }
  _last_subrun = _this_subrun;
  _first_run = false;
}

void Redis::HeaderData(vector<daqAnalysis::HeaderData> *header_data) {
  SendHeaderData();
  FillHeaderData(header_data);
}

void Redis::FillHeaderData(vector<daqAnalysis::HeaderData> *header_data) {
  for (auto &header: *header_data) {
    // update header info in each stream
    for (size_t i = 0; i < _n_streams; i++) {
      // index into the fem data cache
      unsigned fem_ind = header.Ind();
      _event_no[i].Add(header, fem_ind);
      _frame_no[i].Add(header, fem_ind);
      _trig_frame_no[i].Add(header, fem_ind);
      _blocks[i].Add(header, fem_ind);
    }
  }
  // update all of the metrics
  for (size_t i = 0; i < _n_streams; i++) {
    _event_no[i].Update();
    _frame_no[i].Update();
    _trig_frame_no[i].Update();
    _blocks[i].Update();
  }
}

void Redis::SendHeaderData() {
  if (_do_timing) {
    _timing.StartTime();
  }
  unsigned n_commands = 0;
  for (size_t i = 0; i < _stream_take.size(); i++) {
    // send headers to redis if need be
    if (_stream_send[i]) {
      unsigned index = _now / _stream_take[i];
      const char *stream_name = std::to_string(_stream_take[i]).c_str(); 
      n_commands += _event_no[i].Send(context, index, stream_name, _stream_expire[i]);
      n_commands += _frame_no[i].Send(context, index, stream_name, _stream_expire[i]);
      n_commands += _trig_frame_no[i].Send(context, index, stream_name, _stream_expire[i]);
      n_commands += _blocks[i].Send(context, index, stream_name, _stream_expire[i]);
      // the metric was taken iff it was sent to redis
      _event_no[i].Clear();
      _frame_no[i].Clear();
      _trig_frame_no[i].Clear();
      _blocks[i].Clear();
    }
  }
  // special case the sub run stream
  if (_sub_run_stream) {
    unsigned sub_run_ind = _n_streams - 1;  
    // send headers to redis if need be
    if (_stream_send[sub_run_ind]) {
      n_commands += _event_no[sub_run_ind].Send(context, _last_subrun, "sub_run", _sub_run_stream_expire);
      n_commands += _frame_no[sub_run_ind].Send(context, _last_subrun, "sub_run", _sub_run_stream_expire);
      n_commands += _trig_frame_no[sub_run_ind].Send(context, _last_subrun, "sub_run", _sub_run_stream_expire);
      n_commands += _blocks[sub_run_ind].Send(context, _last_subrun, "sub_run", _sub_run_stream_expire);

      // the metric was taken iff it was sent to redis
      _event_no[sub_run_ind].Clear();
      _frame_no[sub_run_ind].Clear();
      _trig_frame_no[sub_run_ind].Clear();
      _blocks[sub_run_ind].Clear();
    }
  }
  
  // actually send the commands out of the pipeline
  FinishPipeline(n_commands);
  
  if (_do_timing) {
    _timing.EndTime(&_timing.send_header_data);
  } 

}

inline size_t pushFFTDat(char *buffer, float re, float im) {
  float dat = re*re + im*im;
  return sprintf(buffer, " %f", dat);
}

void Redis::Snapshot(vector<daqAnalysis::ChannelData> *per_channel_data, vector<NoiseSample> *noise, vector<vector<int>> *fem_summed_waveforms, 
    std::vector<std::vector<double>> *fem_summed_fft, const art::ValidHandle<std::vector<raw::RawDigit>> &digits, const std::vector<unsigned> &channel_to_index) {
  size_t n_commands = 0;

  // record the time for reference
  redisAppendCommand(context, "SET snapshot:time %i", _now);
  redisAppendCommand(context, "SET snapshot:sub_run %i", _this_subrun);
  n_commands += 2;

  if (_do_timing) _timing.StartTime();

  // stuff per fem
  if (fem_summed_waveforms->size() != 0) {
    for (unsigned fem_ind = 0; fem_ind < ChannelMap::NFEM(); fem_ind++) {
      // sending the summed waveforms to redis
      // delete old list
      redisAppendCommand(context, "DEL snapshot:waveform:fem:%i", fem_ind);
      {
	// buffer for writing out waveform
	size_t buffer_len = (*fem_summed_waveforms)[fem_ind].size() * 10 + 50;
	char *buffer = new char[buffer_len];
	
	// print in the base of the command
	// assumes fem_summed_waveforms are already sorted by id
	size_t print_len = sprintf(buffer, "RPUSH snapshot:waveform:fem:%i", fem_ind);
	char *buffer_index = buffer + print_len;
	for (int dat: (*fem_summed_waveforms)[fem_ind]) {
	  print_len += sprintf(buffer_index, " %i", dat); 
 	  buffer_index = buffer + print_len;
	  if (print_len >= buffer_len - 1) {
            std::cerr << "ERROR: BUFFER OVERFLOW IN WAVEFORM DATA" << std::endl;
            std::exit(1);
          }
	}
	// submit the command
	redisAppendCommand(context, buffer);
	n_commands += 2;
	
	delete buffer;
      }
    }
  }
  // send fft's
  if (fem_summed_fft->size() != 0) {
    for (unsigned fem_ind = 0; fem_ind < ChannelMap::NFEM(); fem_ind++) {
      // sending the ffts of summed waveforms to redis
      // delete old list
      redisAppendCommand(context, "DEL snapshot:fft:fem:%i", fem_ind);
      {
        // allocate buffer for fft storage command
        // FFT's are comprised of floats, which can get pretty big
        // so assume you need ~25 digits per float to be on the safe side
	size_t buffer_len = (*fem_summed_fft)[fem_ind].size() * 25 + 50;
	char *buffer = new char[buffer_len];
	
	// print in the base of the command
	// assumes fem_summed_fft is already sorted by id
	size_t print_len = sprintf(buffer, "RPUSH snapshot:fft:fem:%i", fem_ind);
	char *buffer_index = buffer + print_len;
	for (double dat: (*fem_summed_fft)[fem_ind]) {
	  print_len += sprintf(buffer_index, " %f", dat); 
 	  buffer_index = buffer + print_len;
	  if (print_len >= buffer_len - 1) {
            std::cerr << "ERROR: BUFFER OVERFLOW IN WAVEFORM DATA" << std::endl;
            std::exit(1);
          }
	}
	// submit the command
	redisAppendCommand(context, buffer);
	n_commands += 2;
	
	delete buffer;
      }
    }
  }

  if (_do_timing) _timing.EndTime(&_timing.fem_waveforms);

  // stuff per channel
  for (auto &channel: *per_channel_data) { 
    if (_do_timing) {
      _timing.StartTime();
    }
    // store the waveform and fft's
    // also delete old lists
    redisAppendCommand(context, "DEL snapshot:waveform:wire:%i", channel.channel_no);
    
    // we're gonna put the whole waveform into one very large list 
    // allocate enough space for it 
    // Assume at max 4 chars per int plus a space each plus another 50 chars to store the base of the command
    {
      unsigned digits_ind = channel_to_index[channel.channel_no];
      auto waveform = (*digits)[digits_ind].ADCs();
      size_t buffer_len = waveform.size() * 10 + 50;
      char *buffer = new char[buffer_len];

      // print in the base of the command
      size_t print_len = sprintf(buffer, "RPUSH snapshot:waveform:wire:%i", channel.channel_no);
      char *buffer_index = buffer + print_len;
      // throw in all of the data points
      for (int16_t dat: waveform) {
	/* IGNORE: Possible evil way of pushing stuff to redis to be possibly used later */
	// add the space from the previous command
	//*buffer_index = ' ';
	//print_len ++;
	//buffer_index = buffer + print_len;
	
	// add the number in binary:
	// coerce the integer into a char[2]:
	//char *binary_dat = (char *) &dat;
	// to avoid null-termination, mask w/ 
	//buffer_index[0] = binary_dat[0];
	//buffer_index[1] = binary_dat[1];
	
	// 2 bytes
	//print_len += 2;
	
	print_len += sprintf(buffer_index, " %i", dat); 
	
	buffer_index = buffer + print_len;
	
	if (print_len >= buffer_len - 1) {
          std::cerr << "ERROR: BUFFER OVERFLOW IN WAVEFORM DATA" << std::endl;
          std::exit(1);
        }
      }
      // null terminate the string
      //*buffer_index = '\0';
      
      redisAppendCommand(context, buffer);
      n_commands += 2;
      // delete the buffer
      delete buffer;
    }

    if (_do_timing) {
      _timing.EndTime(&_timing.send_waveform);
    }

    if (_do_timing) {
      _timing.StartTime();
    }

    redisAppendCommand(context, "DEL snapshot:fft:wire:%i", channel.channel_no);

    {
      unsigned digits_ind = channel_to_index[channel.channel_no];
      auto waveform = (*digits)[digits_ind].ADCs();
      // allocate buffer for fft storage command
      // FFT's are comprised of floats, which can get pretty big
      // so assume you need ~25 digits per float to be on the safe side
      size_t buffer_len = (waveform.size()/2 +1) * 25 + 50;
      char *buffer = new char[buffer_len];
      
      // print in the base of the command
      size_t print_len = sprintf(buffer, "RPUSH snapshot:fft:wire:%i", channel.channel_no);
      char *buffer_index = buffer + print_len;
      // throw in all of the data points
      
      // use already calculated FFT if there
      if (channel.fft_real.size() != 0) {
        for (size_t i = 0; i < channel.fft_real.size(); i++) {
          print_len += pushFFTDat(buffer_index, channel.fft_real[i], channel.fft_imag[i]);
          buffer_index = buffer + print_len;
        }
      }
      // otherwise calculate it yourself
      else {
        // re-allocate if necessary
        if (_fft_manager.InputSize() != waveform.size()) {
          _fft_manager.Set(waveform.size());
        }
        // fill up the fft data in
        for (size_t i = 0; i < waveform.size(); i++) {
          double *input = _fft_manager.InputAt(i);
          *input = (double) waveform[i];
        }
        _fft_manager.Execute();
        int adc_fft_size = _fft_manager.OutputSize();
        for (int i = 0; i < adc_fft_size; i++) {
          print_len += pushFFTDat(buffer_index, _fft_manager.ReOutputAt(i), _fft_manager.ImOutputAt(i));
          buffer_index = buffer + print_len;
        }
      }
      redisAppendCommand(context, buffer);
      n_commands += 2;
      // delete the buffer
      delete buffer;
    }

    if (_do_timing) {
      _timing.EndTime(&_timing.send_fft);
    }
  }

  if (_do_timing) {
    _timing.StartTime();
  }
  // also store the noise correlation matrix if taking a snapshot
  redisAppendCommand(context, "DEL snapshot:correlation");
  
  {
    // allocate for the covariance matrix command
    // again, get ~25 digits for each float
    size_t buffer_len = ((noise->size() + 1) * noise->size() / 2) * 25 + 50;
    char *buffer = new char[buffer_len];
    
    // print in the base of the command
    size_t print_len = sprintf(buffer, "RPUSH snapshot:correlation");
    char *buffer_index = buffer + print_len;
    // Only calculate the upper-right half of the matrix since it is symmetric
    // The index 'k' into the list of the i-th sample with the j-th sample (where i <= j) is:
    // k = ((n+1)*n/2) - (n-i+1)*(n-i)/2 + j - i
    for (size_t i = 0; i < noise->size(); i++) {
      for (size_t j = i; j < noise->size(); j++) {
        unsigned digits_i = channel_to_index[i];
        unsigned digits_j = channel_to_index[j];
        auto waveform_i = (*digits)[digits_i].ADCs();
        auto waveform_j = (*digits)[digits_j].ADCs();

        float correlation = (*noise)[i].Correlation(waveform_i, (*noise)[j], waveform_j);
        print_len += sprintf(buffer_index, " %f", correlation);
        buffer_index = buffer + print_len;
      }
    }
    redisAppendCommand(context, buffer);
    n_commands += 2;
    delete buffer;
  }

  if (_do_timing) {
    _timing.EndTime(&_timing.correlation);
  }

  if (_do_timing) {
    _timing.StartTime();
  }
  FinishPipeline(n_commands);
  if (_do_timing) {
    _timing.EndTime(&_timing.clear_pipeline);
  }
  
}

bool Redis::WillTakeSnapshot() {
  return _snapshot_time > 0 && (_now - _start) % _snapshot_time == 0 && _last_snapshot != _now;
}

void Redis::ChannelData(vector<daqAnalysis::ChannelData> *per_channel_data, vector<NoiseSample> *noise_samples, vector<vector<int>> *fem_summed_waveforms, 
    std::vector<std::vector<double>> *fem_summed_fft, const art::ValidHandle<std::vector<raw::RawDigit>> &digits, const std::vector<unsigned> &channel_to_index) {

  SendChannelData();
  FillChannelData(per_channel_data);

  bool take_snapshot = _snapshot_time > 0 && (_now - _start) % _snapshot_time == 0 && _last_snapshot != _now;
  if (take_snapshot) {
    Snapshot(per_channel_data, noise_samples, fem_summed_waveforms, fem_summed_fft, digits, channel_to_index);
    _last_snapshot = _now;
  }

}

void Redis::FillChannelData(vector<daqAnalysis::ChannelData> *per_channel_data) {
  if (_do_timing) {
    _timing.StartTime();
  }

  unsigned n_channels = per_channel_data->size();
  unsigned n_fem = ChannelMap::NFEM();
  bool at_end_of_detector = false;

  // iterate over crates and fems
  for (unsigned crate = 0; crate < daqAnalysis::ChannelMap::n_crate; crate++) {
    for (unsigned fem = 0; fem < daqAnalysis::ChannelMap::n_fem_per_crate; fem++) {
      // @VST INSTALLATION: OK -- crate is always 0
      // index into the fem data cache
      unsigned fem_ind = crate * ChannelMap::n_fem_per_crate + fem;

      // detect if at end of fem's
      if (fem_ind >= n_fem) {
        at_end_of_detector = true;
        break;
      }

      for (unsigned channel = 0; channel < daqAnalysis::ChannelMap::n_channel_per_fem; channel++) {
        // get the wire number
	daqAnalysis::ChannelMap::readout_channel readout_channel {crate, fem, channel};
	uint16_t wire = daqAnalysis::ChannelMap::Channel2Wire(readout_channel);
        // detect if at end of detector
        if (wire >= n_channels) {
          at_end_of_detector = true;
          break;
        }
 
        // fill metrics for each stream
        for (size_t i = 0; i < _n_streams; i++) {
          _rms[i].Add((*per_channel_data)[wire], crate, fem_ind, wire);
          _baseline[i].Add((*per_channel_data)[wire], crate, fem_ind, wire);
          _dnoise[i].Add((*per_channel_data)[wire], crate, fem_ind, wire);
          _pulse_height[i].Add((*per_channel_data)[wire], crate, fem_ind, wire);
          _occupancy[i].Add((*per_channel_data)[wire], crate, fem_ind, wire);
        }

      }

      // if at end, break
      if (at_end_of_detector) {
        break;
      }
  
    }
    // if at end, break
    if (at_end_of_detector) {
      break;
    }
  }
  if (_do_timing) {
    _timing.EndTime(&_timing.copy_data);
  }

  // update all of the metrics
  for (size_t i = 0; i < _n_streams; i++) {
    _rms[i].Update();
    _baseline[i].Update();
    _dnoise[i].Update();
    _pulse_height[i].Update();
    _occupancy[i].Update();
  }
}

void Redis::SendChannelData() {
  unsigned n_commands = 0;
  for (size_t i = 0; i < _stream_take.size(); i++) {
    // Send stuff to redis if it's time
    if (_stream_send[i]) {
      if (_do_timing) {
        _timing.StartTime();
      }
      unsigned index = _now / _stream_take[i];
      const char *stream_name = std::to_string(_stream_take[i]).c_str(); 
      // metrics control the sending of everything else
      n_commands += _rms[i].Send(context, index, stream_name, _stream_expire[i]);
      n_commands += _baseline[i].Send(context, index, stream_name, _stream_expire[i]);
      n_commands += _dnoise[i].Send(context, index, stream_name, _stream_expire[i]);
      n_commands += _occupancy[i].Send(context, index, stream_name, _stream_expire[i]);
      n_commands += _pulse_height[i].Send(context, index, stream_name, _stream_expire[i]);

      _rms[i].Clear();
      _baseline[i].Clear();
      _dnoise[i].Clear();
      _pulse_height[i].Clear();
      _occupancy[i].Clear();

      if (_do_timing) {
        _timing.EndTime(&_timing.send_metrics);
      }
    }
  }
  // special case the sub run stream
  if (_sub_run_stream) {
    unsigned sub_run_ind = _n_streams - 1;  
    // Send stuff to redis if it's time
    if (_stream_send[sub_run_ind]) {
      if (_do_timing) {
        _timing.StartTime();
      }
      // metrics control the sending of everything else
      n_commands += _rms[sub_run_ind].Send(context, _last_subrun, "sub_run", _sub_run_stream_expire);
      n_commands += _baseline[sub_run_ind].Send(context, _last_subrun, "sub_run", _sub_run_stream_expire);
      n_commands += _dnoise[sub_run_ind].Send(context, _last_subrun, "sub_run", _sub_run_stream_expire);
      n_commands += _occupancy[sub_run_ind].Send(context, _last_subrun, "sub_run", _sub_run_stream_expire);
      n_commands += _pulse_height[sub_run_ind].Send(context, _last_subrun, "sub_run", _sub_run_stream_expire);

      // the metric was taken iff it was sent to redis
      // clear all of the metrics
      _rms[sub_run_ind].Clear();
      _baseline[sub_run_ind].Clear();
      _dnoise[sub_run_ind].Clear();
      _pulse_height[sub_run_ind].Clear();
      _occupancy[sub_run_ind].Clear();

      if (_do_timing) {
        _timing.EndTime(&_timing.send_metrics);
      }
    }
  }

  // actually send the commands out of the pipeline
  FinishPipeline(n_commands);
}

// clear out a sequence of Append Commands
void Redis::FinishPipeline(size_t n_commands) {
  // TODO: Error Handling
  void *reply;
  for (size_t i =0; i <n_commands; i++) {
    redisGetReply(context, &reply); 
    freeReplyObject(reply);
  }
}

void RedisTiming::StartTime() {
  start = std::chrono::high_resolution_clock::now(); 
}
void RedisTiming::EndTime(float *field) {
  auto now = std::chrono::high_resolution_clock::now();
  *field += std::chrono::duration<float, std::milli>(now- start).count();
}
void RedisTiming::Print() {
  std::cout << "COPY DATA: " << copy_data << std::endl;
  std::cout << "SEND METRICS: " << send_metrics << std::endl;
  std::cout << "SEND HEADER : " << send_header_data << std::endl;
  std::cout << "SEND WAVEFORM " << send_waveform << std::endl;
  std::cout << "SEND FFT    : " << send_fft << std::endl;
  std::cout << "CORRELATION : " << correlation << std::endl;
  std::cout << "CLEAR PIPE  : " << clear_pipeline << std::endl;
  std::cout << "FEM WAVEFORM: " << fem_waveforms << std::endl;
}

