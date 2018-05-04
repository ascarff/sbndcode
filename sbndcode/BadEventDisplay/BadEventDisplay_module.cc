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
  std::vector<double> fThresholds;

  int event_num=0;
};

evd::BadEventDisplay::BadEventDisplay(const fhicl::ParameterSet& pset) : EDAnalyzer(pset){

  fHitsModuleLabel     = pset.get<std::string>("HitsModuleLabel");
  fThresholds          = pset.get<std::vector<double> >("Thresholds"); 


}

void evd::BadEventDisplay::analyze(const art::Event& evt) {


  event_num++;

  //Get the Geometry
  art::ServiceHandle<geo::Geometry> geom;

  int divisions=0;
  //Create the Graphs
  std::map<geo::PlaneID,TGraph*> Graph_map;
  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
    //Graph_map[plane_id] = new TGraph(0);
    std::string graph_string;
    std::stringstream sstm_graph;
    sstm_graph << plane_id << " Event: " << event_num << "RAW";
    graph_string = sstm_graph.str();
    const char* Graph_name = graph_string.c_str();
    std::cout << "Graph Name: " << Graph_name << std::endl; 
    Graph_map[plane_id]= tfs->makeAndRegister<TGraph>(Graph_name, Graph_name,0);
    divisions++;
  }


  // //Create the Graphs                                                                                                                                                                
  // std::map<geo::PlaneID,TGraph*> Graph_multi;
  // for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
  //   //Graph_map[plane_id] = new TGraph(0);                                                                                                                                           
  //   std::string graph_string;
  //   std::stringstream sstm_graph;
  //   sstm_graph << plane_id << " Event: " << event_num << " Multi";
  //   graph_string = sstm_graph.str();
  //   const char* Graph_name = graph_string.c_str();
  //   std::cout << "Graph Name: " << std::endl;
  //   Graph_map_multi[plane_id]= tfs->makeAndRegister<TMultiGraph>(Graph_name,"");
  //   divisions++;
  // }


  //Create the Graphs                                                                                                                                                                
  std::map<geo::PlaneID,TGraph*> Graph_map_daq;
  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
    //Graph_map[plane_id] = new TGraph(0);                                                                                                                                           
    std::string graph_string;
    std::stringstream sstm_graph;
    sstm_graph << plane_id << " Event: " << event_num << " DAQ";
    graph_string = sstm_graph.str();
    const char* Graph_name = graph_string.c_str();
    std::cout << "Graph Name: " <<  Graph_name << std::endl;
    Graph_map_daq[plane_id]= tfs->makeAndRegister<TGraph>(Graph_name, Graph_name,0);
    divisions++;
  }


  // std::cout << "################# Event: " << event_num << " #################" << std::endl; 
  
  //  Get The raw digits 
  int time=0;
  auto const& raw_digits_handle = evt.getValidHandle<std::vector<raw::RawDigit>>("daq");
  for (auto const& digits: *raw_digits_handle) {
    ++time;
    auto adv_vec = digits.ADCs();
    raw::ChannelID_t  channel = digits.Channel(); 
    float pedestal = digits.GetPedestal();
    std::vector<geo::WireID> Wire = geom->ChannelToWire(channel);

    //sbnd is one channel per wire so the vector should be size one.                                                                                                                
    geo::PlaneID  PlaneID = (Wire[0]).planeID();

    for (unsigned i = 0; i < digits.NADC(); i ++) {
      int16_t adc = adv_vec[i];
      if(TMath::Abs(adc-pedestal)>fThresholds[0] && (Wire[0]).Plane==0){
	std::cout << "Wire[0].Wire: " << Wire[0].Wire << " time: " << time << " adc: " << adc << std::endl;
	Graph_map_daq[PlaneID]->SetPoint(Graph_map_daq[PlaneID]->GetN(),Wire[0].Wire, time );
      }
      else if(TMath::Abs(adc-pedestal)>fThresholds[1] && (Wire[0]).Plane==1){
	std::cout << "Wire[0].Wire: " << Wire[0].Wire << " time: " << time << " adc: " << adc << std::endl;
        Graph_map_daq[PlaneID]->SetPoint(Graph_map_daq[PlaneID]->GetN(),Wire[0].Wire, time );
      }
    }
  }

  // Getting the Hit Information 
  art::Handle<std::vector<recob::Hit> > hitListHandle;
  std::vector<art::Ptr<recob::Hit> > hits;
  if(evt.getByLabel(fHitsModuleLabel,hitListHandle))
    {art::fill_ptr_vector(hits, hitListHandle);}
  std::cout << "hits size: " << hits.size() << std::endl;

  //Fill in PeakTime vs Wire Number 
  for(std::vector<art::Ptr<recob::Hit> >::iterator hit_iter=hits.begin(); hit_iter!=hits.end(); ++hit_iter){

    //Get the PeakTime
    int PeakTime = (*hit_iter)->PeakTime();

    //Get the plane ID 
    geo::WireID wireID = (*hit_iter)->WireID();
    geo::PlaneID planeID = wireID.planeID(); 
    
    Graph_map[planeID]->SetPoint(Graph_map[planeID]->GetN(),wireID.Wire, PeakTime);    
    std::cout << "Point: " << Graph_map[planeID]->GetN() << " has wire: " << wireID.Wire << " PlaneID: " << planeID  << " PeakTime: " << PeakTime << std::endl;
  }


  //Write to Canvas
  std::string evd_string;
  std::stringstream sstm_evd;
  sstm_evd << "Event " << event_num ;
  evd_string = sstm_evd.str();
  const char* evd_name = evd_string.c_str();
  //TCanvas* evd_test = new TCanvas(evd_name,evd_name,900,600);
   TCanvas* evd = tfs->makeAndRegister<TCanvas>(evd_name,"");
   evd->cd();
   evd->Divide(1,divisions);

  int division=0; 
  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){

     Graph_map[plane_id]->SetMarkerColor(2);
     Graph_map[plane_id]->SetMarkerStyle(8);
     Graph_map_daq[plane_id]->SetMarkerColor(4);
     Graph_map_daq[plane_id]->SetMarkerStyle(8);

    //Add graph to the mutligraph                                                                                                                                                
     //    Graph_map_multi[plane_id]->Add(Graph_map[plane_id]);
     // Graph_map_multi[plane_id]->Add(Graph_map_daq[plane_id]);

    ++division;
    evd->cd(division);
    std::cout << plane_id << std::endl;
    //    Graph_map[plane_id]->Draw();
    evd->Update();
    //Graph_map[plane_id]->Write();
    //Graph_map[plane_id]->Delete();
  }

  evd->Update();
  //delete evd_test;

}

DEFINE_ART_MODULE(evd::BadEventDisplay)
