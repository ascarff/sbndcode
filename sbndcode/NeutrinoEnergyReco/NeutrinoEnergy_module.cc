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
#include "nusimdata/SimulationBase/MCTrajectory.h"
#include "larsim/MCCheater/BackTracker.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"

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
#include <map>


namespace ana {
  class NeutrinoEnergy;
}

enum class TrackID   : int { };

class ana::NeutrinoEnergy : public art::EDAnalyzer {
public:

  NeutrinoEnergy(const fhicl::ParameterSet& pset);

  void analyze(const art::Event& evt);
  void endJob();
private:

  std::string fHitsModuleLabel;
  std::string fTrackModuleLabel;
  std::string fCalorimetryModuleLabel;
  art::ServiceHandle<cheat::BackTracker> backtracker;
  std::map<int,const simb::MCParticle*> trueParticles;
};

ana::NeutrinoEnergy::NeutrinoEnergy(const fhicl::ParameterSet& pset) : EDAnalyzer(pset) {
  fHitsModuleLabel = pset.get<std::string>("HitsModuleLabel");
  fTrackModuleLabel = pset.get<std::string>("TrackModuleLabel");
  fCalorimetryModuleLabel = pset.get<std::string>("CalorimetryModuleLabel");
}


void ana::NeutrinoEnergy::analyze(const art::Event& evt) {

  art::Handle<std::vector<recob::Hit> > hitHandle;
  std::vector<art::Ptr<recob::Hit> > hits;

  art::Handle<std::vector<recob::Track> > TrackHandle;
  std::vector<art::Ptr<recob::Track> > Tracks; 

  art::Handle<std::vector<anab::Calorimetry> > CalorimetryHandle;
  std::vector<art::Ptr<anab::Calorimetry> > Calo;

  if(evt.getByLabel(fCalorimetryModuleLabel, CalorimetryHandle)){
     art::fill_ptr_vector(Calo, CalorimetryHandle);
     std::cout << "There are " << Calo.size() << std::endl;     
  }

 
  if(evt.getByLabel(fHitsModuleLabel, hitHandle)){
    art::fill_ptr_vector(hits, hitHandle);
    std::cout << "There are " << hits.size() << std::endl;
  }  

  if(evt.getByLabel(fTrackModuleLabel, TrackHandle)){
    art::fill_ptr_vector(Tracks, TrackHandle);
    std::cout << "There are " << Tracks.size() << std::endl;
  }

  // === Association between Tracks and 2d Hits ===                                                                          
  //  art::FindManyP<recob::Track>       fmtk(hitHandle,   evt, fTrackModuleLabel);
  std::cout << "test" << std::endl;
  art::FindManyP<anab::Calorimetry>   focl(TrackHandle, evt, fTrackModuleLabel); 
  art::FindMany<anab::Calorimetry> fmcal(TrackHandle, evt, fCalorimetryModuleLabel);
  std::cout << "test2" << std::endl;
  

// Save true tracks                                                                                                                                                                
  trueParticles.clear();
  const sim::ParticleList& particles = backtracker->ParticleList();
  for (sim::ParticleList::const_iterator particleIt = particles.begin(); particleIt != particles.end(); ++particleIt) {
    const simb::MCParticle *particle = particleIt->second;
    trueParticles[particle->TrackId()] = particle;
  }

  for(std::map<int,const simb::MCParticle*>::iterator it = trueParticles.begin(); it != trueParticles.end(); ++it) {
   
        int    TrueTrackID = it->first;
	double Energy= trueParticles[TrueTrackID]->EndE();  
	double PdgCode= trueParticles[TrueTrackID]->PdgCode();
	double mother = trueParticles[TrueTrackID]->Mother();
	std::cout << "Track ID: " <<  it->first <<" The Energy: "<<  Energy << " Code: " << PdgCode << " Mother: " << mother << std::endl;
  }

  for (size_t iTrk=0;iTrk<Tracks.size(); ++iTrk){
  if (fmcal.isValid()){
    std::vector<const anab::Calorimetry*> calos = fmcal.at(iTrk);
    if (calos.size() > 3) {
      // if you get this message, there is probably a bug somewhere since                                                                                                        
      // the calorimetry planes should be 3.                                                                                                                                     
      // mf::LogError("AnalysisTree:limits")
      //<< "the " << fTrackModuleLabel << " track #" << iTrk
      //	<< " has " << calos.size() << " planes for calorimetry , only ";
      std::cout << "Size of calo is too big for the number of planes" << std::endl;      

    }
    for (size_t ical = 0; ical<calos.size(); ++ical){
      if (!calos[ical]) continue;
      if (!calos[ical]->PlaneID().isValid) continue;
      int planenum = calos[ical]->PlaneID().Plane;
      const double Energy    = calos[ical]->KineticEnergy();
      std::cout << "The track: "<< iTrk <<" in the plane: " << planenum << " has Energy: " << Energy << std::endl;
    }
  }
  }

return;
}


void ana::NeutrinoEnergy::endJob() {
 

}


DEFINE_ART_MODULE(ana::NeutrinoEnergy)


