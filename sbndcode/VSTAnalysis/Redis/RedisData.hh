#ifndef RedisData_h
#define RedisData_h

#include <vector>
#include <ctime>
#include <cmath>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "../ChannelData.hh"
#include "../HeaderData.hh"
#include "../ChannelMap.hh"

namespace daqAnalysis {
  class StreamDataMean;
  class StreamDataVariableMean;
  class StreamDataMax;

  template<class Stream, char const *REDIS_NAME>
  class DetectorMetric;

  class RedisRMS;
  class RedisOccupancy;
  class RedisBaseline;
  class RedisPulseHeight;
  class RedisDNoise;
}

// keeps a running mean of a metric w/ n_data instances and n_points_per_time data points per each time instance
class daqAnalysis::StreamDataMean {
public:
  StreamDataMean(unsigned n_data, unsigned n_points_per_time): _data(n_data, 0.), _n_values(0), _instance_data(n_data, 0.), _n_points_per_time(n_data, n_points_per_time) {}

  // add in a new value
  void Add(unsigned index, float dat);
  // incl the number of values
  void Incl();
  // clear the number of values
  void Clear();
  // take the data value and reset it
  float Take(unsigned index);
  // just take a peek at the data value
  float Peak(unsigned index) { return _data[index]; }
  // returns n_data
  unsigned Size() { return _data.size(); }
  // called per iter
  void Update(bool taken);
  // update a points per time value
  void SetPointsPerTime(unsigned index, unsigned points)
    { _n_points_per_time[index] = points; }

protected:
  // add data from this time instance into the main data container
  void AddInstance(unsigned index);

  // internal data
  std::vector<float> _data;
  // number of values averaged together in each data point
  unsigned _n_values;

  // average of values for this time instance
  std::vector<float> _instance_data;
  // number of values averaged together in each time instance
  // can be different per data point
  std::vector<unsigned> _n_points_per_time;
};

// keeps a running mean of a metric w/ n_data data points
// where each data point may have a different number of entries and may
// receive a different number of values at each time instance 
class daqAnalysis::StreamDataVariableMean {
public:
  // n_points_per_time is not used in this class
  StreamDataVariableMean(unsigned n_data, unsigned _): _data(n_data, 0.), _n_values(n_data, 0), _instance_data(n_data, 0), _n_values_current_instance(n_data,0) {}

  // add in a new value
  void Add(unsigned index, float dat);
  // take the data value and reset it
  float Take(unsigned index);
  // just take a peek at the data value
  float Peak(unsigned index) { return _data[index]; }
  // returns n_data
  unsigned Size() { return _data.size(); }
  // called per iter
  void Update(bool taken);

  // update a points per time value
  // does nothing, since all points per time values are dynamic for this class
  void SetPointsPerTime(unsigned index, unsigned points) {}

protected:
  // add data from this time instance into the main data container
  void AddInstance(unsigned index);

  // internal data
  std::vector<float> _data;
  // number of values averaged together in each data point
  std::vector<unsigned> _n_values;

  // data of the current time instance
  std::vector<float> _instance_data;
  // number of values averaged together in the current time instance
  std::vector<unsigned> _n_values_current_instance;
};

// keeps running max value of a metric w/ n instances
class daqAnalysis::StreamDataMax {
public:
  StreamDataMax(unsigned n_data): _data(n_data, 0.) {}

  void Add(unsigned index, float dat);
  float Take(unsigned index);
  float Peek(unsigned index) { return _data[index]; }
  unsigned Size() { return _data.size(); }
  void Update(bool _) {/* doesn't need to do anything currently */}

protected:
  std::vector<float> _data;
};

// holds a StreamDataMean or StreamDataVariableMean across all instances of the detector
// i.e. per crate, fem, wire, etc.
template<class Stream, char const *REDIS_NAME>
class daqAnalysis::DetectorMetric {
public:
  // how to calculate the metric given a channel data reference
  virtual float Calculate(daqAnalysis::ChannelData &channel) = 0;

  // base class destructors should be virtual
  virtual ~DetectorMetric() {}

  // implementing templated functions in header (because cpp is bad)

  // constructor
  DetectorMetric() :
    _wire_data(ChannelMap::n_wire, 1),
    _fem_data(ChannelMap::NFEM(), ChannelMap::n_channel_per_fem),
    _crate_data(ChannelMap::n_crate, ChannelMap::n_fem_per_crate*ChannelMap::n_channel_per_fem)
  {
    // TODO @INSTALLATION: Make sure that this implementation also works for the VST installation

    unsigned n_fem = ChannelMap::NFEM();
    unsigned n_crate = ChannelMap::n_crate;
    unsigned n_wire = ChannelMap::n_wire;
    unsigned n_fem_per_crate = ChannelMap::n_fem_per_crate;
    unsigned n_channel_per_fem = ChannelMap::n_channel_per_fem;

    unsigned n_channel_last_fem = n_channel_per_fem;
    // last fem might have fewer channels
    if (n_wire % n_channel_per_fem != 0) {
      // shouldn't be missing a whole fem
      assert(n_channel_per_fem * n_fem - n_wire < n_channel_per_crate);

      // last fem has fewer channels
      n_channel_last_fem = n_channel_per_fem - (n_fem * n_channel_per_fem - n_wire);
    }

    unsigned n_fem_last_crate = n_fem_per_crate;
    // if numbers don't add up, the last crate might not have n_fem_per_crate and the last fem might not have n_channel_per_fem
    if (n_fem % n_fem_per_crate != 0) {
      // shouldn't be missing a whole crate
      assert(n_crate * n_fem_per_crate - n_fem < n_fem_per_crate);

      // last crate has fewer fem
      n_fem_last_crate = n_fem_per_crate - (n_crate * n_fem_per_crate - n_fem);
    }

    _fem_data.SetPointsPerTime(n_fem - 1, n_channel_last_fem); 
    _crate_data.SetPointsPerTime(n_crate - 1, (n_fem_last_crate-1)*n_channel_per_fem + n_channel_last_fem);
  }

  // add in data
  void Add(daqAnalysis::ChannelData &channel, unsigned crate, unsigned fem_ind, unsigned wire) {
    // calculate and add to each container
    float dat = Calculate(channel);

    // persist through nan's
    if (std::isnan(dat)) {
      fprintf(stderr, "Error: Metric %s is NAN on wire %i", REDIS_NAME, wire);
      return;
    }

    // each container is aware of how often it is filled per time instance
    _crate_data.Add(crate, dat);
    _fem_data.Add(fem_ind, dat);
    _wire_data.Add(wire, dat);
  }

  float TakeCrate(unsigned index) {
    return _crate_data.Take(index);
  }

  float TakeFEM(unsigned index) {
    return _fem_data.Take(index);
  }

  float TakeWire(unsigned index) {
    return _wire_data.Take(index);
  }

  // to be called once per time instance
  void Update(bool taken) {
    _crate_data.Update(taken);
    _fem_data.Update(taken);
    _wire_data.Update(taken);
  }

  // send stuff to Redis
  unsigned Send(redisContext *context, std::time_t now, unsigned stream_no, unsigned stream_expire) {
    // send all the wire stuff
    unsigned n_wires = _wire_data.Size();
    for (unsigned wire = 0; wire < n_wires; wire++) {
      redisAppendCommand(context, "SET stream/%i:%i:%s:wire:%i %f",
        stream_no, now/stream_no, REDIS_NAME,wire, TakeWire(wire)); 

      if (stream_expire != 0) {
        redisAppendCommand(context, "EXPIRE stream/%i:%i:%s:wire:%i %i",
          stream_no, now/stream_no, REDIS_NAME,wire, stream_expire); 
      }
    } 
    // and the fem stuff
    unsigned n_fem = _fem_data.Size();
    for (unsigned fem_ind = 0; fem_ind < n_fem; fem_ind++) {
      // @VST: this is ok because there is only 1 crate
      // TEMPORARY IMPLEMENTATION
      unsigned fem = fem_ind % ChannelMap::n_fem_per_crate;
      unsigned crate = fem_ind / ChannelMap::n_fem_per_crate;
      redisAppendCommand(context, "SET stream/%i:%i:%s:crate:%i:fem:%i %f",
        stream_no, now/stream_no, REDIS_NAME, crate, fem, TakeFEM(fem_ind)); 

      if (stream_expire != 0) {
        redisAppendCommand(context, "EXPIRE stream/%i:%i:%s:crate:%i:fem:%i %i",
          stream_no, now/stream_no, REDIS_NAME, crate, fem, stream_expire); 
      }
    } 
    // and the crate stuff
    unsigned n_crate = _crate_data.Size();
    for (unsigned crate = 0; crate < n_crate; crate++) {
      redisAppendCommand(context, "SET stream/%i:%i:%s:crate:%i %f",
         stream_no, now/stream_no, REDIS_NAME, crate, TakeCrate(crate));

      if (stream_expire != 0) {
        redisAppendCommand(context, "EXPIRE stream/%i:%i:%s:crate:%i %i",
           stream_no, now/stream_no, REDIS_NAME, crate, stream_expire);
      }
    }
    // return number of commands sent
    return (n_wires + n_fem + n_crate) * ((stream_expire == 0) ? 1:2);
  }

protected:
  Stream _wire_data;
  Stream _fem_data;
  Stream _crate_data;
};

// string literals can't be template arguments for some reason, so declare them here

// also they can't be defined here becasue of stupid linking things
// see RedisData.cc for definitions
extern char REDIS_NAME_RMS[];
extern char REDIS_NAME_OCCUPANCY[];
extern char REDIS_NAME_DNOISE[];
extern char REDIS_NAME_BASLINE[];
extern char REDIS_NAME_PULSE_HEIGHT[];

// RMS is Variable Mean because sometimes noise calculation algorithm can fail
class daqAnalysis::RedisRMS: public daqAnalysis::DetectorMetric<StreamDataVariableMean, REDIS_NAME_RMS> {
  // inherit constructor
  using daqAnalysis::DetectorMetric<StreamDataVariableMean, REDIS_NAME_RMS>::DetectorMetric;

  // implement calculate
 inline float Calculate(daqAnalysis::ChannelData &channel) override
   { return channel.rms; }
};


class daqAnalysis::RedisOccupancy: public daqAnalysis::DetectorMetric<StreamDataMean, REDIS_NAME_OCCUPANCY> {
  // inherit constructor
  using daqAnalysis::DetectorMetric<StreamDataMean, REDIS_NAME_OCCUPANCY>::DetectorMetric;

  // implement calculate
 inline float Calculate(daqAnalysis::ChannelData &channel) override
   { return channel.occupancy; }
};

class daqAnalysis::RedisDNoise: public daqAnalysis::DetectorMetric<StreamDataVariableMean, REDIS_NAME_DNOISE> {
  // inherit constructor
  using daqAnalysis::DetectorMetric<StreamDataVariableMean, REDIS_NAME_DNOISE>::DetectorMetric;

  // implement calculate
 inline float Calculate(daqAnalysis::ChannelData &channel) override
   { return channel.next_channel_dnoise; }
};

class daqAnalysis::RedisBaseline: public daqAnalysis::DetectorMetric<StreamDataMean, REDIS_NAME_BASLINE> {
  // inherit constructor
  using daqAnalysis::DetectorMetric<StreamDataMean, REDIS_NAME_BASLINE>::DetectorMetric;

  // implement calculate
 inline float Calculate(daqAnalysis::ChannelData &channel) override
   { return channel.baseline; }
};

class daqAnalysis::RedisPulseHeight: public daqAnalysis::DetectorMetric<StreamDataVariableMean, REDIS_NAME_PULSE_HEIGHT> {
  // inherit constructor
  using daqAnalysis::DetectorMetric<StreamDataVariableMean, REDIS_NAME_PULSE_HEIGHT>::DetectorMetric;

  // implement calculate
 inline float Calculate(daqAnalysis::ChannelData &channel) override
   { return channel.mean_peak_height; }
};

#endif
