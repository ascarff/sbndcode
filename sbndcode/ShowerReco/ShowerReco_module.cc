// Analyser to look at the reco file we just made                                                 

// ##########################                                                                     
// ### Framework includes ###                                                                     
// ##########################                                                                     
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Persistency/Common/FindManyP.h"



// ########################                                                                       
// ### LArSoft includes ###                                                                       
// ########################                                                                       
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireGeo.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/SpacePoint.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardata/Utilities/AssociationUtil.h"
#include "larsim/MCCheater/BackTracker.h"
#include "lardataobj/AnalysisBase/ParticleID.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larsim/MCCheater/BackTracker.h"

// #####################                                                                          
// ### ROOT includes ###                                                                          
// #####################                                                                          
#include "TComplex.h"
#include "TMath.h"
#include "Math/Functor.h"
#include "Math/Minimizer.h"
#include "Math/Factory.h"
#include "TRandom2.h"
#include "TError.h"
#include "TMinuit.h"
#include "TFile.h"
#include "TH2D.h"
#include "TH1D.h"
#include "TF1.h"
#include "TGraph.h"
#include "TTree.h"
#include "TTimeStamp.h"
#include "TChain.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TGraphAsymmErrors.h"
#include "TSystem.h"

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <utility>

namespace ana {
  class ShowerReco;
}

class ana::ShowerReco : public art::EDAnalyzer {
public:

  ShowerReco(const fhicl::ParameterSet& pset);

  void analyze(const art::Event& evt);
  void endJob();
private:

  std::string fRecoHitsModuleLabel;
  std::string fClusterModuleLabel; 
  std::string fShowerModuleLabel;
  // Hit containers                                                                                                                                                                   
  std::vector<double> dEdx;
  std::vector<double> Energy;
  double Total_Energy;
  double Length;
};

ana::ShowerReco::ShowerReco(const fhicl::ParameterSet& pset) : EDAnalyzer(pset) {
  fRecoHitsModuleLabel = pset.get<std::string>("HitsModuleLabel");
  fClusterModuleLabel = pset.get<std::string>("ClusterModuleLabel");
  fShowerModuleLabel = pset.get<std::string>("ShowerModuleLabel");
}










void ana::ShowerReco::analyze(const art::Event& evt) {

  art::Handle<std::vector<recob::Hit> > hitHandle;
  std::vector<art::Ptr<recob::Hit> > hits;

  art::Handle<std::vector<recob::Cluster> > ClusterHandle;
  std::vector<art::Ptr<recob::Cluster> >Clusters;

  art::Handle<std::vector<recob::Shower> > ShowerHandle;
  std::vector<art::Ptr<recob::Shower> > Showers; 


  if(evt.getByLabel(fRecoHitsModuleLabel, hitHandle)){
     art::fill_ptr_vector(hits, hitHandle);
     std::cout << "There are " << hits.size() << " hits in this event!" << std::endl;
    }

  if(evt.getByLabel(fClusterModuleLabel, ClusterHandle)){
    art::fill_ptr_vector(Clusters,ClusterHandle);
    std::cout << "There are " << Clusters.size() << " clusters in this event!" << std::endl;
  }

  if(evt.getByLabel(fShowerModuleLabel,ShowerHandle)){
    art::fill_ptr_vector(Showers,ShowerHandle);
    std::cout << "There are " << Showers.size() << " Showers in this event!" << std::endl; 
  }
  
  
  for(unsigned int j=0; j<Showers.size(); ++j)
    { 
      Length = Showers[j]->Length();
      dEdx = Showers[j]->dEdx();
      Energy = Showers[j]->Energy();
      Total_Energy = Energy[0] + Total_Energy;
      
      
       std::cout << "This is the shower: " << j << " dEdx: " << dEdx[0] <<std::endl;
       std::cout << "This is the shower: " << j << " Energy: " << Energy[0] <<std::endl;

    }

  std::cout<< "This is the total Energy of the Event in the collection plane: " << Total_Energy << std::endl;


return;

}


void ana::ShowerReco::endJob() {
 

}


DEFINE_ART_MODULE(ana::ShowerReco)


