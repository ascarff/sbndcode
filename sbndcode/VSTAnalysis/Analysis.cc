//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <numeric>
#include <getopt.h>
#include <chrono>
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

#include "Analysis.hh"
#include "ChannelData.hh"
#include "ChannelMap.hh"
#include "HeaderData.hh"
#include "FFT.hh"
#include "Noise.hh"
#include "PeakFinder.hh"
#include "Mode.hh"

using namespace daqAnalysis;

Analysis::Analysis(fhicl::ParameterSet const & p) :
  _config(p),
  _per_channel_data(ChannelMap::n_wire),
  _per_channel_data_reduced((_config.reduce_data) ? ChannelMap::n_wire : 0), // setup reduced event vector if we need it
  _noise_samples(ChannelMap::n_wire),
  _header_data(std::max(_config.n_headers,0)),
  _thresholds( (_config.threshold_calc == 3) ? ChannelMap::n_wire : 0),
  _fem_summed_waveforms((_config.sum_waveforms) ? ChannelMap::NFEM() : 0),
  _fft_manager(  (_config.static_input_size > 0) ? _config.static_input_size: 0),
  _analyzed(false)
{
  _event_ind = 0;
}

Analysis::AnalysisConfig::AnalysisConfig(const fhicl::ParameterSet &param) {
  // set up config

  // conversion of frame number to time (currently unused)
  frame_to_dt = param.get<float>("frame_to_dt", 1.6e-3 /* units of seconds */);
  // whether to print stuff
  verbose = param.get<bool>("verbose", false);
  // number of events to take in before exiting
  // will never exit if set to negative
  // Also--currently does nothing.
  n_events = param.get<unsigned>("n_events", -1);

  // configuring analysis code:

  // thresholds for peak finding
  // 0 == use static threshold
  // 1 == use gauss fitter rms
  // 2 == use raw rms
  // 3 == use rolling average of rms
  threshold_calc = param.get<unsigned>("threshold_calc", 0);
  threshold_sigma = param.get<float>("threshold_sigma", 5.);
  threshold = param.get<float>("threshold", 100);

  // determine method to get noise sample
  // 0 == use first `n_baseline_samples`
  // 1 == use peakfinding
  noise_range_sampling = param.get<unsigned>("noise_range_sampling",0);

  // whether to use plane data in peakfinding
  use_planes = param.get<bool>("use_planes", false);

  // method to calculate baseline:
  // 0 == assume baseline is 0
  // 1 == assume baseline is in digits.GetPedestal()
  // 2 == use mode finding to get baseline
  baseline_calc = param.get<unsigned>("baseline_calc", 1);
 
  // only used if noise_range_sampling == 0
  // number of samples in noise sample
  n_noise_samples = param.get<unsigned>("n_noise_samples", 20);

  // number of samples to average in each direction for peak finding
  n_smoothing_samples = param.get<unsigned>("n_smoothing_samples", 1);
  // number of consecutive samples that must be above threshold to count as a peak
  n_above_threshold = param.get<unsigned>("n_above_threshold", 1);

  // Number of input adc counts per waveform. Set to negative if unknown.
  // Setting to some positive number will speed up FFT's.
  static_input_size = param.get<int>("static_input_size", -1);
  // how many headers to expect (set to negative if don't process) 
  n_headers = param.get<int>("n_headers", -1);

  // whether to calculate/save certain things
  sum_waveforms = param.get<bool>("sum_waveforms", false);
  fft_per_channel = param.get<bool>("fft_per_channel", false);
  reduce_data = param.get<bool>("reduce_data", false);
  timing = param.get<bool>("timing", false);

  // name of producer of raw::RawDigits
  std::string producer = param.get<std::string>("producer_name");
  daq_tag = art::InputTag(producer, ""); 
}

void Analysis::AnalyzeEvent(art::Event const & event) {
  //if (_config.n_events >= 0 && _event_ind >= (unsigned)_config.n_events) return false;

  _event_ind ++;

  // clear out containers from last iter
  for (unsigned i = 0; i < ChannelMap::n_wire; i++) {
    _per_channel_data[i].waveform.clear();
    _per_channel_data[i].fft_real.clear();
    _per_channel_data[i].fft_imag.clear();
    _per_channel_data[i].peaks.clear();
  }
  // also for summed waveforms
  if (_config.sum_waveforms) {
    for (unsigned i = 0; i < ChannelMap::NFEM(); i++) {
      _fem_summed_waveforms[i].clear();
    }
  }
  _analyzed = true;

  auto const& raw_digits_handle = event.getValidHandle<std::vector<raw::RawDigit>>(_config.daq_tag);
  
  // calculate per channel stuff
  for (auto const& digits: *raw_digits_handle) {
    ProcessChannel(digits);
  }

  if (_config.timing) {
    _timing.StartTime();
  }
  // make the reduced channel data stuff if need be
  if (_config.reduce_data) {
    for (size_t i = 0; i < _per_channel_data.size(); i++) {
      _per_channel_data_reduced[i] = daqAnalysis::ReducedChannelData(_per_channel_data[i]);
    }
  }
  if (_config.timing) {
    _timing.EndTime(&_timing.reduce_data);
  }

  if (_config.timing) {
    _timing.StartTime();
  }

  // now calculate stuff that depends on stuff between channels

  // DNoise
  for (unsigned i = 0; i < ChannelMap::n_wire - 1; i++) {
    unsigned next_channel = i + 1; 

    if (!_per_channel_data[i].empty && !_per_channel_data[next_channel].empty) {
      float unscaled_dnoise = _noise_samples[i].DNoise(
          _per_channel_data[i].waveform, _noise_samples[next_channel],  _per_channel_data[next_channel].waveform);
      // Doon't use same noise sample to scale dnoise
      // This should probably be ok, as long as the dnoise sample is large enough
      float dnoise_scale = sqrt(_per_channel_data[i].rms * _per_channel_data[i].rms + 
                                _per_channel_data[next_channel].rms * _per_channel_data[next_channel].rms);
      _per_channel_data[i].next_channel_dnoise = unscaled_dnoise / dnoise_scale; 
    }
  }
  if (_config.timing) {
    _timing.EndTime(&_timing.coherent_noise_calc);
  }
 
  // summed waveforms
  // TODO @INSTALLATION: Make sure this still works
  if (_config.sum_waveforms) {
    size_t n_fem = ChannelMap::NFEM();
    std::vector<std::vector<std::vector<int16_t> *>> channel_waveforms_per_fem(n_fem);
    // collect the waveforms
    for (unsigned i = 0; i < ChannelMap::n_wire; i++) {
      daqAnalysis::ChannelMap::readout_channel info = daqAnalysis::ChannelMap::Wire2Channel(i);
      size_t fem_ind = info.crate + info.slot * ChannelMap::n_fem_per_crate; 
      channel_waveforms_per_fem[fem_ind].push_back(&_per_channel_data[i].waveform);
    }
    // sum all of them
    for (unsigned i = 0; i < n_fem; i++) {
      daqAnalysis::SumWaveforms(_fem_summed_waveforms[i] ,channel_waveforms_per_fem[i]);
    }
  }

  if (_config.timing) {
    _timing.StartTime();
  }
  // deal with the header
  if (_config.n_headers > 0) {
    auto const &headers_handle = event.getValidHandle<std::vector<daqAnalysis::HeaderData>>(_config.daq_tag);
    for (auto const &header: *headers_handle) {
      ProcessHeader(header);
    }
  }
  if (_config.timing) {
    _timing.EndTime(&_timing.copy_headers);
  }
  // print stuff out
  if (_config.verbose) {
    std::cout << "EVENT NUMBER: " << _event_ind << std::endl;
    for (auto &channel_data: _per_channel_data) {
      std::cout << channel_data.Print();
    }
  }
  if (_config.timing) {
    _timing.Print();
  }
}

void Analysis::ProcessHeader(const daqAnalysis::HeaderData &header) {
  _header_data[header.Ind()] = header;
}

void Analysis::ProcessChannel(const raw::RawDigit &digits) {
  auto channel = digits.Channel();
  if (channel < ChannelMap::n_wire) {
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

    int16_t max = -INT16_MAX;
    int16_t min = INT16_MAX;
    auto adv_vec = digits.ADCs();
    if (_config.timing) {
      _timing.StartTime();
    }
    for (unsigned i = 0; i < digits.NADC(); i ++) {
      int16_t adc = adv_vec[i];
      if (adc > max) max = adc;
      if (adc < min) min = adc;
    
      // TODO: is it possible to do analysis w/out copying waveform?
      // fill up waveform
      _per_channel_data[channel].waveform.push_back(adc);
      if (_config.fft_per_channel) {
        // fill up fftw array
        double *input = _fft_manager.InputAt(i);
        *input = (double) adc;
      }
    }
    if (_config.timing) {
      _timing.EndTime(&_timing.fill_waveform);
    }
    if (_config.timing) {
      _timing.StartTime();
    }
    if (_config.baseline_calc == 0) {
      _per_channel_data[channel].baseline = 0;
    }
    else if (_config.baseline_calc == 1) {
      _per_channel_data[channel].baseline = digits.GetPedestal();
    }
    else if (_config.baseline_calc == 2) {
      _per_channel_data[channel].baseline = Mode(digits.ADCs());
    }
    if (_config.timing) {
      _timing.EndTime(&_timing.baseline_calc);
    }

    _per_channel_data[channel].max = max;
    _per_channel_data[channel].min = min;
      
    if (_config.timing) {
      _timing.StartTime();
    }
    // calculate FFTs
    if (_config.fft_per_channel) {
      _fft_manager.Execute();
      int adc_fft_size = _fft_manager.OutputSize();
      for (int i = 0; i < adc_fft_size; i++) {
        _per_channel_data[channel].fft_real.push_back(_fft_manager.ReOutputAt(i));
        _per_channel_data[channel].fft_imag.push_back(_fft_manager.ImOutputAt(i));
      } 
    }
    if (_config.timing) {
      _timing.EndTime(&_timing.execute_fft);
    }

    if (_config.timing) {
      _timing.StartTime();
    }
    // get thresholds 
    float threshold = _config.threshold;
    if (_config.threshold_calc == 0) {
      threshold = _config.threshold;
    }
    else if (_config.threshold_calc == 1) {
      auto thresholds = Threshold(_per_channel_data[channel].waveform, _per_channel_data[channel].baseline, _config.threshold_sigma, _config.verbose);
      threshold = thresholds.Val();
    }
    else if (_config.threshold_calc == 2) {
      NoiseSample temp({{0, (unsigned)digits.NADC()-1}}, _per_channel_data[channel].baseline);
      float raw_rms = temp.RMS(_per_channel_data[channel].waveform);
      threshold = raw_rms * _config.threshold_sigma;
    }
    else if (_config.threshold_calc == 3) {
      // if using plane data, make collection planes reach a higher threshold
      float n_sigma = _config.threshold_sigma;
      if (_config.use_planes && ChannelMap::PlaneType(channel) == 2) n_sigma = n_sigma * 1.5;

      threshold = _thresholds[channel].Threshold(_per_channel_data[channel].waveform, _per_channel_data[channel].baseline, n_sigma);
    }
    if (_config.timing) {
      _timing.EndTime(&_timing.calc_threshold);
    }

    _per_channel_data[channel].threshold = threshold;

    if (_config.timing) {
      _timing.StartTime();
    }
    // get Peaks
    unsigned peak_plane = (_config.use_planes) ? ChannelMap::PlaneType(channel) : 0;
    PeakFinder peaks(_per_channel_data[channel].waveform, _per_channel_data[channel].baseline, 
         threshold, _config.n_smoothing_samples, _config.n_above_threshold, peak_plane);
    _per_channel_data[channel].peaks.assign(peaks.Peaks()->begin(), peaks.Peaks()->end());

    if (_config.timing) {
      _timing.EndTime(&_timing.find_peaks);
    }

    if (_config.timing) {
      _timing.StartTime();
    }
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
    if (_config.timing) {
      _timing.EndTime(&_timing.calc_noise);
    }

    // register rms if using running threshold
    if (_config.threshold_calc == 3) {
      _thresholds[channel].AddRMS(_per_channel_data[channel].rms);
    }

    // calculate derived quantities
    _per_channel_data[channel].occupancy = _per_channel_data[channel].Occupancy();
    _per_channel_data[channel].mean_peak_height = _per_channel_data[channel].meanPeakHeight();
  }
}

bool Analysis::ReadyToProcess() {
  return _analyzed && !_per_channel_data[0].empty;
}

void Timing::StartTime() {
  start = std::chrono::high_resolution_clock::now(); 
}
void Timing::EndTime(float *field) {
  auto now = std::chrono::high_resolution_clock::now();
  *field += std::chrono::duration<float, std::milli>(now- start).count();
}
void Timing::Print() {
  std::cout << "FILL WAVEFORM: " << fill_waveform << std::endl;
  std::cout << "CALC BASELINE: " << baseline_calc << std::endl;
  std::cout << "FFT   EXECUTE: " << execute_fft << std::endl;
  std::cout << "CALC THRESHOLD " << calc_threshold << std::endl;
  std::cout << "CALC PEAKS   : " << find_peaks << std::endl;
  std::cout << "CALC NOISE   : " << calc_noise << std::endl;
  std::cout << "REDUCE DATA  : " << reduce_data << std::endl;
  std::cout << "COHERENT NOISE " << coherent_noise_calc << std::endl;
  std::cout << "COPY HEADERS : " << copy_headers << std::endl;
}


