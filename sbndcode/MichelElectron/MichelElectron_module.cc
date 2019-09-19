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
#include "TCanvas.h"
#include "TH1D.h"
#include <string>

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
  TCanvas *c = new TCanvas();
  std::stringstream histname ; 
  raw::Channel_t fChNumber ;
  raw::TimeStamp_t fStartTime , fEndTime ;

  double fSampling = 0.5 ;
  TH1D * h_waveform ; 
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
  //  art::ServiceHandle<art::TFileService> tfs;

  if( !e.isRealData()){
    // grab a data productfrom the event
    auto const& wvFormsHandle = *e.getValidHandle<std::vector<raw::OpDetWaveform>>("opdaq");
 
    for(auto const& wvf : wvFormsHandle){
      fChNumber = wvf.ChannelNumber();
      histname.str(std::string());
      histname << "event_" << event_id 
	       << "_opchannel_" <<fChNumber;
        
      fStartTime = wvf.TimeStamp()/1000.0; //in us
      fEndTime = double(wvf.size())/fSampling + fStartTime;
      fEndTime = fEndTime/1000; //in us
      //Create a new histogram
      h_waveform = new TH1D(histname.str().c_str(), TString::Format(";t - %f (#mus);",fStartTime), wvf.size(), fStartTime, fEndTime);

      //TH1D *wvfHist = tfs->make< TH1D >(histname.str().c_str(), TString::Format(";t - %f (#mus);",fStartTime), wvf.size(), fStartTime, fEndTime);
      for(unsigned int i=0; i<wvf.size();i++){
	h_waveform->SetBinContent(i+1,(double)wvf[i]);
      }        
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

  h_waveform->GetXaxis()->SetTitle("t[mus]");
  h_waveform->Draw("hist");
  c->SaveAs("wf_event1.root") ; 
  c->Clear();
  event_id = -999;
}

void MichelElectron::reconfigure(fhicl::ParameterSet const & p)
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(MichelElectron)
