#ifndef Analysis_h
#define Analysis_h
////////////////////////////////////////////////////////////////////////
// Class:       SimpleDaqAnalysis
// Plugin Type: analyzer (art v2_07_03)
// File:        Analysis.h
//
// Generated at Tue Feb 20 14:22:21 2018 by Gray Putnam using cetskelgen
// from cetlib version v3_00_01.
////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
#include <ctime>

#include "TROOT.h"
#include "TTree.h"

#include "art/Framework/Core/EDAnalyzer.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "lardataobj/RawData/RawDigit.h"

#include "ChannelData.hh"
#include "HeaderData.hh"
#include "FFT.hh"
#include "Redis.hh"
#include "Noise.hh"

/*
  * Main analysis code of the online Monitoring.
  * Takes as input raw::RawDigits and produces
  * a number of useful metrics all defined in 
  * ChannelData.hh
*/

namespace daqAnalysis {
  class SimpleDaqAnalysis;
  class Timing;
}

// keep track of timing information
class daqAnalysis::Timing {
public:
  clock_t start;
  float fill_waveform;
  float baseline_calc;
  float execute_fft;
  float calc_threshold;
  float find_peaks;
  float calc_noise;
  float reduce_data;
  float coherent_noise_calc;
  float copy_headers;
  float write_to_file;
  float redis_channel_data;
  float redis_header_data;

  Timing():
    fill_waveform(0),
    baseline_calc(0),
    execute_fft(0),
    calc_threshold(0),
    find_peaks(0),
    calc_noise(0),
    reduce_data(0),
    coherent_noise_calc(0),
    copy_headers(0),
    write_to_file(0),
    redis_channel_data(0),
    redis_header_data(0)
  {}
  
  void StartTime();
  void EndTime(float *field);

  void Print();
};


class daqAnalysis::SimpleDaqAnalysis : public art::EDAnalyzer {
public:
  explicit SimpleDaqAnalysis(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  SimpleDaqAnalysis(SimpleDaqAnalysis const &) = delete;
  SimpleDaqAnalysis(SimpleDaqAnalysis &&) = delete;
  SimpleDaqAnalysis & operator = (SimpleDaqAnalysis const &) = delete;
  SimpleDaqAnalysis & operator = (SimpleDaqAnalysis &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;

  // configuration
  struct AnalysisConfig {
    public:
    std::string output_file_name;
    float frame_to_dt;
    bool verbose;
    int n_events;
    size_t n_channels;
    art::InputTag daq_tag;
    int static_input_size;

    bool redis;
    int n_headers;

    int16_t threshold;
    float threshold_sigma;
    
    unsigned baseline_calc;
    unsigned noise_range_sampling;
    bool use_planes;
    unsigned threshold_calc;
    unsigned n_noise_samples;
    unsigned n_smoothing_samples;

    bool fft_per_channel;
    bool reduce_data;
    bool write_to_file;
    bool timing;

    AnalysisConfig(const fhicl::ParameterSet &param);
    AnalysisConfig() {}
  };

  // other functions
  void ProcessChannel(const raw::RawDigit &digits);
  void ProcessHeader(const daqAnalysis::HeaderData &header);

  void ReportEvent(art::Event const &art_event);

private:
  // Declare member data here.
  AnalysisConfig _config;
  std::vector<daqAnalysis::ChannelData> _per_channel_data;
  std::vector<daqAnalysis::ReducedChannelData> _per_channel_data_reduced;
  std::vector<daqAnalysis::NoiseSample> _noise_samples;
  std::vector<daqAnalysis::HeaderData> _header_data;
  std::vector<RunningThreshold> _thresholds;
  unsigned _event_ind;
  TTree *_output;
  FFTManager _fft_manager;
  // use pointer to avoid double-construction
  daqAnalysis::Redis *_redis_manager;
  // keep track of timing data (maybe)
  daqAnalysis::Timing _timing;
};



#endif /* Analysis_h */
