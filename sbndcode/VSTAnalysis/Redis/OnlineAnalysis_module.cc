#include <vector>
#include <iostream>
#include <string>

#include "TROOT.h"
#include "TTree.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 

#include "../ChannelData.hh"
#include "../HeaderData.hh"
#include "../Analysis.hh"

#include "Redis.hh"

/*
 * Uses the Analysis class to send stuff to Redis
*/

namespace daqAnalysis {
  class OnlineAnalysis;
}


class daqAnalysis::OnlineAnalysis : public art::EDAnalyzer {
public:
  explicit OnlineAnalysis(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  OnlineAnalysis(OnlineAnalysis const &) = delete;
  OnlineAnalysis(OnlineAnalysis &&) = delete;
  OnlineAnalysis & operator = (OnlineAnalysis const &) = delete;
  OnlineAnalysis & operator = (OnlineAnalysis &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;
private:
  daqAnalysis::Analysis _analysis;
  daqAnalysis::Redis *_redis_manager;
};

daqAnalysis::OnlineAnalysis::OnlineAnalysis(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p),
  _analysis(p) 
{
  // config
  auto stream_take = p.get<std::vector<unsigned>>("stream_take");
  auto stream_expire = p.get<std::vector<unsigned>>("stream_expire");
  int snapshot_time = p.get<int>("snapshot_time", -1);
  const char *hostname = p.get<std::string>("hostname", "127.0.0.1").c_str();
  
  // have Redis alloc fft if you don't calculate them and you know the input size
  int waveform_input_size = (!_analysis._config.fft_per_channel && _analysis._config.static_input_size > 0) ?
    _analysis._config.static_input_size : -1;
  // setup redis
  _redis_manager = new Redis(hostname, stream_take, stream_expire, snapshot_time, waveform_input_size, _analysis._config.timing);

}

void daqAnalysis::OnlineAnalysis::analyze(art::Event const & e) {
  _analysis.AnalyzeEvent(e);
  auto const& raw_digits_handle = e.getValidHandle<std::vector<raw::RawDigit>>(_analysis._config.daq_tag);
  if (_analysis.ReadyToProcess() && !_analysis.EmptyEvent()) {
    _redis_manager->StartSend();
    _redis_manager->SendChannelData(&_analysis._per_channel_data, &_analysis._noise_samples, &_analysis._fem_summed_waveforms, 
        raw_digits_handle, _analysis._channel_index_map);
    // send headers if _analysis was configured to copy them
    if (_analysis._config.n_headers > 0) {
        _redis_manager->SendHeaderData(&_analysis._header_data);
    }
    _redis_manager->FinishSend();
  }
}


DEFINE_ART_MODULE(daqAnalysis::OnlineAnalysis)
