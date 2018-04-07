//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <numeric>
#include <getopt.h>
#include <float.h>

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
#include "HeaderData.hh"
#include "FFT.hh"
#include "Noise.hh"
#include "PeakFinder.hh"
#include "Redis.hh"

using namespace daqAnalysis;

SimpleDaqAnalysis::SimpleDaqAnalysis(fhicl::ParameterSet const & p) :
  art::EDAnalyzer{p},
  _config(p),
  _per_channel_data(_config.n_channels),
  _noise_samples(_config.n_channels),
  _header_data(std::max(_config.n_headers,0))
{
  _event_ind = 0;
  art::ServiceHandle<art::TFileService> fs;

  // set up tree and the channel data branch for output
  _output = fs->make<TTree>("event", "event");
  _output->Branch("channel_data", &_per_channel_data);
  if (_config.n_headers > 0) {
    _output->Branch("header_data", &_header_data);
  }

  // subclasses to do FFT's and send stuff to Redis
  _fft_manager = (_config.static_input_size > 0) ? FFTManager(_config.static_input_size) : FFTManager();
  if (_config.redis) {
    auto stream_take = p.get<std::vector<unsigned>>("stream_take");
    auto stream_expire = p.get<std::vector<unsigned>>("stream_expire");
    int snapshot_time = p.get<int>("snapshot_time", -1);
    _redis_manager = new Redis(stream_take, stream_expire, snapshot_time);
  }
}

SimpleDaqAnalysis::AnalysisConfig::AnalysisConfig(const fhicl::ParameterSet &param) {
  // set up config

  // conversion of frame number to time (currently unused)
  frame_to_dt = param.get<double>("frame_to_dt", 1.6e-3 /* units of seconds */);
  // whether to print stuff
  verbose = param.get<bool>("verbose", false);
  // number of events to take in before exiting
  // will never exit if set to negative
  // Also--currently does nothing.
  n_events = param.get<unsigned>("n_events", -1);

  // configuring analysis code:

  // thresholds for peak finding
  threshold_hi = param.get<double>("threshold_hi", 100);
  threshold_lo = param.get<double>("threshold_lo", -1);

  // determine method to get noise sample
  // 0 == use first `n_baseline_samples`
  // 1 == use peakfinding
  noise_range_sampling = param.get<unsigned>("noise_range_sampling",0);

  // method to calculate baseline:
  // 0 == assume baseline is 0
  // 1 == assume baseline is in digits.GetPedestal()
  baseline_calc = param.get<unsigned>("baseline_calc", 1);
 
  // only used if noise_range_sampling == 0
  // number of samples in noise sample
  n_noise_samples = param.get<unsigned>("n_noise_samples", 20);

  // number of samples to average in each direction for peak finding
  n_smoothing_samples = param.get<unsigned>("n_smoothing_samples", 1);

  // Number of input adc counts per waveform. Set to negative if unknown.
  // Setting to some positive number will speed up FFT's.
  static_input_size = param.get<int>("static_input_size", -1);
  // whether to send stuff to redis
  redis = param.get<bool>("redis", false);
  // how many headers to expect (set to negative if don't process) 
  n_headers = param.get<int>("n_headers", -1);

  // number of input channels
  // TODO: how to detect this?
  n_channels = param.get<unsigned>("n_channels", 16 /* currently only the first 16 channels have data */);

  // name of producer of raw::RawDigits
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
  _noise_samples.clear();

  auto const& raw_digits_handle = event.getValidHandle<std::vector<raw::RawDigit>>(_config.daq_tag);
  
  // calculate per channel stuff
  for (auto const& digits: *raw_digits_handle) {
    ProcessChannel(digits);
  }

  // TODO: better cross-channel calculations

  // now calculate stuff that depends on stuff between channels
  for (unsigned i = 0; i < _config.n_channels; i++) {
    unsigned last_channel_ind = i == 0 ? _config.n_channels - 1 : i - 1;
    unsigned next_channel_ind = i == _config.n_channels - 1 ? 0 : i + 1;

    if (!_per_channel_data[i].empty && !_per_channel_data[last_channel_ind].empty) {
      // cross channel correlations
      _per_channel_data[i].last_channel_correlation = _noise_samples[i].Correlation(
          _per_channel_data[i].waveform, _noise_samples[last_channel_ind],  _per_channel_data[last_channel_ind].waveform);
      // cross channel sum RMS
      _per_channel_data[i].last_channel_sum_rms = _noise_samples[i].SumRMS(
        _per_channel_data[i].waveform, _noise_samples[last_channel_ind],  _per_channel_data[last_channel_ind].waveform);
    }

    if (!_per_channel_data[i].empty && !_per_channel_data[next_channel_ind].empty) {
      _per_channel_data[i].next_channel_correlation = _noise_samples[i].Correlation(
          _per_channel_data[i].waveform, _noise_samples[next_channel_ind],  _per_channel_data[next_channel_ind].waveform);
      _per_channel_data[i].next_channel_sum_rms = _noise_samples[i].SumRMS(
          _per_channel_data[i].waveform, _noise_samples[next_channel_ind],  _per_channel_data[next_channel_ind].waveform);
    }
  }

  // deal with the header
  if (_config.n_headers > 0) {
    auto const &headers_handle = event.getValidHandle<std::vector<daqAnalysis::HeaderData>>(_config.daq_tag);
    for (auto const &header: *headers_handle) {
      ProcessHeader(header);
    }
  }
  ReportEvent(event);
}

void SimpleDaqAnalysis::ProcessHeader(const daqAnalysis::HeaderData &header) {
  _header_data[header.Ind()] = header;
}

void SimpleDaqAnalysis::ReportEvent(art::Event const &art_event) {
  // Fill the output
  _output->Fill();

  // print stuff out
  if (_config.verbose) {
    std::cout << "EVENT NUMBER: " << _event_ind << std::endl;
    for (auto &channel_data: _per_channel_data) {
      std::cout << channel_data.JsonifyPretty();
    }
  }

  // Send stuff to Redis
  if (_config.redis) {
    _redis_manager->StartSend();
    _redis_manager->SendChannelData(&_per_channel_data, &_noise_samples);
    if (_config.n_headers > 0) {
      _redis_manager->SendHeaderData(&_header_data);
    }
    _redis_manager->FinishSend();
  }
}

void SimpleDaqAnalysis::ProcessChannel(const raw::RawDigit &digits) {
  auto channel = digits.Channel();
  /*
  std::cout << "CHANNEL: " << channel << std::endl;
  std::cout << "PEDESTAL: " << digits.GetPedestal() << std::endl;
  std::cout << "NADC: " << digits.NADC() << std::endl;
  if (digits.NADC() > 0) {
    std::cout << "ADC0: " << digits.ADCs()[0] << std::endl;
  }*/
  if (channel < _config.n_channels) {
    // handle empty events
    if (digits.NADC() == 0) {
      // default constructor handles empty event
      _per_channel_data[channel] = ChannelData(channel);
      _noise_samples[channel] = NoiseSample();
      return;
    }
   
    // if there are ADC's, the channel isn't empty
    _per_channel_data[channel].empty = false;
 
    // re-allocate FFT if necessary
    if (_fft_manager.InputSize() != digits.NADC()) {
      _fft_manager.Set(digits.NADC());
    }
   
    _per_channel_data[channel].channel_no = channel;

    double max = 0;
    double min = DBL_MAX;
    for (unsigned i = 0; i < digits.NADC(); i ++) {
      double adc = (double) digits.ADCs()[i];
      if (adc > max) max = adc;
      if (adc < min) min = adc;
    
      // fill up waveform
      _per_channel_data[channel].waveform.push_back(adc);
      // fill up fftw array
      double *input = _fft_manager.InputAt(i);
      *input = adc;
    }
    if (_config.baseline_calc == 0) {
      _per_channel_data[channel].baseline = 0.;
    }
    else if (_config.baseline_calc == 1) {
      _per_channel_data[channel].baseline = (double) digits.GetPedestal();
    }

    _per_channel_data[channel].max = max;
    _per_channel_data[channel].min = min;
      
    // calculate FFTs
    _fft_manager.Execute();
    int adc_fft_size = _fft_manager.OutputSize();
    for (int i = 0; i < adc_fft_size; i++) {
      _per_channel_data[channel].fft_real.push_back(_fft_manager.ReOutputAt(i));
      _per_channel_data[channel].fft_imag.push_back(_fft_manager.ImOutputAt(i));
    } 

    // get Peaks
    PeakFinder peaks(_per_channel_data[channel].waveform, _per_channel_data[channel].baseline, 
         _config.n_smoothing_samples, _config.threshold_hi, _config.threshold_lo);
    _per_channel_data[channel].peaks.assign(peaks.Peaks()->begin(), peaks.Peaks()->end());

    // get noise samples
    if (_config.noise_range_sampling == 0) {
      // use first n_noise_samples
      _noise_samples[channel] = NoiseSample( { { 0, _config.n_noise_samples -1 } }, _per_channel_data[channel].baseline);
    }
    else {
      // or use peak finding
      _noise_samples[channel] = NoiseSample(_per_channel_data[channel].peaks, _per_channel_data[channel].baseline, digits.NADC()); 
    }

    _per_channel_data[channel].rms = _noise_samples[channel].RMS(_per_channel_data[channel].waveform);
    _per_channel_data[channel].noise_ranges = *_noise_samples[channel].Ranges();
  }

}
