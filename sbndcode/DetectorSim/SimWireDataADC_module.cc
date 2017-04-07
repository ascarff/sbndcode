////////////////////////////////////////////////////////////////////////
// Class:       SimWireDataADC
// Module Type: producer
// File:        SimWireDataADC_module.cc
//
// This module uses a transfer function (voltage to ADC count mapping)
// measured in data to distort the existing MC raw digits from SimWire.
//
// Author: A. Mastbaum <mastbaum@uchicago.edu>, 2017/03/10
//
// Generated at Fri Mar 10 400:37:59 2017 by Andrew Mastbaum using artmod
// from cetpkgsupport v1_10_01.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardata/RawData/RawDigit.h"
#include "lardata/RawData/raw.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "larsim/RandomUtils/LArSeedService.h"

#include <TFile.h>
#include <TH2S.h>
#include <TH1D.h>

#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/RandGaussQ.h"

#include <memory>
#include <vector>
#include <cstdlib>

namespace detsim {
  class SimWireDataADC;
}

class detsim::SimWireDataADC : public art::EDProducer {
public:
  explicit SimWireDataADC(fhicl::ParameterSet const & p);
  ~SimWireDataADC();

  SimWireDataADC(SimWireDataADC const &) = delete;
  SimWireDataADC(SimWireDataADC &&) = delete;
  SimWireDataADC & operator = (SimWireDataADC const &) = delete;
  SimWireDataADC & operator = (SimWireDataADC &&) = delete;

  void produce(art::Event & e) override;

  void beginJob() override {}
  void reconfigure(fhicl::ParameterSet const & p) override;

private:
  // ROOT file containing response maps
  std::string fResponseFilePath;
  TFile* fResponseFile;

  // 1D histograms of response for a given true ADC input, for a given channel,
  // centered at zero.
  std::vector<std::vector<TH1D*> > response;

  // The offset for a given true ADC for a given channel, to shift the response
  // histogram to its true value.
  std::vector<std::vector<short> > offset;

  // An ordered list of the "chips" in the detector, used to look up the
  // response for a channel.
  std::vector<size_t> chips;

  std::string fDAQModuleLabel;  //!< Label for the RawDigits generator
  size_t fNChips;  //!< Number of chips for which we have data
  size_t fChPerChip;  //!< Number of channels per chip
  size_t fADCMax;  //!< Max ADC value + 1 (4096 for 12 bits)
  size_t fNTicks;  //!< Number of time ticks in event waveforms

  // Shift the mode value of the ADC response to the ideal value (after
  // applying stuck code removal)
  bool fDoPerfectCalibration;
};


detsim::SimWireDataADC::SimWireDataADC(fhicl::ParameterSet const& p) {
  this->reconfigure(p);

  // Load the ADC response maps from a ROOT file, which is built from
  // testing data.
  //
  // We read two things: a 2D histogram, which is an ADC distribution versus
  // "true" ADC after linear slope/offset calibration shifted to zero, and
  // a bin-by-bin offset. (This is to avoid a sparse but gigantic 2D response
  // histogram, and implies a cutoff in the width of the distribution).
  std::cout << "Loading response histograms ";
  std::cout.flush();

  fResponseFile = TFile::Open(fResponseFilePath.c_str());  // FIXME IFDH fetch
  assert(fResponseFile && fResponseFile->IsOpen());

  size_t nch = fNChips * fChPerChip;
  response.resize(nch);
  offset.resize(nch);

  for (size_t ichip=0; ichip<fNChips; ichip++) {
    for (size_t ichan=0; ichan<fChPerChip; ichan++) {
      std::cout << ".";
      std::cout.flush();

      size_t i = fChPerChip * ichip + ichan;
      char hname[50];

      snprintf(hname, 50, "hs_%05lu", i);
      TH2S* hs = (TH2S*) fResponseFile->Get(hname);
      assert(hs);

      snprintf(hname, 50, "ho_%05lu", i);
      TH1S* ho = (TH1S*) fResponseFile->Get(hname);
      assert(ho);

      response[i].resize(fADCMax);
      offset[i].resize(fADCMax);
      for (size_t j=0; j<fADCMax; j++) {
        char hpname[50];
        snprintf(hpname, 50, "h_%05lu_%04lu", i, j);
        response[i][j] = hs->ProjectionY(hpname, j, j);
        offset[i][j] = ho->GetBinContent(j);
      }
    }
  }

  std::cout << " done." << std::endl;

  // Randomly distribute what limited chips we have throughout the detector
  // (keeping blocks of channels on the same chip together)
  art::ServiceHandle<geo::Geometry> geoService;
  size_t chips_needed = geoService->Nchannels() / fChPerChip + 1;
  for (size_t i=0; i<chips_needed; i++) {
    chips.push_back(CLHEP::RandFlat::shootInt(fNChips));
  }

  // Number of time ticks
  auto const* detprop = \
    lar::providerFrom<detinfo::DetectorPropertiesService>();
  fNTicks = detprop->NumberTimeSamples();

  produces<std::vector<raw::RawDigit> >();
}


detsim::SimWireDataADC::~SimWireDataADC() {
  fResponseFile->Close();
}


void detsim::SimWireDataADC::reconfigure(fhicl::ParameterSet const& p) {
  fDAQModuleLabel = p.get<std::string>("DAQModuleLabel", "daq");
  fNChips = p.get<int>("NChips", 11);
  fChPerChip = p.get<int>("ChPerChip", 16);
  fADCMax = p.get<int>("ADCMax", 4096);
  fDoPerfectCalibration = p.get<bool>("DoPerfectCalibration", false);
  fResponseFilePath = p.get<std::string>("ResponseFilePath");
}


void detsim::SimWireDataADC::produce(art::Event& e) {
  art::ServiceHandle<geo::Geometry> geoService;

  std::vector<const raw::RawDigit*> rawDigitHandle;
  e.getView(fDAQModuleLabel, rawDigitHandle);

  std::unique_ptr<std::vector<raw::RawDigit> > dig(new std::vector<raw::RawDigit>);

  // Create "event display" histograms for each plane, named with the cryostat,
  // TPC, and plane IDs.
  art::ServiceHandle<art::TFileService> tfs;
  TFile& f = tfs->file();

  std::vector<std::vector<std::vector<TH2S> > > hevd_adc(geoService->Ncryostats());
  std::vector<std::vector<std::vector<TH2S> > > hevd(geoService->Ncryostats());

  for (size_t i=0; i<geoService->Ncryostats(); i++) {
    hevd_adc[i].resize(geoService->NTPC(i));
    hevd[i].resize(geoService->NTPC(i));
    for (size_t j=0; j<geoService->NTPC(i); j++) {
      hevd_adc[i][j].resize(geoService->Nplanes(i));
      hevd[i][j].resize(geoService->Nplanes(i));
      for (size_t k=0; k<geoService->Nplanes(j, i); k++) {
        unsigned int nwires = geoService->Nwires(k, j, i);
        char htname[50];
        snprintf(htname, 50, "hevd_adcd_%lu_%lu_%lu", i, j, k);
        hevd_adc[i][j][k] = \
          TH2S(htname, ";Wire;Tick;ADC", nwires, 0, nwires, fNTicks, 0, fNTicks);
        snprintf(htname, 50, "hevd_orig_%lu_%lu_%lu", i, j, k);
        hevd[i][j][k] = \
          TH2S(htname, ";Wire;Tick;ADC", nwires, 0, nwires, fNTicks, 0, fNTicks);
      }
    }
  }

  // Loop over RawDigits for each channel
  for (auto const rd : rawDigitHandle) {
    // Get an ADCvector_t of uncompressed RawDigits
    raw::RawDigit::ADCvector_t adcs(rd->Samples());
    raw::Uncompress(rd->ADCs(), adcs, rd->Compression());

    // Get data ADC channel response
    size_t chip = chips.at(rd->Channel() / fChPerChip);
    raw::ChannelID_t channel = \
      fChPerChip * chip + (rd->Channel() % fChPerChip);

    // Get wire ID within plane
    std::vector<geo::WireID> wire_ids = \
      geoService->ChannelToWire(rd->Channel());
    geo::WireID w = wire_ids.at(0);

    // Loop through samples and sample distorted ADCs
    size_t nstuck = 0;
    std::vector<short> adc(rd->NADC());
    for (size_t i=0; i<rd->NADC(); i++) {
      short j = rd->ADC(i);

      TH1D* hr = response[channel].at(j);
      short o = offset[channel].at(j);

      short v = hr->GetRandom() + o + 1;

      // Apply "stuck code" removal (TODO: Channel-by-channel lists)
      if ((v & 0x3f) < 2 || (v & 0x3f) == 0x3f || v < 130 || v > 4094) {
        adc[i] = -9999;
        nstuck++;
      }
      else {
        if (fDoPerfectCalibration) {
          v -= (o - j);
        }
        adc[i] = v;
      }

      hevd[w.Cryostat][w.TPC][w.Plane].SetBinContent(w.Wire+1, i+1, j);
      hevd_adc[w.Cryostat][w.TPC][w.Plane].SetBinContent(w.Wire+1, i+1, adc[i]);
    }

    // Print out a message for channels with a large fraction of bad samples
    float stuck_fraction = 1.0 * nstuck / fNTicks;
    if (stuck_fraction > 0.5) {
      std::cout << "Stuck channel "
                << "(" << 100.0 * stuck_fraction << "%):"
                << " ch" << rd->Channel() << ", "
                << "data " << chip << "/" << channel % fChPerChip
                << std::endl;
    }

    // Add distorted waveforms to the event. TODO: Add zero suppression here
    // to remove the flagged values from the event for downstream processing.
    auto compression = rd->Compression();
    raw::Compress(adc, compression);
    raw::RawDigit rds(rd->Channel(), adc.size(), adc, compression);
    rds.SetPedestal(rd->GetPedestal());
    dig->push_back(rds);
  }

  // Write out event display histograms
  f.cd();
  for (size_t i=0; i<geoService->Ncryostats(); i++) {
    for (size_t j=0; j<geoService->NTPC(i); j++) {
      for (size_t k=0; k<geoService->Nplanes(j, i); k++) {
        hevd_adc[i][j][k].Write();
        hevd[i][j][k].Write();
      }
    }
  }

  e.put(std::move(dig));
}


DEFINE_ART_MODULE(detsim::SimWireDataADC)

