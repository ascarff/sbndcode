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
#include "lardataobj/RawData/RawDigit.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardataobj/RecoBase/Hit.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"

//Root Includes                                                                                                                                                       
#include "TFile.h"
#include "TMath.h"
#include "TTree.h"
#include "TGraph.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TH1D.h"
#include "TMultiGraph.h"

//C++ Includes                                                                                                                                                 
#include <vector>
#include <iostream>

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
  //std::vector<double> fThresholds

  int event_num=0;
  TFile *file = new TFile("test.root","RECREATE");

};

evd::BadEventDisplay::BadEventDisplay(const fhicl::ParameterSet& pset) : EDAnalyzer(pset){

  fHitsModuleLabel     = pset.get<std::string>("HitsModuleLabel");
  //  fThresholds          = pset.get<std::vector<double>("Thresholds"); 


}

void evd::BadEventDisplay::analyze(const art::Event& evt) {

  event_num++;

  //Get the Geometry
  art::ServiceHandle<geo::Geometry> geom;

  int divisions=0;
  //Create the Graphs
  std::map<geo::PlaneID,TGraph*> Graph_map;
  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
    Graph_map[plane_id] = new TGraph(0);
    divisions++;
  }

  // Getting the Hit Information                                                                                                                                             
  art::Handle<std::vector<recob::Hit> > hitListHandle;
  std::vector<art::Ptr<recob::Hit> > hits;
  if(evt.getByLabel(fHitsModuleLabel,hitListHandle))
    {art::fill_ptr_vector(hits, hitListHandle);}

  //Fill in PeakTime vs Wire Number 
  for(std::vector<art::Ptr<recob::Hit> >::iterator hit_iter=hits.begin(); hit_iter!=hits.end(); ++hit_iter){

    //Get the PeakTime
    int PeakTime = (*hit_iter)->PeakTime();

    //Get the plane ID 
    geo::WireID wireID = (*hit_iter)->WireID();
    geo::PlaneID planeID = wireID.planeID(); 
    
    Graph_map[planeID]->SetPoint(Graph_map[planeID]->GetN(),wireID.Wire, PeakTime);    
    std::cout << "Point: " << Graph_map[planeID]->GetN() << " has wire: " << wireID.Wire << " PeakTime: " << PeakTime << std::endl;
  }


  //Write to Canvas
  std::string evd_string;
  std::stringstream sstm_evd;
  sstm_evd << "Event " << event_num ;
  evd_string = sstm_evd.str();
  const char* evd_name = evd_string.c_str();
  TCanvas* evd_test = new TCanvas(evd_name,evd_name,900,600);
  //  TCanvas* evd = tfs->makeAndRegister<TCanvas>(evd_name,"");
  //evd->cd();
  evd_test->Divide(1,divisions);

  int division=0; 
  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
    ++division;
    evd_test->cd(division);
    Graph_map[plane_id]->Draw();
    Graph_map[plane_id]->Write();
    Graph_map[plane_id]->Delete();
  }

  evd_test->Update();
  delete evd_test;

}

DEFINE_ART_MODULE(evd::BadEventDisplay)
