//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <numeric>
#include <getopt.h>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TTree.h"
#include "TFile.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TGraph.h"
#include "TFFTReal.h"

// art includes
#include "canvas/Utilities/InputTag.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "fhiclcpp/ParameterSet.h" 
#include "messagefacility/MessageLogger/MessageLogger.h"  
#include "art/Framework/Services/Optional/TFileService.h"
#include "lardataobj/RawData/RawDigit.h"

#include "Analysis.h"

#include "ChannelData.hh"
#include "FFT.hh"
#include "Noise.hh"
#include "PeakFinder.hh"
#include "Redis.hh"

using namespace daqAnalysis;

SimpleDaqAnalysis::SimpleDaqAnalysis(fhicl::ParameterSet const & p) :
  art::EDAnalyzer{p},
  _config(p),
  _per_channel_data(_config.n_channels)
{
  _event_ind = 0;
  art::ServiceHandle<art::TFileService> fs;

  //_output = fs->makeAndRegister<TBranch>("channel_data", "channel_data", "channel_data", "channel_data", &_per_channel_data);
  _output = fs->make<TTree>("channel_data", "channel_data");
  _output->Branch("channel_data", &_per_channel_data);

  _fft_manager = (_config.static_input_size > 0) ? FFTManager(_config.static_input_size) : FFTManager();
  if (_config.redis) {
    _redis_manager = new Redis(); 
  }
}

SimpleDaqAnalysis::AnalysisConfig::AnalysisConfig(const fhicl::ParameterSet &param) {
  // set up config
  frame_to_dt = param.get<double>("frame_to_dt", 1.6e-3 /* units of seconds */);
  verbose = param.get<bool>("verbose", true);
  n_events = param.get<unsigned>("n_events", 10);
  n_baseline_samples = param.get<unsigned>("n_baseline_samples", 20);
  n_smoothing_samples = param.get<unsigned>("n_smoothing_samples", 1);
  static_input_size = param.get<int>("static_input_size", -1);
  redis = param.get<bool>("redis", false);

  // TODO: how to detect this?
  n_channels = param.get<unsigned>("n_channels", 16 /* currently only the first 16 channels have data */);

  std::string producer = param.get<std::string>("producer_name");
  daq_tag = art::InputTag(producer, ""); 
}

void SimpleDaqAnalysis::analyze(art::Event const & event) {
  //if (_config.n_events >= 0 && _event_ind >= (unsigned)_config.n_events) return false;


  _event_ind ++;

  // clear out containers from last iter
  for (unsigned i = 0; i < _config.n_channels; i++) {
    _per_channel_data[i].waveform.clear();
    _per_channel_data[i].fft_real.clear();
    _per_channel_data[i].fft_imag.clear();
    _per_channel_data[i].peaks.clear();
  }

  auto const& raw_digits_handle = event.getValidHandle<std::vector<raw::RawDigit>>(_config.daq_tag);
  
  for (auto const& digits: *raw_digits_handle) {
    ProcessChannel(digits);
  }

  // now calculate stuff that depends on stuff between channels
  for (unsigned i = 0; i < _config.n_channels; i++) {
    unsigned last_channel_ind = i == 0 ? _config.n_channels - 1 : i - 1;
    unsigned next_channel_ind = i == _config.n_channels - 1 ? 0 : i + 1;
    Noise last_channel_noise(_per_channel_data[i].waveform, _per_channel_data[last_channel_ind].waveform, _config.n_baseline_samples);
    Noise next_channel_noise(_per_channel_data[i].waveform, _per_channel_data[next_channel_ind].waveform, _config.n_baseline_samples);
    _per_channel_data[i].rms = last_channel_noise.RMS1();
    _per_channel_data[i].last_channel_correlation = last_channel_noise.Correlation();
    _per_channel_data[i].next_channel_correlation = next_channel_noise.Correlation();
    // vector assignment copies
    _per_channel_data[i].noise_sample = *last_channel_noise.NoiseSample1();
  }
  ReportEvent(event);
}

void SimpleDaqAnalysis::ReportEvent(art::Event const &art_event) {
  // don't need this for now
  (void) art_event;
  // Fill the output
  _output->Fill();

  // Send stuff to Redis
  Redis::EventDef event;
  event.per_channel_data = &_per_channel_data;
  if (_config.redis) {
    _redis_manager->Send(event);
  }

}

void SimpleDaqAnalysis::ProcessChannel(const raw::RawDigit &digits) {

  /*
  auto fragment_header = fragment.header(); 
  
  //_output_file.cd();
  
  if (_config.verbose) {
    std::cout << "EVENT NUMBER: "  << _header_data.event_number << std::endl;
    std::cout << "FRAME NUMBER: "  << _header_data.frame_number << std::endl;
    std::cout << "CHECKSUM: "  << _header_data.checksum << std::endl;
    std::cout << "ADC WORD COUNT: "  << _header_data.adc_word_count << std::endl;
    std::cout << "TRIG FRAME NUMBER: "  << _header_data.trig_frame_number << std::endl;
  }*/
  
  auto channel = digits.Channel();
  if (channel < _config.n_channels) {
    // re-allocate FFT if necessary
    if (_fft_manager.InputSize() != digits.NADC()) {
      _fft_manager.Set(digits.NADC());
    }
   
    _per_channel_data[channel].channel_no = channel;

    double max = 0;
    for (unsigned i = 0; i < digits.NADC(); i ++) {
      double adc = (double) digits.ADCs()[i];
      if (adc > max) max = adc;
    
      _per_channel_data[channel].waveform.push_back(adc);
      // fill up fftw array
      double *input = _fft_manager.InputAt(i);
      *input = adc;
    }

    // Baseline calculation assumes baseline is constant and that the first
    // `n_baseline_samples` of the adc values represent a baseline
    _per_channel_data[channel].baseline = 
        std::accumulate(_per_channel_data[channel].waveform.begin(), _per_channel_data[channel].waveform.begin() + _config.n_baseline_samples, 0.0) 
        / _config.n_baseline_samples;
      
    _per_channel_data[channel].max = max;
      
    // calculate FFTs
    _fft_manager.Execute();
    int adc_fft_size = _fft_manager.OutputSize();
    for (int i = 0; i < adc_fft_size; i++) {
      _per_channel_data[channel].fft_real.push_back(_fft_manager.ReOutputAt(i));
      _per_channel_data[channel].fft_imag.push_back(_fft_manager.ImOutputAt(i));
    } 

    // get Peaks
    PeakFinder peaks(_per_channel_data[channel].waveform, _per_channel_data[channel].baseline, _config.n_smoothing_samples);
    _per_channel_data[channel].peaks.assign(peaks.Peaks()->begin(), peaks.Peaks()->end());
  }

}
