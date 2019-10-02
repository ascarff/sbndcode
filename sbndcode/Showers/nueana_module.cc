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
#include "larcoreobj/SummaryData/POTSummary.h"
#include "larcore/Geometry/Geometry.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "lardataobj/Simulation/SimChannel.h"
#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/ParticleInventoryService.h"

//Root Includes
#include "TMath.h"
#include "TTree.h"
#include "TRandom3.h"

//C++ Includes
#include <vector>
#include <iostream>


namespace ana {
  class nueana;
}

class ana::nueana: public art::EDAnalyzer {
public:

  nueana(const fhicl::ParameterSet& pset);

  void analyze(const art::Event& evt);
  void endJob();
  void beginJob();

private:
  //fcl Parameters
  std::string fGenieGenModuleLabel;
  std::string fLArGeantModuleLabel;
  std::string fPOTModuleLabel;
  
  art::ServiceHandle<cheat::ParticleInventoryService> particleInventory;
  
  TTree* Tree;
  int intType;
  int CCNC;
  double QSqr;
  double E;
  double leptonE;

  int neutPDG;
  int numProtons;
  int numPi;
  int numPi0;

  int    subRun;  
  double pot;  
  //std::string endProcess;

};


ana::nueana::nueana(const fhicl::ParameterSet& pset) : EDAnalyzer(pset){

  fGenieGenModuleLabel   = pset.get<std::string>("GenieGenModuleLabel");
  fLArGeantModuleLabel   = pset.get<std::string>("LArGeantModuleLabel");
  fPOTModuleLabel        = pset.get< std::string>("POTModuleLabel");

 }

void ana::nueana::beginJob(){
  
  subRun = -999;
  
  art::ServiceHandle<art::TFileService> tfs;
  Tree = tfs->make<TTree>("Tree","Tree");
  
  Tree->Branch("intType",&intType,"intType/I");
  Tree->Branch("CCNC",&CCNC,"CCNC/I");
  Tree->Branch("QSqr",&QSqr,"QSqr/D");
  Tree->Branch("E",&E,"E/D");
  Tree->Branch("leptonE",&leptonE,"leptonE/D");
  Tree->Branch("pot",&pot,"pot/D");
  Tree->Branch("neutPDG",&neutPDG,"NeutPDG/I");
  Tree->Branch("numProtons",&numProtons,"numProtons/I");
  Tree->Branch("numPi",&numPi,"numPi/I");  
  Tree->Branch("numPi0",&numPi0,"numPi0/I");


  //dEdxTree->Branch("endProcess",&endProcess);
}

void ana::nueana::analyze(const art::Event& evt){

  //eventRun    = evt.run();
  int eventSubRun = evt.subRun();
  //eventNumber = evt.event();
  

  numProtons = 0;
  numPi      = 0;
  numPi0     = 0;
  QSqr       = 0;
  E          = 0;
  
  const art::SubRun& sr = evt.getSubRun();
  
  art::Handle< sumdata::POTSummary > potListHandle;
  sr.getByLabel(fPOTModuleLabel,potListHandle);                                                    
  if (eventSubRun!=subRun){
    pot+=potListHandle->totpot;
    subRun=eventSubRun;
  }
  std::cout<<"Subrun: "<< eventSubRun<<" with POT: "<<pot<<std::endl;
  

  //Getting  MC truth information
  art::Handle<std::vector<simb::MCTruth> > truthHandle;
  std::vector<art::Ptr<simb::MCTruth> > truths;
  if(evt.getByLabel(fGenieGenModuleLabel,truthHandle))
    {art::fill_ptr_vector(truths, truthHandle);}
  
  if (truths.size()==1){
    for (auto truth : truths){
      const simb::MCNeutrino & neut = truth->GetNeutrino();
      intType = neut.Mode();
      CCNC    = neut.CCNC(); 
      QSqr    = neut.QSqr();
      std::cout<<"Interaction Type: "<<intType<<std::endl;
      std::cout<<"NC / CC: "<<CCNC<<std::endl;
      std::cout<<"Q^2: "<<QSqr<<std::endl;
      
      const simb::MCParticle Nu  = neut.Nu();
      E = Nu.E();
      neutPDG = Nu.PdgCode();

      const simb::MCParticle Lepton  = neut.Lepton();
      leptonE = Lepton.E();

      std::cout<<Nu.PdgCode()<<" Num Daughters: "<<Nu.NumberDaughters()
	       <<" Energy: "<<E<<" Track ID: "<<Nu.TrackId()<<std::endl;
    }	
  


  const sim::ParticleList& particles = particleInventory->ParticleList();
  for (sim::ParticleList::const_iterator particleIt = particles.begin();
       particleIt != particles.end(); ++particleIt){
    const simb::MCParticle *particle = particleIt->second;
    if (particle->Mother()==0){
      //double energy = particle->E()-particle->Mass();
      //std::cout<<"ID: "<<particle->TrackId()<<" pdg: "<<particle->PdgCode()<<" mother: "
      //	       <<particle->Mother()<<" energy: "<<energy<<std::endl;
      
      //if (particle->PdgCode() == 2212 && energy>0.021) { 
      if (particle->PdgCode() == 2212) { 
	std::cout<<"Proton"<<std::endl; 
	++numProtons;
      }  else if (TMath::Abs(particle->PdgCode()) == 211) {
	std::cout<<"Pi +-"<<std::endl;
	++numPi;
      }  else if (TMath::Abs(particle->PdgCode()) == 111) {
	std::cout<<"Pi 0"<<std::endl; 
	++numPi0;
      }
    }
  }
  Tree->Fill();
  } else {
    std::cout<<"Broken: "<<truths.size();
  }

}

void ana::nueana::endJob(){
}

DEFINE_ART_MODULE(ana::nueana)

