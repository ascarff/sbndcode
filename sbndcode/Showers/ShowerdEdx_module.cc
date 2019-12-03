#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/View.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

//Larsoft includes
#include "larcore/Geometry/Geometry.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "lardataobj/MCBase/MCShower.h"
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
  class ShowerdEdx;
}

class ana::ShowerdEdx: public art::EDAnalyzer {
  public:

    ShowerdEdx(const fhicl::ParameterSet& pset);

    void analyze(const art::Event& evt);
    void endJob();
    void beginJob();

  private:
    //fcl Parameters
    std::string fGenieGenModuleLabel;
    std::string fLArGeantModuleLabel;
    std::string fMCShowerTag;

    float fTrackStubLength;

    art::ServiceHandle<cheat::ParticleInventoryService> particleInventory;

    TTree* dEdxTree;
    float dEdx; // From MCShower
    float energy;
    float pdg;

    int trackID;
    float startX;
    float startY;
    float startZ;

    int numIDEs;
    int numShowers;
    int numMCShowers;
    int numParticles;

    int eventRun;
    int eventSubRun;
    int eventNumber;

    float thetaXZ;
    float thetaYZ;

    int width;
    float smear;

    /*
       std::vector<float> IDEsX;
       std::vector<float> IDEsY;
       std::vector<float> IDEsZ;
       std::vector<float> IDEsT;
       std::vector<float> IDEsE;
       */
    std::string endProcess;

    float newdEdx; // Calculated in this module
};


ana::ShowerdEdx::ShowerdEdx(const fhicl::ParameterSet& pset) : EDAnalyzer(pset){

  fGenieGenModuleLabel   = pset.get<std::string>("GenieGenModuleLabel");
  fLArGeantModuleLabel   = pset.get<std::string>("LArGeantModuleLabel");
  fMCShowerTag           = pset.get<std::string>("MCShowerTag");

  fTrackStubLength       = pset.get<float>("TrackStubLength");
}

void ana::ShowerdEdx::beginJob(){



  art::ServiceHandle<art::TFileService> tfs;
  dEdxTree = tfs->make<TTree>("dEdxTree","dEdxTree");
  dEdxTree->Branch("dEdx",&dEdx,"dEdx/F");
  dEdxTree->Branch("energy",&energy,"energy/F");
  dEdxTree->Branch("pdg",&pdg,"pdg/F");

  dEdxTree->Branch("trackID",&trackID,"trackID/I");
  dEdxTree->Branch("startX",&startX,"startX/F");
  dEdxTree->Branch("startY",&startY,"startY/F");
  dEdxTree->Branch("startZ",&startZ,"startZ/F");

  dEdxTree->Branch("numIDEs",&numIDEs,"numIDEs/I");
  dEdxTree->Branch("numShowers",&numShowers,"numShowers/I");
  dEdxTree->Branch("numMCShowers",&numMCShowers,"numMCShowers/I");
  dEdxTree->Branch("numParticles",&numParticles,"numParticles/I");

  dEdxTree->Branch("eventRun",&eventRun,"eventRun/I");
  dEdxTree->Branch("eventSubRun",&eventSubRun,"eventSubRun/I");
  dEdxTree->Branch("eventNumber",&eventNumber,"eventNumber/I");

  dEdxTree->Branch("thetaXZ",&thetaXZ,"thetaXZ/F");
  dEdxTree->Branch("thetaYZ",&thetaYZ,"thetaYZ/F");

  dEdxTree->Branch("endProcess",&endProcess);
  /*
     dEdxTree->Branch("IDEsX",&IDEsX);
     dEdxTree->Branch("IDEsY",&IDEsY);
     dEdxTree->Branch("IDEsZ",&IDEsZ);
     dEdxTree->Branch("IDEsT",&IDEsT);
     dEdxTree->Branch("IDEsE",&IDEsE);
     */
  dEdxTree->Branch("newdEdx",&newdEdx,"newdEdx/F");
  dEdxTree->Branch("width",&width,"width/I");
  dEdxTree->Branch("smear",&smear,"smear/F");

  gRandom = new TRandom3;

  std::cout<<"Track stub length: "<<fTrackStubLength<<std::endl;
}

void ana::ShowerdEdx::analyze(const art::Event& evt){

  eventRun    = evt.run();
  eventSubRun = evt.subRun();
  eventNumber = evt.event();

  //Getting  MC truth information

  // Get all of the simChannels so we can access the IDEs
  art::Handle<std::vector<sim::SimChannel> > simChannelHandle;
  std::vector<art::Ptr<sim::SimChannel> > simchannels;
  if(evt.getByLabel(fLArGeantModuleLabel,simChannelHandle))
  {art::fill_ptr_vector(simchannels, simChannelHandle);}

  numShowers   = 0;
  numMCShowers = 0;
  numParticles = 0;

  // Create a map of MCShower track IDs to dEdx values, for comparison to MCParticle
  std::map<int,float> showerMap;
  auto const& mcshowers = *evt.getValidHandle<std::vector<sim::MCShower> >(fMCShowerTag);

  for (auto const &mcs: mcshowers) {
    if ((TMath::Abs(mcs.PdgCode())==11) || (TMath::Abs(mcs.PdgCode())==22)){
      trackID = mcs.TrackID();
      dEdx   = mcs.dEdx();

      showerMap[trackID] = dEdx;
      numMCShowers++;
    }
  }

  // Create a map of trackIDs to MCParticles
  std::map<int,const simb::MCParticle*> particleMap;
  const sim::ParticleList& particles = particleInventory->ParticleList();
  for (sim::ParticleList::const_iterator particleIt = particles.begin();
      particleIt != particles.end(); ++particleIt){
    const simb::MCParticle *particle = particleIt->second;
    particleMap[particle->TrackId()] = particle;
    numParticles++;
    if (TMath::Abs(particle->PdgCode()) == 11 || TMath::Abs(particle->PdgCode()) == 22) {
      numShowers++;
    }
  }

  // Look over the MCShowers
  for (std::map<int,float>::iterator showerIt = showerMap.begin();
      showerIt != showerMap.end(); showerIt++){

    // reinitialise tree values for each shower
    dEdx     = -99999;
    energy   = -99999;
    pdg      = -99999;

    trackID  = -99999;
    startX   = -99999;
    startY   = -99999;
    startZ   = -99999;

    thetaXZ  = -99999;
    thetaYZ  = -99999;

    numIDEs  = 0;
    newdEdx  = -99999;

    // Get the MCParticle that corresponds to the MCShower
    const simb::MCParticle *particle = particleMap.at((*showerIt).first);
    // Get MCParticle truth info for the TTree
    dEdx       = (*showerIt).second;
    energy     = particle->E();
    pdg        = particle->PdgCode();
    trackID    = particle->TrackId();
    endProcess = particle->EndProcess();

    // Find the start position of the shower for electrons
    // For photons we want the end of the particle as this is
    // when the photon interacts and the showers start
    TLorentzVector startPos;
    if (TMath::Abs(particle->PdgCode()) == 11) {
      startPos = particle->Position();
    } else if (TMath::Abs(particle->PdgCode()) == 22) {
      int numTrajPoints = particle->NumberTrajectoryPoints();
      bool firstPoint = true;
      for (int i=0; i< numTrajPoints; i++){
        TLorentzVector trajPos = particle->Position(i);
        if ((particle->E(i)<0.9*energy) && firstPoint){
          startPos = particle->Position(i);
          firstPoint = false;
          //std::cout<<"First Point found: "<<energy<<" "<<particle->E(i)<<std::endl;
          //break???
        }
        //std::cout<<"TrajPoint: "<<i<<", energy: "<<particle->E(i)<<", x: "<<trajPos.Px()
        //         <<", y: "<<trajPos.Py()<<", z: "<<trajPos.Pz()<<std::endl;
      }
      //startPos = particle->Position(1);
    } else {
      std::cout<<"PDG: "<<particle->PdgCode()<<", Not a shower!!"<<std::endl;
      return;
    }

    startX =startPos.Px();
    startY =startPos.Py();
    startZ =startPos.Pz();

    // Calculate the angle of the track
    float PX = particle->Px();
    float PY = particle->Py();
    float PZ = particle->Pz();

    thetaXZ = TMath::ATan(PX/PZ);
    thetaYZ = TMath::ATan(PY/PZ);
    /*
       if (endProcess=="phot"){
    //std::cout<<"Particle: dEdx :"<<dEdx<<" Energy: "<<energy<<" End Process: "<<endProcess
    <<" PDG: "<<pdg<<" Track ID: "<<trackID<<std::endl;
    std::cout<<"Track ID: "<< trackID <<", Start Position: X: "<<startX
    <<", Y: "<<startY<<", Z: "<<startZ<<std::endl;
    }*/

    //std::vector<float> dEdxSteps;
    //  std::vector<float> dxSteps;
    float dESum   = 0;
    //float ideXOld = startX;
    //float ideYOld = startY;
    //float ideZOld = startZ;

    // Loop over the simChannels to get the IDEs
    for (std::vector<art::Ptr<sim::SimChannel>>::iterator channelIt = simchannels.begin();
        channelIt!=simchannels.end(); ++channelIt){

      //std::cout<<(*channelIt)->Channel()<<std::endl;

      // Get a map of TDCs to IDEs, then loop over the IDEs
      auto tdc_ide_map = (*channelIt)->TDCIDEMap();
      for(auto const& tdc_ide_pair : tdc_ide_map) {
        //std::cout<<"Time: "<<tdc_ide_pair.first << std::endl;
        auto const& ide_v = tdc_ide_pair.second;
        for(auto const& ide : ide_v) {
          if (TMath::Abs(ide.trackID) == TMath::Abs(trackID)){

            float ideX = ide.x;
            float ideY = ide.y;
            float ideZ = ide.z;

            // Calculate distances from ides to MCParticle starting position
            float dx = ideX - startX;
            float dy = ideY - startY;
            float dz = ideZ - startZ;
            /*
               IDEsX.push_back(ideX);
               IDEsY.push_back(ideY);
               IDEsZ.push_back(ideZ);
               IDEsT.push_back(tdc_ide_pair.first);
               IDEsE.push_back(ide.energy);
               */
            /*
               if (endProcess=="phot"){
               std::cout<<"TrackID: "<<trackID<<" IDE energy (MeV): "<<ide.energy<<std::endl;
               std::cout<<"TrackID IDE: "<<ide.trackID<<std::endl;
               std::cout<<"IDE Position: X: "<<ideX<<", Y: "<<ideY<<", Z: "<<ideZ<<", T: "
               <<tdc_ide_pair.first <<" IDE energy (MeV): "<<ide.energy<<std::endl;
               if (TMath::Sqrt(dx*dx+dy*dy+dz*dz) < fTrackStubLength){
               std::cout<<"Within Distance to Vertex"<<std::endl;
               }
               }*/

            // Only use IDEs within the distance cut of true vertex
            if (TMath::Sqrt(dx*dx+dy*dy+dz*dz) < fTrackStubLength){
              //std::cout<<"TrackID: "<<trackID<<std::endl;
              //std::cout<<"IDE energy (MeV): "<<ide.energy<<std::endl;
              //std::cout<<"TrackID IDE: "<<ide.trackID<<std::endl;

              //float dxOld = ideX - ideXOld;
              //float dyOld = ideY - ideYOld;
              //float dzOld = ideZ - ideZOld;

              //float ideDist = TMath::Sqrt(dxOld*dxOld + dyOld*dyOld + dzOld*dzOld);

              //std::cout<<"IDE Position: X: "<<ideX<<", Y: "<<ideY<<", Z: "<<ideZ<<std::endl;

              //std::cout<<"IDE Difference: "<<ideDist<<" Track ID: "<<ide.trackID<<std::endl;
              //std::cout<<"Shower Diff: "<<TMath::Sqrt(dx*dx+dy*dy+dz*dz)<<std::endl;

              //ideXOld = ideX;
              //ideYOld = ideY;
              //ideZOld = ideZ;

              numIDEs++;
              dESum += ide.energy;
            }
          }
        }
      }
    }
    // Check we have some IDEs within cut, else return -99999
    if (numIDEs>0){
      newdEdx = dESum/fTrackStubLength; // Calculate dE/dx
      newdEdx = newdEdx/3; // We are triple counting planes in the IDEs
    } else {
      std::cout<<"No Hits found"<<std::endl;
    }

    double newdEdxOLD = newdEdx;
    for (int gaussWidth=0; gaussWidth<51; gaussWidth+=1){
      newdEdx = newdEdxOLD;

      width = gaussWidth;
      smear = gRandom->Gaus(1,(float)width/100.);
      //std::cout<<"Width: "<<width<<" and Rand: "<<smear<<std::endl;
      newdEdx = newdEdx*smear;

      dEdxTree->Fill();
    }
    /*
       IDEsX.clear();
       IDEsY.clear();
       IDEsZ.clear();
       IDEsT.clear();
       IDEsE.clear();
       */
    /*
       if (endProcess=="phot"){
    //if ((numIDEs>0 &&  newdEdx<.05)){
    std::cout<<"Run: "<< eventRun<<" SubRun: "<<eventSubRun<<" Number: "<<eventNumber<<std::endl;
    std::cout<<"Process: "<<particle->Process()<<" and EndProcess: "<<endProcess<<std::endl;
    std::cout<<"Track ID: "<< trackID <<", Start Position: X: "<<startX
    <<", Y: "<<startY<<", Z: "<<startZ<<std::endl;
    std::cout<<"PDG: "<<pdg<<" and numIDEs: "<<numIDEs<<std::endl;
    std::cout<<"MCShower dEdx: "<<dEdx<<std::endl;
    std::cout<<"My New   dEdx: "<<newdEdx<<std::endl;

    int numDaughters = particle->NumberDaughters();
    std::cout<<"Number of daughters: "<<numDaughters<<std::endl;

    //  std::map<int,const simb::MCParticle*> particleMap;
    for (std::map<int,const simb::MCParticle*>::iterator partIt = particleMap.begin();
    partIt != particleMap.end(); partIt++){

    const TLorentzVector startPosition = (*partIt).second->Position();
    const TLorentzVector endPosition   = (*partIt).second->EndPosition();

    std::cout<<"PDG: "<<(*partIt).second->PdgCode()<<" start energy "<<(*partIt).second->E()
    <<" and ID: "<<(*partIt).second->TrackId()<<" Mother: "<<(*partIt).second->Mother()
    <<" Process: "<<(*partIt).second->Process() <<" End Process:" <<(*partIt).second->EndProcess()
    <<"\nstart X: "<<startPosition.Px()<<" start Y: "<<startPosition.Py()
    <<" start Z: "<<startPosition.Pz()<<"\nend X: "<<endPosition.Px()
    <<" end Y: "<<endPosition.Py()<<" end Z: "<<endPosition.Pz()
    <<" Number of Trajectory points: "<< (*partIt).second->NumberTrajectoryPoints()
    <<std::endl;;
    }
    }*/
  }
  }

  void ana::ShowerdEdx::endJob(){
  }

  DEFINE_ART_MODULE(ana::ShowerdEdx)

