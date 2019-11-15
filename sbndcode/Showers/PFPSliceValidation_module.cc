////////////////////////////////////////////////////////////////////////
// Class:       PFPSliceValidation
// Plugin Type: analyzer (art v3_02_06)
// File:        PFPSliceValidation_module.cc
//
// Generated at Wed Oct  2 03:27:09 2019 by Edward Tyley using cetskelgen
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
#include "canvas/Persistency/Common/FindOneP.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"

//LArSoft Includes
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "larsim/MCCheater/BackTrackerService.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Vertex.h"
#include "lardataobj/Simulation/SimChannel.h"
//Root Includes
#include "TMath.h"
#include "TTree.h"
#include <vector>
#include <iostream>

namespace ana {
  class PFPSliceValidation;
}


class ana::PFPSliceValidation : public art::EDAnalyzer {
  public:
    explicit PFPSliceValidation(fhicl::ParameterSet const& pset);
    // The compiler-generated destructor is fine for non-base
    // classes without bare pointers or other resource use.

    // Plugins should not be copied or assigned.
    PFPSliceValidation(PFPSliceValidation const&) = delete;
    PFPSliceValidation(PFPSliceValidation&&) = delete;
    PFPSliceValidation& operator=(PFPSliceValidation const&) = delete;
    PFPSliceValidation& operator=(PFPSliceValidation&&) = delete;

    // Required functions.
    void analyze(art::Event const& evt) override;
    void beginJob();

    void GetSliceHits(art::Event const& evt,
        const art::Ptr<recob::PFParticle>& pfp, std::vector< art::Ptr< recob::Hit> >& pfpHits,
        std::map<long unsigned int, art::Ptr<recob::PFParticle> >& pfpMap,
        art::FindManyP<recob::Cluster>& fmpfc, art::FindManyP<recob::Hit>& fmch);

    std::map<int,std::vector<int> > GetTruePrimaries(
        std::map<int,const simb::MCParticle*>& trueParticles,
        std::map<int, simb::MCNeutrino>& trueNeutrinoMap);

    std::map<int, int> GetTruePrimaryHits(
        std::map<int,const simb::MCParticle*>& trueParticles,
        std::map<int,std::vector<int> >& truePrimaries,
        std::vector< art::Ptr< recob::Hit> >& allHits);

    std::map<int, float> GetTruePrimaryEnergies(
        std::map<int,const simb::MCParticle*>& trueParticles,
        std::map<int,std::vector<int> >& truePrimaries,
        std::vector< art::Ptr< recob::Hit> >& allHits);

    int GetSliceTruthMatchHits(std::vector< art::Ptr< recob::Hit> >& pfpHits,
        std::map<int,std::vector<int> >& truePrimaries, std::map<int, int>& truePrimaryHits,
        float& completeness, float& purity);
    int GetSliceTruthMatchEnergy(std::vector< art::Ptr< recob::Hit> >& pfpHits,
        std::map<int,std::vector<int> >& truePrimaries, std::map<int, float>& truePrimaryEnergies,
        float& completeness, float& purity);

  private:

    std::string fHitLabel, fClusterLabel, fPFParticleLabel, fGenieGenModuleLabel;
    art::ServiceHandle<art::TFileService> tfs;
    art::ServiceHandle<cheat::BackTrackerService> bt_serv;
    art::ServiceHandle<cheat::ParticleInventoryService> particleInventory;
    // Declare member data here.
    TTree* eventTree;

    int trueNeutrinos, pfpNeutrinos;
    std::vector<int> trueMatches;
    std::vector<float> purities, completenesses;

    std::vector<int> intType, CCNC, neutrinoPDG, numProtons, numNeutrons, numPi, numPi0;
    std::vector<double> W, X, Y, QSqr, Pt, Theta, neutrinoE, leptonP;

    std::vector<float> trueVertexX, trueVertexY ,trueVertexZ;
    std::vector<float> pfpVertexX, pfpVertexY, pfpVertexZ, pfpVertexMag;
    std::vector<float> pfpVertexDistX, pfpVertexDistY, pfpVertexDistZ, pfpVertexDistMag;
};


ana::PFPSliceValidation::PFPSliceValidation(fhicl::ParameterSet const& pset)
  : EDAnalyzer{pset}
{
  fHitLabel            = pset.get<std::string>("HitLabel");
  fClusterLabel        = pset.get<std::string>("ClusterLabel");
  fPFParticleLabel     = pset.get<std::string>("PFParticleLabel");
  fGenieGenModuleLabel = pset.get<std::string>("GenieGenModuleLabel");
}

void ana::PFPSliceValidation::beginJob() {
  eventTree = tfs->make<TTree>("eventTree", "Tree with event wide metrics");

  eventTree->Branch("trueNeutrinos",&trueNeutrinos);
  eventTree->Branch("pfpNeutrinos",&pfpNeutrinos);
  eventTree->Branch("trueMatches",&trueMatches);
  eventTree->Branch("purities",&purities);
  eventTree->Branch("completenesses",&completenesses);

  eventTree->Branch("intType",&intType);
  eventTree->Branch("CCNC",&CCNC);
  eventTree->Branch("neutrinoPDG",&neutrinoPDG);
  eventTree->Branch("numProtons",&numProtons);
  eventTree->Branch("numNeutrons",&numNeutrons);
  eventTree->Branch("numPi",&numPi);
  eventTree->Branch("numPi0",&numPi0);

  eventTree->Branch("W",&W);
  eventTree->Branch("X",&X);
  eventTree->Branch("Y",&Y);
  eventTree->Branch("QSqr",&QSqr);
  eventTree->Branch("Pt",&Pt);
  eventTree->Branch("Theta",&Theta);
  eventTree->Branch("neutrinoE",&neutrinoE);
  eventTree->Branch("leptonP",&leptonP);

  eventTree->Branch("trueVertexX",&trueVertexX);
  eventTree->Branch("trueVertexY",&trueVertexY);
  eventTree->Branch("trueVertexZ",&trueVertexZ);

  eventTree->Branch("pfpVertexX", &pfpVertexX);
  eventTree->Branch("pfpVertexY", &pfpVertexY);
  eventTree->Branch("pfpVertexZ", &pfpVertexZ);

  eventTree->Branch("pfpVertexDistX", &pfpVertexDistX);
  eventTree->Branch("pfpVertexDistY", &pfpVertexDistY);
  eventTree->Branch("pfpVertexDistZ", &pfpVertexDistZ);
  eventTree->Branch("pfpVertexDistMag", &pfpVertexDistMag);
}

void ana::PFPSliceValidation::analyze(art::Event const& evt)
{

  // Get MC truth information
  art::Handle<std::vector<simb::MCTruth> > truthHandle;
  std::vector<art::Ptr<simb::MCTruth> > truths;
  if(evt.getByLabel(fGenieGenModuleLabel,truthHandle))
  {art::fill_ptr_vector(truths, truthHandle);}

  // Get the true neutrinos
  std::map<int, simb::MCNeutrino> trueNeutrinoMap;
  for (unsigned int i=0; i<truths.size(); i++){
    auto truth = truths.at(i);
    const simb::MCNeutrino neutrinoNeutrino = truth->GetNeutrino();
    const simb::MCParticle neutrinoParticle = neutrinoNeutrino.Nu();
    trueNeutrinoMap[i] = neutrinoNeutrino;
  }

  // Get the true g4 particles and make a map form trackID
  std::map<int,const simb::MCParticle*> trueParticles;
  const sim::ParticleList& particles = particleInventory->ParticleList();
  for (auto const& particleIt: particles){
    const simb::MCParticle* particle = particleIt.second;
    trueParticles[particle->TrackId()] = particle;
  }

  // Get a map of the true neutrinos to the g4 particles
  std::map<int,std::vector<int> > truePrimaries = GetTruePrimaries(trueParticles, trueNeutrinoMap);

  //check the neutrino has some particles from g4
  for (auto trueNeutrinoIter=trueNeutrinoMap.begin();
      trueNeutrinoIter!= trueNeutrinoMap.end();){
    if(truePrimaries.find(trueNeutrinoIter->first) == truePrimaries.end()){
      std::cout<<"Neutrino has no g4 daughters. Erasing:"<<trueNeutrinoIter->first<<std::endl;
      trueNeutrinoMap.erase(trueNeutrinoIter++);
    } else {
      trueNeutrinoIter++;
    }
  }

  // Get reco
  // Initialse some stuff???
  std::vector<art::Handle<std::vector<recob::Hit> > > hitHandles;
  evt.getManyByType(hitHandles);
  std::vector<art::Handle<std::vector<recob::Cluster> > > clusterHandles;
  evt.getManyByType(clusterHandles);
  std::vector<art::Handle<std::vector<recob::Vertex> > > vertexHandles;
  evt.getManyByType(vertexHandles);

  // Set the handles
  art::Handle<std::vector<recob::Hit> > hitHandle;
  art::Handle<std::vector<recob::Cluster> > clusterHandle;
  art::Handle<std::vector<recob::PFParticle> > pfpHandle;
  art::Handle<std::vector<recob::Vertex > > vertexHandle;

  // Get all the hits
  std::vector<art::Ptr<recob::Hit> > allHits;
  if(evt.getByLabel(fHitLabel,hitHandle))
  {art::fill_ptr_vector(allHits, hitHandle);}

  // Get all the PFPs
  std::vector<art::Ptr<recob::PFParticle> > pfps;
  if(evt.getByLabel(fPFParticleLabel,pfpHandle))
  {art::fill_ptr_vector(pfps,pfpHandle);}


  // Get map of true primary particle to number of reco hits / energy in reco hits
  std::map<int, int> truePrimaryHits = GetTruePrimaryHits(trueParticles, truePrimaries, allHits);
  std::map<int, float> truePrimaryEnergies = GetTruePrimaryEnergies(trueParticles, truePrimaries,
      allHits);

  art::FindManyP<recob::Cluster> fmpfc(pfpHandle, evt, fPFParticleLabel);
  if(fmpfc.isValid() && fmpfc.size()>0){
    for (unsigned int fmpfcIter=0; fmpfcIter<fmpfc.size(); fmpfcIter++){
      if (fmpfc.at(fmpfcIter).size()==0) continue;
      evt.get(fmpfc.at(fmpfcIter).front().id(),clusterHandle);
      if (!clusterHandle.isValid()) {
        std::cout<<"Cluster handle not valid"<<std::endl;
        return;
      }
      break;
    }
  }

  art::FindManyP<recob::Hit> fmch(clusterHandle, evt, fClusterLabel);
  if(fmch.isValid() && fmch.size()>0){
    for (unsigned int fmchIter=0; fmchIter<fmch.size(); fmchIter++){
      if (fmch.at(fmchIter).size()==0) continue;
      evt.get(fmch.at(fmchIter).front().id(),hitHandle);
      if (!hitHandle.isValid()) {
        std::cout<<"Hit handle not valid"<<std::endl;
        return;
      }
      break;
    }
  }

  art::FindOneP<recob::Vertex> fopfv(pfpHandle, evt, fPFParticleLabel);
  if (fopfv.isValid() && fopfv.size()>0){
    evt.get(fopfv.at(0).id(),vertexHandle);
    if(!vertexHandle.isValid()) {
      std::cout<<"Vertex handle not valid"<<std::endl;
      return;
    }
  }

  // Create a map between PFParticles and their IDs
  std::map<long unsigned int, art::Ptr<recob::PFParticle> > pfpMap;
  std::vector<art::Ptr<recob::PFParticle> > pfpNeutrinoVec;
  for (auto const& pfp: pfps){
    pfpMap[pfp->Self()] = pfp;
    if ((pfp->PdgCode()==12) ||(pfp->PdgCode()==14)){
      pfpNeutrinoVec.push_back(pfp);
    }
  }

  for (const auto& pfpNeutrino: pfpNeutrinoVec){

    art::Ptr<recob::Vertex> pfpVertex = fopfv.at(pfpNeutrino.key());
    double pfpVtx[3];
    pfpVertex->XYZ(pfpVtx);

    // std::cout<<"Reco vertex: X: "<<pfpVtx[0]<<" Y: "<<pfpVtx[1]<<" z: "<<pfpVtx[2]<<std::endl;
    // if (TMath::Abs(pfpVtx[0]) > 190.0 ||
    //     TMath::Abs(pfpVtx[1]) > 190.0 ||
    //     pfpVtx[2] < 10.0 || pfpVtx[2]> 490.0){
    //   std::cout<<"Reco vertex failed FV cut."<<std::endl;
    //   continue;
    // }

    pfpVertexX.push_back(pfpVtx[0]);
    pfpVertexY.push_back(pfpVtx[1]);
    pfpVertexZ.push_back(pfpVtx[2]);

    std::vector<art::Ptr<recob::Hit> > pfpNeutrinoHits;
    GetSliceHits(evt, pfpNeutrino, pfpNeutrinoHits, pfpMap, fmpfc, fmch);
    // std::cout<<pfpNeutrinoHits.size()<<std::endl;
    float purity = -999, completeness = -999;
    int trueMatch = GetSliceTruthMatchHits(pfpNeutrinoHits, truePrimaries, truePrimaryHits,
        completeness, purity);

    std::cout<<"True Match: "<<trueMatch<<" with completeness: "<<completeness
      <<" and purity: "<<purity<<std::endl;
    trueMatches.push_back(trueMatch);
    purities.push_back(purity);
    completenesses.push_back(completeness);

    if (trueMatch != -999){

      // Get the true neutrino vertex
      const simb::MCNeutrino& trueNeutrino = trueNeutrinoMap.at(trueMatch);
      TLorentzVector trueNeutrinoStart = trueNeutrino.Nu().Trajectory().Position(0);

      // Get the pfp neutrino vertex

      double xDiff = pfpVtx[0] - trueNeutrinoStart.X();
      double yDiff = pfpVtx[1] - trueNeutrinoStart.Y();
      double zDiff = pfpVtx[2] - trueNeutrinoStart.Z();

      pfpVertexDistX.push_back(xDiff);
      pfpVertexDistY.push_back(yDiff);
      pfpVertexDistZ.push_back(zDiff);
      pfpVertexDistMag.push_back(TMath::Sqrt(TMath::Power(xDiff,2) +
            TMath::Power(yDiff,2) + TMath::Power(zDiff,2)));

    } else {
      pfpVertexDistX.push_back(-999);
      pfpVertexDistY.push_back(-999);
      pfpVertexDistZ.push_back(-999);
      pfpVertexDistMag.push_back(-999);
    }
  }


  // Do FV cut at the end, this way we don't penalise matching to an out of fv
  // neutrino
  for (auto trueNeutrinoIter=trueNeutrinoMap.begin();
      trueNeutrinoIter!= trueNeutrinoMap.end();){
    TLorentzVector trueNeutrinoStart = trueNeutrinoIter->second.Nu().Trajectory().Position(0);

    if (TMath::Abs(trueNeutrinoStart.X()) > 190.0 ||
        TMath::Abs(trueNeutrinoStart.Y()) > 190.0 ||
        trueNeutrinoStart.Z() < 10.0 || trueNeutrinoStart.Z() > 490.0){

      std::cout<<"Out of FV. Removing: "<<trueNeutrinoIter->first<<std::endl;

      for (unsigned int i=0; i<trueMatches.size();){
        if (trueMatches.at(i) == trueNeutrinoIter->first){
          trueMatches.erase(trueMatches.begin()+i);
          purities.erase(purities.begin()+i);
          completenesses.erase(completenesses.begin()+i);
        } else {
          i++;
        }
      }
      trueNeutrinoMap.erase(trueNeutrinoIter++);
    } else {

      const simb::MCParticle Lepton  = trueNeutrinoIter->second.Lepton();

      intType.push_back(trueNeutrinoIter->second.Mode());
      CCNC.push_back(trueNeutrinoIter->second.CCNC());
      neutrinoPDG.push_back(trueNeutrinoIter->second.Nu().PdgCode());

      W.push_back(trueNeutrinoIter->second.W());
      X.push_back(trueNeutrinoIter->second.X());
      Y.push_back(trueNeutrinoIter->second.Y());
      QSqr.push_back(trueNeutrinoIter->second.QSqr());
      Pt.push_back(trueNeutrinoIter->second.Pt());
      Theta.push_back(trueNeutrinoIter->second.Theta());
      neutrinoE.push_back(trueNeutrinoIter->second.Nu().E());
      leptonP.push_back(Lepton.P());

      trueVertexX.push_back(trueNeutrinoStart.X());
      trueVertexY.push_back(trueNeutrinoStart.Y());
      trueVertexZ.push_back(trueNeutrinoStart.Z());


      trueNeutrinoIter++;
    }
  }

  trueNeutrinos = trueNeutrinoMap.size();
  pfpNeutrinos = trueMatches.size();
  std::cout<<"trueNeutrinos: "<<trueNeutrinos<<" and pfpNeutrinos: "<<pfpNeutrinos<<std::endl;

  eventTree->Fill();
  std::cout<<"\n"<<std::endl;

  trueMatches.clear();
  purities.clear();
  completenesses.clear();

  intType.clear();
  CCNC.clear();
  neutrinoPDG.clear();
  numProtons.clear();
  numNeutrons.clear();
  numPi.clear();
  numPi0.clear();

  W.clear();
  X.clear();
  Y.clear();
  QSqr.clear();
  Pt.clear();
  Theta.clear();
  neutrinoE.clear();
  leptonP.clear();

  trueVertexX.clear();
  trueVertexY.clear();
  trueVertexZ.clear();

  pfpVertexX.clear();
  pfpVertexY.clear();
  pfpVertexZ.clear();

  pfpVertexDistX.clear();
  pfpVertexDistY.clear();
  pfpVertexDistZ.clear();
  pfpVertexDistMag.clear();
}

void ana::PFPSliceValidation::GetSliceHits(art::Event const& evt,
    const art::Ptr<recob::PFParticle>& pfp, std::vector< art::Ptr< recob::Hit> >& pfpHits,
    std::map<long unsigned int, art::Ptr<recob::PFParticle> >& pfpMap,
    art::FindManyP<recob::Cluster>& fmpfc, art::FindManyP<recob::Hit>& fmch){

  // Get the hits from the PFParticle
  const std::vector< art::Ptr< recob::Cluster> >& clusters = fmpfc.at(pfp.key());
  for (const auto& cluster: clusters){
    const std::vector< art::Ptr< recob::Hit> >& hits = fmch.at(cluster.key());
    pfpHits.insert(pfpHits.end(), hits.begin(), hits.end());
  }

  // Get the daughters
  const std::vector<long unsigned int> daughters = pfp->Daughters();
  for (const auto daughterIter: daughters){
    art::Ptr<recob::PFParticle> daughter =  pfpMap.at(daughterIter);
    // Get the hits from the daughters
    GetSliceHits(evt, daughter, pfpHits, pfpMap, fmpfc,fmch);
  }
  return;
}

std::map<int,std::vector<int> >  ana::PFPSliceValidation::GetTruePrimaries(
    std::map<int, const simb::MCParticle*>& trueParticles,
    std::map<int, simb::MCNeutrino>& trueNeutrinoMap){

  std::map<int, TLorentzVector> trueNeutrinoStartMap;
  for (const auto trueNeutrinoIter: trueNeutrinoMap){
    TLorentzVector trueNeutrinoStart = trueNeutrinoIter.second.Nu().Trajectory().Position(0);
    std::cout<<"True Neutrino at: "<<trueNeutrinoIter.first
      <<" X: "<<trueNeutrinoStart.X()
      <<" Y: "<<trueNeutrinoStart.Y()
      <<" Z: "<<trueNeutrinoStart.Z()
      <<" T: "<<trueNeutrinoStart.T()
      <<" int. type: "<<trueNeutrinoIter.second.CCNC()<<std::endl;
    trueNeutrinoStartMap[trueNeutrinoIter.first] = trueNeutrinoStart;
  }


  std::map<int,std::vector<int> > truePrimaries;
  for (auto const& particleIt: trueParticles){
    const simb::MCParticle* particle = particleIt.second;
    const simb::MCParticle *mother   = particle;
    // if (TMath::Abs(particle->PdgCode()==12) ||TMath::Abs(particle->PdgCode()==14))
    // std::cout<<particle->TrackId()<<" and mother: "<<particle->Mother()<<" and pdg: "<<
    // particle->PdgCode()<<" and process: "<<particle->Process()<<std::endl;
    while (mother->Mother()!=0){
      if (trueParticles.find(mother->Mother())==trueParticles.end()){
        break;
      }
      mother = trueParticles[mother->Mother()];
    }

    //If cosmic: ID = -999
    int motherID = -999;
    for (const auto trueNeutrinoIter: trueNeutrinoMap){
      TLorentzVector particleStart = mother->Position();
      if ((trueNeutrinoStartMap.at(trueNeutrinoIter.first)-particleStart).Mag() < 1)
        motherID = trueNeutrinoIter.first;
    }

    // TLorentzVector partstart = mct.Start().Position();
    truePrimaries[motherID].push_back(particle->TrackId());
  }
  return truePrimaries;
}

std::map<int, int> ana::PFPSliceValidation::GetTruePrimaryHits(
    std::map<int,const simb::MCParticle*>& trueParticles,
    std::map<int,std::vector<int> >& truePrimaries,
    std::vector< art::Ptr< recob::Hit> >& allHits){

  std::map<int,int> trueParticleHits;
  for (const auto& hit: allHits){
    int trackID     = 0;
    float hitEnergy = 0;
    std::vector<sim::TrackIDE> trackIDEs = bt_serv->HitToTrackIDEs(hit);
    for (const auto& ide: trackIDEs) {
      if (ide.energy > hitEnergy){
        hitEnergy = ide.energy;
        trackID   = TMath::Abs(ide.trackID);
      }
    }
    ++trueParticleHits[trackID];
  }

  std::map<int, int> truePrimaryHits;
  for (const auto& truePrimary: truePrimaries){
    for (const auto& trueDaughter: truePrimary.second){
      truePrimaryHits[truePrimary.first] += trueParticleHits[trueDaughter];
    }
  }
  return truePrimaryHits;
}

std::map<int, float> ana::PFPSliceValidation::GetTruePrimaryEnergies(
    std::map<int,const simb::MCParticle*>& trueParticles,
    std::map<int,std::vector<int> >& truePrimaries,
    std::vector< art::Ptr< recob::Hit> >& allHits){

  std::map<int, float> trueParticleEnergies;
  for (const auto& hit: allHits){
    std::vector<sim::TrackIDE> trackIDEs = bt_serv->HitToTrackIDEs(hit);
    for (const auto& ide: trackIDEs) {
      trueParticleEnergies[TMath::Abs(ide.trackID)] += ide.energy;
    }
  }
  std::map<int, float> truePrimaryEnergies;
  for (const auto& truePrimary: truePrimaries){
    for (const auto& trueDaughter: truePrimary.second){
      truePrimaryEnergies[truePrimary.first] += trueParticleEnergies[trueDaughter];
    }
  }
  return truePrimaryEnergies;
}

int ana::PFPSliceValidation::GetSliceTruthMatchHits(std::vector< art::Ptr< recob::Hit> >& pfpHits,
    std::map<int,std::vector<int> >& truePrimaries, std::map<int, int>& truePrimaryHits,
    float& completeness, float& purity){

  std::map<int,int> particleHits;
  for (const auto& hit: pfpHits){
    int trackID     = 0;
    float hitEnergy = 0;
    std::vector<sim::TrackIDE> trackIDEs = bt_serv->HitToTrackIDEs(hit);
    for (const auto& ide: trackIDEs) {
      if (ide.energy > hitEnergy){
        hitEnergy = ide.energy;
        trackID   = TMath::Abs(ide.trackID);
      }
    }
    ++particleHits[trackID];
  }

  std::map<int, int> primaryHits;
  for (const auto& primary: truePrimaries){
    for (const auto& trueDaughter: primary.second){
      primaryHits[primary.first] += particleHits[trueDaughter];
    }
  }

  int maxHits     = 0;
  int bestPrimary = -999;
  for (const auto& primaryHit: primaryHits){
    if (primaryHit.first!=-999 && primaryHit.second > maxHits){
      bestPrimary = primaryHit.first;
      maxHits     = primaryHit.second;
    }
  }

  purity       = (float) maxHits / pfpHits.size();
  completeness = (float) maxHits / truePrimaryHits.at(bestPrimary);
  return bestPrimary;
}

int ana::PFPSliceValidation::GetSliceTruthMatchEnergy(std::vector< art::Ptr< recob::Hit> >& pfpHits,
    std::map<int,std::vector<int> >& truePrimaries, std::map<int, float>& truePrimaryEnergies,
    float& completeness, float& purity){

  float pfpEnergySum = 0;
  std::map<int, float> particleEnergies;
  for (const auto& hit: pfpHits){
    std::vector<sim::TrackIDE> trackIDEs = bt_serv->HitToTrackIDEs(hit);
    for (const auto& ide: trackIDEs) {
      particleEnergies[TMath::Abs(ide.trackID)] += ide.energy;
      pfpEnergySum +=ide.energy;
    }
  }
  std::map<int, float> primaryEnergies;
  for (const auto& primary: truePrimaries){
    for (const auto& trueDaughter: primary.second){
      primaryEnergies[primary.first] += particleEnergies[trueDaughter];
    }
  }

  int maxEnergy   = 0;
  int bestPrimary = -999;
  for (const auto& primaryEnergy: primaryEnergies){
    if (primaryEnergy.first!=-999 && primaryEnergy.second > maxEnergy){
      bestPrimary = primaryEnergy.first;
      maxEnergy   = primaryEnergy.second;
    }
  }

  purity       = (float) maxEnergy / pfpEnergySum;
  completeness = (float) maxEnergy / truePrimaryEnergies.at(bestPrimary);
  return bestPrimary;
}
DEFINE_ART_MODULE(ana::PFPSliceValidation)
