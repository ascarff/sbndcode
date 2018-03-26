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
}


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
    double frame_to_dt;
    bool verbose;
    int n_events;
    size_t n_channels;
    art::InputTag daq_tag;
    int static_input_size;

    bool redis;

    double threshold_hi;
    double threshold_lo;
    
    unsigned noise_range_sampling;
    unsigned n_noise_samples;
    unsigned n_smoothing_samples;

    AnalysisConfig(const fhicl::ParameterSet &param);
    AnalysisConfig() {}
  };

  // other functions
  void ProcessChannel(const raw::RawDigit &digits);

  void ReportEvent(art::Event const &art_event);

  short Mode(const std::vector<short> &adcs);

private:
  // Declare member data here.
  AnalysisConfig _config;
  std::vector<daqAnalysis::ChannelData> _per_channel_data;
  std::vector<daqAnalysis::NoiseSample> _noise_samples;
  unsigned _event_ind;
  TTree *_output;
  FFTManager _fft_manager;
  // use pointer to avoid double-construction
  daqAnalysis::Redis *_redis_manager;
};

#endif /* Analysis_h */
