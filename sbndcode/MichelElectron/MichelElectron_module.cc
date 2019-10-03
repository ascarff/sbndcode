////////////////////////////////////////////////////////////////////////
// Class:       MichelElectron
// Plugin Type: analyzer (art v3_02_06)
// File:        MichelElectron_module.cc
//
// Generated at Thu Sep 19 12:15:48 2019 by Iker De-icaza-astiz using cetskelgen
// from cetlib version v3_07_02.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art_root_io/TFileService.h"

#include "larcore/Geometry/Geometry.h"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardataobj/RecoBase/OpHit.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"

#include "TTree.h"
#include "TFile.h"
#include "TNtuple.h"
#include "TROOT.h"
#include "TLegend.h"
#include "TCanvas.h"
#include "TH1D.h"
#include <string>

#include "sbndPDMapAlg.h"


class MichelElectron;


class MichelElectron : public art::EDAnalyzer {
public:
  explicit MichelElectron(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  MichelElectron(MichelElectron const&) = delete;
  MichelElectron(MichelElectron&&) = delete;
  MichelElectron& operator=(MichelElectron const&) = delete;
  MichelElectron& operator=(MichelElectron&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;
  opdet::sbndPDMapAlg map; //map for photon detector types
  
  // Selected optional functions.
  void beginJob() override;
  void endJob() override;
  void reconfigure(fhicl::ParameterSet const & p);

private:

  // Declare member data here.

  /******************************************
   *  TREES                                 *
   ******************************************/
  TTree * opt_tree ;

  /******************************************
   *  DATA MEMBERS                          *
   ******************************************/
  int event_id;
  std::stringstream histname ; 
  raw::Channel_t fChNumber ;
  raw::TimeStamp_t fStartTime , fEndTime ;

  double fSampling = 0.5 ;
  TCanvas *c = new TCanvas();
  TLegend *l = new TLegend( 0.58, 0.68, 0.88, 0.88 );
  TH1D * pmt_wvfHist ;
  TH1D * arapuca_wvfHist ;
};


MichelElectron::MichelElectron(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
  
  this->reconfigure(p);
}

void MichelElectron::analyze(art::Event const& e)
{
  // Implementation of required member function here.

  event_id = e.id().event();
  
  std::cout<<  " Processing event # " << event_id << std::endl;
  art::ServiceHandle<art::TFileService> tfs;

  if( !e.isRealData()){
    // grab a data productfrom the event
    auto const& wvFormsHandle = *e.getValidHandle<std::vector<raw::OpDetWaveform>>("opdaq");

    fStartTime = 0.;
    fEndTime = 23.;

    // Issues defining these two histograms
    pmt_wvfHist = new TH1D("pmt_wvfHist", TString::Format(";t - %f (#mus);",fStartTime), 6000, fStartTime, fEndTime);
    arapuca_wvfHist = new TH1D("arapuca_wvfHist", TString::Format(";t - %f (#mus);",fStartTime), 6000, fStartTime, fEndTime);

    double threshold = 7980;
    int hist_id = 0;
    for(auto const& wvf : wvFormsHandle){
      fChNumber = wvf.ChannelNumber();
      histname.str(std::string());
      histname << "event_" << event_id
               << "_opchannel_" << fChNumber
               << "_" << hist_id;
      // fStartTime = wvf.TimeStamp()/1000.0; //in us
      // fEndTime = double(wvf.size())/fSampling + fStartTime;
      // fEndTime = fEndTime/1000; //in us

      //Create a new histogram
      TH1D *wvfHist = tfs->make< TH1D >(histname.str().c_str(), TString::Format(";t - %f (#mus);",fStartTime), 6000, fStartTime, fEndTime);

      for(unsigned int i=0; i<wvf.size();i++){
        wvfHist->SetBinContent(i+1,(double)wvf[i]);
      }

      // Adding up the pmt and arapuca histograms
      if(map.pdType(fChNumber, "pmt") || map.pdType(fChNumber, "barepmt")){
        if (threshold > wvfHist->GetBinContent(wvfHist->GetMinimumBin())){
          pmt_wvfHist->Add(wvfHist);
        }
      }
      if(map.pdType(fChNumber, "arapucaT1") || map.pdType(fChNumber, "arapucaT2") || map.pdType(fChNumber, "xarapucaprime") || map.pdType(fChNumber, "xarapuca")){
        arapuca_wvfHist->Add(wvfHist);
      }
      hist_id++;
    }
    //auto const& wforms = wvFormsHandle.at(i);

  }
}

void MichelElectron::beginJob()
{
  // Implementation of optional member function here.

  event_id = -999;
  
  // Define output tree structure here
  opt_tree   = new TTree( "opt_tree",    "Opt tree: General information about the light information in the event" ) ;
  
  opt_tree -> Branch( "event_id",     & event_id,     "event_id/I" ) ;

  opt_tree -> SetDirectory(0);
}

void MichelElectron::endJob()
{
  // Implementation of optional member function here.
  // store tree
  TFile f("output_wftree.root", "RECREATE");
  opt_tree -> Write() ;
  f.Write();
  f.Close();

  arapuca_wvfHist -> GetXaxis() -> SetTitle(" t[us]" ) ;
  arapuca_wvfHist -> Draw("HIST");
  c->SaveAs("arapuca_total_wvfHist.root") ;
  l->Clear();
  c->Clear();
  pmt_wvfHist -> GetXaxis() -> SetTitle(" t[us]" ) ;
  pmt_wvfHist -> Draw("HIST");
  c->SaveAs("pmt_total_wvfHist.root") ;
  l->Clear();
  c->Clear();
}

void MichelElectron::reconfigure(fhicl::ParameterSet const & p)
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(MichelElectron)
