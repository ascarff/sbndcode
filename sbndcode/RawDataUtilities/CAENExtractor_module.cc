/**
 * Extract and plot CAEN waveforms (for test stand checks)
 *
 * This module creates an output ROOT file "wf.root" with a bunch of
 * TGraphs (for every channel) and TCanvases (one for every event with all
 * the channel waveforms plotted).
 *
 * A. Mastbaum <mastbaum@uchicago.edu>, 2016/11/11
 */

/// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "fhiclcpp/ParameterSet.h"

// LArSoft includes
#include "lardata/RawData/AuxDetDigit.h"
#include "lardata/RawData/RawDigit.h"
#include "larcore/SimpleTypesAndConstants/RawTypes.h"

// ROOT includes
#include "TFile.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TAxis.h"

// C++ includes
#include <vector>
#include <iostream>
#include <sstream>

namespace sbnd {

  class CAENExtractor : public art::EDAnalyzer {
    public:
      explicit CAENExtractor(fhicl::ParameterSet const& pset);
      ~CAENExtractor();

      void beginJob() {}
      void analyze(const art::Event& evt);
      void beginSubRun (const art::SubRun& subrun) {}
      void reset() {}

    private:
      unsigned fID;
      std::string fDAQModuleLabel;
      TFile* f;
  };


  CAENExtractor::CAENExtractor(fhicl::ParameterSet const& pset)
      : EDAnalyzer(pset)
      , fID(0)
      , fDAQModuleLabel(pset.get<std::string>("DAQModuleLabel")) {
    // FIXME: Use the TFileService
    f = new TFile("wfs.root", "recreate");
  }


  CAENExtractor::~CAENExtractor() {
    f->Close();
  }


  void CAENExtractor::analyze(const art::Event& evt) {
    std::vector<const raw::AuxDetDigit*> auxDigits;
    evt.getView(fDAQModuleLabel, auxDigits);

    f->cd();
    std::ostringstream oss;
    oss << "c_" << fID++;
    TCanvas c(oss.str().c_str(), "", 800, 800);
    c.Divide(2, 16);
    std::vector<TGraph*> graphs;

    for (auto ad : auxDigits) {
      std::cout << "------" << std::endl
                << "AuxDetName: " << ad->AuxDetName() << std::endl
                << "Channel: " << ad->Channel() << std::endl
                << "NADC: " << ad->NADC() << std::endl
                << "ADC[0]: " << ad->ADC(0) << std::endl
                << "TimeStamp: " << ad->TimeStamp() << std::endl;

      int board = (ad->AuxDetName() == "board0") ? 0 : 1;
      int pad = board * 16 + ad->Channel() + 1;
      std::cout << "pad " << pad << std::endl;

      TGraph* g = new TGraph(ad->NADC());
      std::ostringstream oss2;
      oss2 << "Board " << board
           << ", channel " << ad->Channel() << ";Sample;ADC";
      g->SetTitle(oss2.str().c_str());
      for (size_t i=0; i<ad->NADC(); i++) {
        g->SetPoint(i, i, ad->ADC(i));
      }
      c.cd(pad);
      g->Draw("al");
      g->GetYaxis()->SetRangeUser(0, 0x3fff);

      std::ostringstream oss3;
      oss3 << "g_b" << board << "c" << ad->Channel();
      g->SetTitle(oss2.str().c_str());
      g->SetName(oss3.str().c_str());
      g->Write();
      graphs.push_back(g);
    }

    c.Update();
    c.Write();
  }

  DEFINE_ART_MODULE(CAENExtractor)

}  // namespace sbnd

