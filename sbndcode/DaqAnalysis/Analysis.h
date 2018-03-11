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
    unsigned n_baseline_samples;
    unsigned n_smoothing_samples;
    size_t n_channels;
    art::InputTag daq_tag;
    int static_input_size;
    bool redis;

    AnalysisConfig(const fhicl::ParameterSet &param);
    AnalysisConfig() {}
  };

  // other functions
  void ProcessChannel(const raw::RawDigit &digits);

  void ReportEvent(art::Event const &art_event);

private:
  // Declare member data here.
  AnalysisConfig _config;
  std::vector<daqAnalysis::ChannelData> _per_channel_data;
  unsigned _event_ind;
  TTree *_output;
  FFTManager _fft_manager;
  // This is a pointer b.c. art for some reason default constructs 
  // the Analysis class before constructing it w/ a ParameterSet 
  // and we don't want Redis() to get default constructed. 
  // So there might be a better way to do it but I just threw the 
  // Redis behind a pointer so it gets default constructed to NULL.
  daqAnalysis::Redis *_redis_manager;
};

#endif /* Analysis_h */
