// Framework includes                                                                                                                                                             
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/View.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

//Larsoft includes                                                                                                                                                                  
#include "larcore/Geometry/Geometry.h"
//#include "larcorealg/Geometry/GeometryCore.h"
#include "lardataobj/RawData/RawDigit.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardataobj/RecoBase/Hit.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"

//Root Includes                                                                                                                                                       
#include "TFile.h"
#include "TMath.h"
#include "TTree.h"
#include "TBranch.h"
#include "TGraph.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TH1D.h"
#include "TMultiGraph.h"

//C++ Includes                                                                                                                                                 
#include <vector>
#include <iostream>

//Online Anlysis Includes 

 #include "sbndcode/VSTAnalysis/PeakFinder.hh"
// #ifdef __MAKECINT__ 
// #pragma link off all globals;
// #pragma link off all classes;
// #pragma link off all structs;
// #pragma link off all functions;
// #pragma link C++ class PeakFinder+;
// #pragma link C++ class PeakFinder::Peak+;
// #pragma link C++ class vector<PeakFinder::Peak>+;
// #endif


namespace evd {
  class BadEventDisplay;
}

class evd::BadEventDisplay: public art::EDAnalyzer {
public:

  BadEventDisplay(const fhicl::ParameterSet& pset);

  void analyze(const art::Event& evt);
  //  void endJob();
  //void beginJob();

private:

  art::ServiceHandle<art::TFileService> tfs;
  std::string fHitsModuleLabel;
  bool fReadOnlineFile;
  bool fVerbose;
  std::vector<double> fThresholds;

  int event_num=0;
};

evd::BadEventDisplay::BadEventDisplay(const fhicl::ParameterSet& pset) : EDAnalyzer(pset){

  fHitsModuleLabel     = pset.get<std::string>("HitsModuleLabel");
  fThresholds          = pset.get<std::vector<double> >("Thresholds"); 
  fReadOnlineFile      = pset.get<bool>("ReadOnlineFile");
  fVerbose      = pset.get<bool>("Verbose");

}

void evd::BadEventDisplay::analyze(const art::Event& evt) {

  //  gInterpreter->GenerateDictionary("vector<PeakFinder::Peak>","vector");
  //gInterpreter->GenerateDictionary("PeakFinder::Peak","sbndcode/VSTAnalysis/PeakFinder.hh");
  event_num++;

  //Get the Geometry
  art::ServiceHandle<geo::Geometry> geom;

    int divisions=0;

  //Create the Graphs       
  std::map<geo::PlaneID,TGraph*> Graph_map;
  std::map<geo::PlaneID,TGraph*> Graph_map_daq;
  std::map<geo::PlaneID,TMultiGraph*> Graph_map_multi;
  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
    Graph_map[plane_id] = new TGraph(0);                                                                                                                                           
    Graph_map_daq[plane_id]= new TGraph(0);
    Graph_map_multi[plane_id] = new TMultiGraph();
    divisions++;
  }


  // Get The raw digits
  int daq_hits=0;
  auto const& raw_digits_handle = evt.getValidHandle<std::vector<raw::RawDigit>>("daq");
  for (auto const& digits: *raw_digits_handle) {

    auto adv_vec = digits.ADCs();
    raw::ChannelID_t  channel = digits.Channel(); 
    float pedestal = digits.GetPedestal();
    std::vector<geo::WireID> Wire = geom->ChannelToWire(channel);

    //sbnd is one channel per wire so the vector should be size one.          
    geo::PlaneID  PlaneID = (Wire[0]).planeID();

    int time=0;
    for (unsigned i = 0; i < digits.NADC(); i ++) {
      ++time;
      int16_t adc = adv_vec[i];
      if(TMath::Abs(adc-pedestal)>fThresholds[0] && (Wire[0]).Plane==0){
	++daq_hits;
	if(fVerbose == true){
	std::cout << "adc: " << adc << " ped" << pedestal << " value: " << adc-pedestal << std::endl;
	}
  	Graph_map_daq[PlaneID]->SetPoint(Graph_map_daq[PlaneID]->GetN(),Wire[0].Wire, time );
      }
      else if(TMath::Abs(adc-pedestal)>fThresholds[1] && (Wire[0]).Plane==1){
	++daq_hits;
	if(fVerbose == true){
	std::cout << "adc: " <<adc << " ped" << pedestal << " value: "<< adc-pedestal<< std::endl;
	}
        Graph_map_daq[PlaneID]->SetPoint(Graph_map_daq[PlaneID]->GetN(),Wire[0].Wire, time );
      }
    }
  }


  std::cout << "daq hits size: " << daq_hits << std::endl; 

  // Getting the Hit Information 
  art::Handle<std::vector<recob::Hit> > hitListHandle;
  std::vector<art::Ptr<recob::Hit> > hits;
  if(evt.getByLabel(fHitsModuleLabel,hitListHandle))
    {art::fill_ptr_vector(hits, hitListHandle);}
  std::cout << "hits size: " << hits.size() << std::endl;


  //  Fill in PeakTime vs Wire Number 
  for(std::vector<art::Ptr<recob::Hit> >::iterator hit_iter=hits.begin(); hit_iter!=hits.end(); ++hit_iter){

    //Get the PeakTime
    int PeakTime = (*hit_iter)->PeakTime();

    //Get the plane ID 
    geo::WireID wireID = (*hit_iter)->WireID();
    geo::PlaneID planeID = wireID.planeID(); 


    
    Graph_map[planeID]->SetPoint(Graph_map[planeID]->GetN(),wireID.Wire, PeakTime);    
    std::cout << "Point: " << Graph_map[planeID]->GetN() << " has wire: " << wireID.Wire << " PlaneID: " << planeID  << " PeakTime: " << PeakTime << std::endl;
  }

  if(fReadOnlineFile == true){

    TFile *f = new TFile("out.root");
    TTree *t = (TTree*)f->Get("VSTAnalysis/event");

    std::vector<PeakFinder::Peak> * peaks;
    std::cout<< "peaks size: " << peaks->size() << std::endl;
    std::cout << "test" << std::endl;
    t->Branch("channel_data.peaks",&peaks,32000,0);

    //TBranch *b_channel_data_peaks = t->GetBranch("channel_data.peaks");
    //TBranch *b_channel_data_peaks;
    //    t->SetBranchAddress("channel_data.peaks",&peaks);
    
    Long64_t nentries = t->GetEntries();
    for (Long64_t i=0;i<nentries;i++) {
      t->GetEntry(i);
      std::cout << "peaks size: " << peaks->size() << std::endl;

      for(int i=0; i<10; ++i){
	std::cout << "amplitude: " << (*peaks)[i].amplitude << std::endl;
      }
    }

  }


  //Write to Canvas
  std::string evd_string;
  std::stringstream sstm_evd;
  sstm_evd << "Event " << event_num ;
  evd_string = sstm_evd.str();
  const char* evd_name = evd_string.c_str();
  TCanvas* evd = tfs->makeAndRegister<TCanvas>(evd_name,"");

  std::cout << "divisions: " << divisions << std::endl;
  evd->Divide(1,divisions);

  int division=0; 
  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
    
    Graph_map[plane_id]->SetMarkerColor(2);//Red
    Graph_map[plane_id]->SetMarkerStyle(8);
    Graph_map_daq[plane_id]->SetMarkerColor(4);//Blue
    Graph_map_daq[plane_id]->SetMarkerStyle(8);
    
    //    Add graph to the mutligraph                                                              
    if(hits.size() > 0 && evt.getByLabel(fHitsModuleLabel,hitListHandle) && daq_hits >0){
      Graph_map_multi[plane_id]->Add(Graph_map[plane_id]);
      Graph_map_multi[plane_id]->Add(Graph_map_daq[plane_id]);
    }
    else if(daq_hits >0){
      Graph_map_multi[plane_id]->Add(Graph_map_daq[plane_id]);
    }
    
    ++division;
    evd->cd(division);
    Graph_map_multi[plane_id]->Draw("ap");
    
    //Graph_map[plane_id]->Delete();
   }

}

DEFINE_ART_MODULE(evd::BadEventDisplay)
