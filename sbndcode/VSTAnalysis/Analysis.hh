#ifndef Analysis_hh
#define Analysis_hh

#include <vector>
#include <string>
#include <ctime>
#include <chrono>
#include <numeric>

#include "TROOT.h"
#include "TTree.h"

#include "art/Framework/Core/EDAnalyzer.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RecoBase/Hit.h"

#include "ChannelData.hh"
#include "HeaderData.hh"
#include "NevisTPCMetaData.hh"
#include "VSTChannelMap.hh"
#include "FFT.hh"
#include "Noise.hh"
#include "EventInfo.hh"

/*
  * Main analysis code of the online Monitoring.
  * Takes as input raw::RawDigits and produces
  * a number of useful metrics all defined in 
  * ChannelData.hh
*/

namespace daqAnalysis {
  class Analysis;
  class Timing;
}

// keep track of timing information
class daqAnalysis::Timing {
public:
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  float fill_waveform;
  float baseline_calc;
  float execute_fft;
  float calc_threshold;
  float find_peaks;
  float calc_noise;
  float reduce_data;
  float coherent_noise_calc;
  float copy_headers;

  Timing():
    fill_waveform(0),
    baseline_calc(0),
    execute_fft(0),
    calc_threshold(0),
    find_peaks(0),
    calc_noise(0),
    reduce_data(0),
    coherent_noise_calc(0),
    copy_headers(0)
  {}
  
  void StartTime();
  void EndTime(float *field);

  void Print();
};


class daqAnalysis::Analysis {
public:
  explicit Analysis(fhicl::ParameterSet const & p);

  // actually analyze stuff
  void AnalyzeEvent(art::Event const & e);
  // sum FEM waveforms
  void SumWaveforms(art::Event const & event);

  // configuration
  struct AnalysisConfig {
    public:
    std::string output_file_name;
    bool verbose;
    int n_events;
    art::InputTag daq_tag;
    int static_input_size;

    int n_headers;
    int n_metadata;

    float threshold;
    float threshold_sigma;
    
    unsigned baseline_calc;
    bool refine_baseline;
    unsigned n_mode_skip;
    unsigned noise_range_sampling;
    bool use_planes;
    unsigned threshold_calc;
    unsigned n_noise_samples;
    unsigned n_smoothing_samples;
    unsigned n_above_threshold;

    bool sum_waveforms;
    bool fft_summed_waveforms;
    bool fft_per_channel;
    bool fill_waveforms;
    bool reduce_data;
    bool timing;
    bool fUseRawHits;
    bool fProcessRawHits;
    bool fUseNevisClock;
    bool fDoPurityAna;
    bool fCosmicRun;

    std::string fHitsModuleLabel;

    //Purity config... Sorry for loads I don't know if they will ned changing.  
    int min_col_hits;       // Minimum number of collection plane hits          
    int min_ind_hits;       // Minimum number of induction plane hits
    int min_wires;          // Minimum extent of track in wire number
    int min_ticks;          // Minimum extent of track in time [ticks]
    float trigger_time;     // Time between cosmic trigger and readout [ticks]
    float chi2_cut;         // Minimum chi2/ndof after 2 linear fits
    float pca_cut;          // Maximum value of principal component analysis
    float min_overlap;      // Minimim percentage overlap pf col/ind tracks in time
    float charge_width;     // Sigma multiplier for landau tail cut
    float shaping_time;     // Shaping time [us]
    float drift_vel;        // Drift velocity[mm/us]
    float wire_spacing;     // Wire spacing [mm]
    float angle_cut;        // Maximum value of angle to wire planes? [degrees]
    bool force_angle_cut;   // Use the angle cut rather than atan(st*dv/ws)
    bool do_angle_cut;      // Apply the angle cut
    bool lifetime_plots;    // Make plots
    bool purity_verbose;    // Print Stuff

    AnalysisConfig(const fhicl::ParameterSet &param);
    AnalysisConfig() {}
  };

  // other functions
  void ProcessChannel(const raw::RawDigit &digits);
  void ProcessChannel(const raw::RawDigit &digits, const std::vector<art::Ptr<recob::Hit> > &hits);
  void ProcessHeader(const daqAnalysis::HeaderData &header);
  void ProcessMetaData(const daqAnalysis::NevisTPCMetaData &metadata); 
  void ProcessEventInfo(double &lifetime);

  // if the containers filled by the analysis are ready to be processed
  bool ReadyToProcess();
  bool EmptyEvent();

private:
  // Declare member data here.
  // handle to the channel map service
  art::ServiceHandle<daqAnalysis::VSTChannelMap> _channel_map;

public:
  // configuration is available publicly
  AnalysisConfig _config;
  // keeping track of wire id to index into stuff from Decoder
  std::vector<unsigned> _channel_index_map;
  // output containers of analysis code. Only use after calling ReadyToProcess()
  std::vector<daqAnalysis::ChannelData> _per_channel_data;
  std::vector<daqAnalysis::ReducedChannelData> _per_channel_data_reduced;
  std::vector<daqAnalysis::NoiseSample> _noise_samples;
  std::vector<daqAnalysis::HeaderData> _header_data;
  daqAnalysis::EventInfo _event_info;
  std::vector<daqAnalysis::NevisTPCMetaData> _nevis_tpc_metadata;
  std::vector<RunningThreshold> _thresholds;
  std::vector<std::vector<int>> _fem_summed_waveforms;
  std::vector<std::vector<double>> _fem_summed_fft;

private:
  unsigned _event_ind;
  FFTManager _fft_manager;
  // keep track of timing data (maybe)
  daqAnalysis::Timing _timing;
  // whether we have analyzed stuff
  bool _analyzed;
  uint32_t _sub_run_start_time;
  uint32_t _sub_run_holder;

};



#endif /* Analysis_hh */
