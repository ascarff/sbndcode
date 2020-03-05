////////////////////////////////////////////////////////////////////////
// Class:       PFPValidation
// Plugin Type: analyzer (art v3_02_06)
// File:        PFPValidation_module.cc
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
#include "lardataobj/RecoBase/PFParticleMetadata.h"
#include "larreco/RecoAlg/MCRecoUtils/RecoUtils.h"
#include "larreco/RecoAlg/MCRecoUtils/ShowerUtils.h"

//Root Includes
#include "TMath.h"
#include "TTree.h"
#include <vector>
#include <iostream>

namespace ana {
  class PFPValidation;
}

class ana::PFPValidation : public art::EDAnalyzer {
  public:
    explicit PFPValidation(fhicl::ParameterSet const& pset);
    // The compiler-generated destructor is fine for non-base
    // classes without bare pointers or other resource use.

    // Plugins should not be copied or assigned.
    PFPValidation(PFPValidation const&) = delete;
    PFPValidation(PFPValidation&&) = delete;
    PFPValidation& operator=(PFPValidation const&) = delete;
    PFPValidation& operator=(PFPValidation&&) = delete;

    // Required functions.
    void analyze(art::Event const& evt) override;
    void beginJob();

    void clearPFPTree();
    void clearTrueTree();

    std::map<int, int> GetTruePrimaryHits(
        std::map<int,const simb::MCParticle*>& trueParticles,
        std::map<int,std::vector<int> >& truePrimaries,
        std::vector< art::Ptr< recob::Hit> >& allHits);

    std::map<int, float> GetTruePrimaryEnergies(
        std::map<int,const simb::MCParticle*>& trueParticles,
        std::map<int,std::vector<int> >& truePrimaries,
        std::vector< art::Ptr< recob::Hit> >& allHits);

    float GetTotalEnergyInHits(std::vector<art::Ptr<recob::Hit> > hits);

    template <class T>
      void initTree(TTree* Tree, std::string branchName,
          std::map<std::string, T>& Metric,
          std::vector<std::string> fPFParticleLabels);

  private:

    std::string fHitLabel, fGenieGenModuleLabel;
    std::vector<std::string> fPFParticleLabels;
    art::ServiceHandle<art::TFileService> tfs;
    art::ServiceHandle<cheat::BackTrackerService> bt_serv;
    art::ServiceHandle<cheat::ParticleInventoryService> particleInventory;

    // Declare member data here.
    TTree* trueTree;
    TTree* pfpTree;

    // Fill the tree once per particle
    int truePdg, numTrueHits;
    float trueEnergy, trueDepositedEnergy;
    std::map<std::string, int> recoPdg, recoHits, recoPFPs;
    std::map<std::string, float> hitPurity, energyPurity, hitComp, energyComp;

    std::string pfpModuleLabel;
    int pfpPdg, pfpTruePdg, pfpNumHits;
    float pfpTrueEnergy, pfpTrueDepositedEnergy;
    float pfpHitPurity, pfpHitComp, pfpEnergyPurity, pfpEnergyComp;
};


ana::PFPValidation::PFPValidation(fhicl::ParameterSet const& pset)
  : EDAnalyzer{pset}
{
  fHitLabel            = pset.get<std::string>("HitLabel");
  fGenieGenModuleLabel = pset.get<std::string>("GenieGenModuleLabel");
  fPFParticleLabels    = pset.get<std::vector<std::string> >("PFParticleLabels");
}

void ana::PFPValidation::beginJob() {

  trueTree = tfs->make<TTree>("trueTree", "Tree with event wide metrics");
  pfpTree = tfs->make<TTree>("pfpTree", "Tree with event wide metrics");

  trueTree->Branch("truePdg", &truePdg);
  trueTree->Branch("numTrueHits", &numTrueHits);
  trueTree->Branch("trueEnergy", &trueEnergy);
  trueTree->Branch("trueDepositedEnergy", &trueDepositedEnergy);

  initTree(trueTree, "recoPdg", recoPdg, fPFParticleLabels);
  initTree(trueTree, "recoHits", recoHits, fPFParticleLabels);
  initTree(trueTree, "recoPFPs", recoPFPs, fPFParticleLabels);
  initTree(trueTree, "hitPurity", hitPurity, fPFParticleLabels);
  initTree(trueTree, "hitComp", hitComp, fPFParticleLabels);
  initTree(trueTree, "energyPurity", energyPurity, fPFParticleLabels);
  initTree(trueTree, "energyComp", energyComp, fPFParticleLabels);

  pfpTree->Branch("pfpModuleLabel", &pfpModuleLabel);
  pfpTree->Branch("pfpPdg", &pfpPdg);
  pfpTree->Branch("pfpTruePdg", &pfpTruePdg);
  pfpTree->Branch("pfpNumHits", &pfpNumHits);
  pfpTree->Branch("pfpTrueEnergy", &pfpTrueEnergy);
  pfpTree->Branch("pfpTrueDepositedEnergy", &pfpTrueDepositedEnergy);
  pfpTree->Branch("pfpHitPurity", &pfpHitPurity);
  pfpTree->Branch("pfpHitComp", &pfpHitComp);
  pfpTree->Branch("pfpEnergyPurity", &pfpEnergyPurity);
  pfpTree->Branch("pfpEnergyComp", &pfpEnergyComp);
}

void ana::PFPValidation::analyze(art::Event const& evt)
{

  // Get the true g4 particles and make a map form trackId
  std::map<int,const simb::MCParticle*> trueParticles;
  const sim::ParticleList& particles = particleInventory->ParticleList();
  for (auto const& particleIt: particles){
    const simb::MCParticle* particle = particleIt.second;
    trueParticles[particle->TrackId()] = particle;
  }

  std::map<int, std::vector<int> > showerMothers = ShowerUtils::GetShowerMothersCandidates(
      trueParticles);

  std::map<int, std::vector<int> > truePrimaries;
  for (auto const& [trackId, particle]: trueParticles){
    if (abs(particle->PdgCode())==11 || abs(particle->PdgCode())==22){
      auto const& showerMotherIter = showerMothers.find(trackId);
      if (showerMotherIter != showerMothers.end()){
        truePrimaries[trackId] = showerMothers[trackId];
      }
    } else {
      truePrimaries[trackId] = {trackId};
    }
  }

  // Get reco
  // Initialse some stuff???
  std::vector<art::Handle<std::vector<recob::Hit> > > hitHandles;
  evt.getManyByType(hitHandles);
  std::vector<art::Handle<std::vector<recob::Cluster> > > clusterHandles;
  evt.getManyByType(clusterHandles);

  // Set the handles
  art::Handle<std::vector<recob::Hit> > hitHandle;
  art::Handle<std::vector<recob::Cluster> > clusterHandle;
  art::Handle<std::vector<recob::PFParticle> > pfpHandle;

  // Get all the hits
  std::vector<art::Ptr<recob::Hit> > allHits;
  if(evt.getByLabel(fHitLabel,hitHandle))
  {art::fill_ptr_vector(allHits, hitHandle);}

  // Get map of true primary particle to number of reco hits / energy in reco hits
  std::map<int, int> truePrimaryHits = GetTruePrimaryHits(trueParticles, truePrimaries, allHits);
  std::map<int, float> truePrimaryEnergies = GetTruePrimaryEnergies(trueParticles, truePrimaries,
      allHits);

  std::map<std::string, std::map<long unsigned int, art::Ptr<recob::PFParticle> > > pfpMap;
  std::map<std::string, std::map<long unsigned int, int>  > pfpTrueParticleMap, pfpHitMap;
  std::map<std::string, std::map<long unsigned int, float> >  pfpHitCompMap, pfpHitPurityMap,
    pfpEnergyCompMap, pfpEnergyPurityMap;

  for (auto const fPFParticleLabel: fPFParticleLabels){
    // Get all the PFPs
    std::vector<art::Ptr<recob::PFParticle> > pfps;
    if(evt.getByLabel(fPFParticleLabel,pfpHandle))
    {art::fill_ptr_vector(pfps,pfpHandle);}
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
    if (!clusterHandle.isValid()) {
      std::cout<<"Cluster handle not valid"<<std::endl;
      return;
    }

    art::FindManyP<recob::Hit> fmch(clusterHandle, evt, fPFParticleLabel);
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
      if (!hitHandle.isValid()) {
        std::cout<<"Hit handle not valid"<<std::endl;
        return;
      }
    }

    // Create a map between PFParticles and their IDs
    art::FindManyP<larpandoraobj::PFParticleMetadata> fmpfpmd(pfps, evt, fPFParticleLabel);
    if (!fmpfpmd.isValid() || fmpfpmd.size()==0){
      std::cout<<"PFP MetaData handle not valid"<<std::endl;
      return;
    }

    // std::map<long unsigned int, float > pfpScoreMap;
    std::vector<art::Ptr<recob::PFParticle> > pfpNeutrinoVec;
    for (auto const& pfp: pfps){
      long unsigned int pfpID = pfp->Self();
      pfpMap[fPFParticleLabel][pfpID] = pfp;
      // const std::vector< art::Ptr<larpandoraobj::PFParticleMetadata> > pfpMetaVec = fmpfpmd.at(pfpID);
      // for (auto const pfpMeta: pfpMetaVec)
      // {
      //   larpandoraobj::PFParticleMetadata::PropertiesMap propertiesMap = pfpMeta->GetPropertiesMap();
      //   pfpScoreMap[pfpID] = propertiesMap.at("NuScore");
      // }
    }

    for (auto const& pfp: pfps){
      // Get the hits from the PFParticle
      std::vector<art::Ptr<recob::Hit> > pfpHits;
      const std::vector< art::Ptr< recob::Cluster> >& clusters = fmpfc.at(pfp.key());
      for (const auto& cluster: clusters){
        const std::vector< art::Ptr< recob::Hit> >& hits = fmch.at(cluster.key());
        pfpHits.insert(pfpHits.end(), hits.begin(), hits.end());
      }

      if (!pfpHits.size())
        continue;

      std::pair<int, double> trueId = ShowerUtils::TrueParticleIDFromTrueChain(truePrimaries,
          pfpHits, 2);

      std::map<int, int> pfpTrueHitsMap = GetTruePrimaryHits(trueParticles, truePrimaries, pfpHits);
      std::map<int, float> pfpTrueEnergyMap = GetTruePrimaryEnergies(trueParticles, truePrimaries, pfpHits);

      const simb::MCParticle* trueParticle = trueParticles.at(trueId.first);

      pfpModuleLabel = fPFParticleLabel;
      pfpPdg = pfp->PdgCode();
      pfpTruePdg = trueParticle->PdgCode();
      pfpNumHits = pfpHits.size();
      pfpTrueEnergy = trueParticle->E();

      int pfpHitsTrueHits = pfpTrueHitsMap.at(trueId.first);
      float pfpHitsTrueEnergy = pfpTrueEnergyMap.at(trueId.first);
      float pfpHitsTotalEnergy = GetTotalEnergyInHits(pfpHits);

      pfpHitPurity = (float)pfpHitsTrueHits / pfpHits.size();
      pfpHitComp = (float)pfpHitsTrueHits / truePrimaryHits.at(trueId.first);
      pfpEnergyPurity = pfpHitsTrueEnergy / pfpHitsTotalEnergy;
      pfpEnergyComp= pfpHitsTrueEnergy / truePrimaryEnergies.at(trueId.first);
      // Energy is in all planes, hence divide deposited energy by 3
      pfpTrueDepositedEnergy = truePrimaryEnergies.at(trueId.first)/3;

      pfpTree->Fill();

      pfpTrueParticleMap[fPFParticleLabel][pfp->Self()] = trueId.first;
      pfpHitMap[fPFParticleLabel][pfp->Self()] = pfpHits.size();
      pfpHitPurityMap[fPFParticleLabel][pfp->Self()] = pfpHitPurity;
      pfpHitCompMap[fPFParticleLabel][pfp->Self()] = pfpHitComp;
      pfpEnergyPurityMap[fPFParticleLabel][pfp->Self()] =  pfpEnergyPurity;
      pfpEnergyCompMap[fPFParticleLabel][pfp->Self()] = pfpEnergyComp;

      clearPFPTree();
    }
  }

  for (auto const& [trueId, trueDaughters]: truePrimaries){

    const simb::MCParticle* trueParticle = trueParticles.at(trueId);
    truePdg = trueParticle->PdgCode();
    numTrueHits = truePrimaryHits.at(trueId);
    trueEnergy = trueParticle->E();
    trueDepositedEnergy = truePrimaryEnergies.at(trueId);


    for (auto const fPFParticleLabel: fPFParticleLabels){
      std::vector<long unsigned int> pfpMatches;
      for (auto const& [pfpId, pfpMatch]: pfpTrueParticleMap[fPFParticleLabel]){
        if (trueId==pfpMatch){
          pfpMatches.push_back(pfpId);
        }
      }
      recoPFPs[fPFParticleLabel] = pfpMatches.size();

      if (pfpMatches.size()>0){
        int bestMatchPFP = -999;
        float bestComp = -999;
        // Find the "Best Match" as the pfp with the highest completeness
        for (auto const& [pfpId, hitComp]: pfpHitCompMap [fPFParticleLabel]){
          if (hitComp>bestComp){
            bestMatchPFP = pfpId;
            bestComp = hitComp;
          }
        }
        recoPdg[fPFParticleLabel] = pfpMap[fPFParticleLabel].at(bestMatchPFP)->PdgCode();
        recoHits[fPFParticleLabel] = pfpHitMap[fPFParticleLabel].at(bestMatchPFP);
        hitPurity[fPFParticleLabel] = pfpHitPurityMap[fPFParticleLabel].at(bestMatchPFP);
        hitComp[fPFParticleLabel] = pfpHitCompMap[fPFParticleLabel].at(bestMatchPFP);
        energyPurity[fPFParticleLabel] = pfpEnergyPurityMap[fPFParticleLabel].at(bestMatchPFP);
        energyComp [fPFParticleLabel] = pfpEnergyCompMap[fPFParticleLabel].at(bestMatchPFP);
      }
    }

    // if (numTrueHits>0)
    trueTree->Fill();

    clearTrueTree();
  }

  // std::cout<<"\n"<<std::endl;
}

std::map<int, int> ana::PFPValidation::GetTruePrimaryHits(
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

std::map<int, float> ana::PFPValidation::GetTruePrimaryEnergies(
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

float ana::PFPValidation::GetTotalEnergyInHits(std::vector<art::Ptr<recob::Hit> >  hits){
  float energy = 0;
  for (auto const& hit:hits){
    std::vector<sim::TrackIDE> trackIDEs = bt_serv->HitToTrackIDEs(hit);
    for (auto const& trackIDE: trackIDEs){
      energy+=trackIDE.energy;
    }
  }
  return energy;
}

template <class T>
void ana::PFPValidation::initTree(TTree* Tree, std::string branchName,
    std::map<std::string, T>& Metric, std::vector<std::string> fPFParticleLabels){

  for (auto const& fPFParticleLabel: fPFParticleLabels){
    std::string branchString = branchName + "_" + fPFParticleLabel;
    const char* branchChar   = branchString.c_str();
    Tree->Branch(branchChar, &Metric[fPFParticleLabel], 32000, 0);
  }
}

void ana::PFPValidation::clearPFPTree(){
  pfpModuleLabel = "";
  pfpPdg = -999;
  pfpTruePdg = -999;
  pfpTrueEnergy = -999;
  pfpTrueDepositedEnergy = -999;
  pfpNumHits = -999;
  pfpHitPurity = -999;
  pfpHitComp = -999;
  pfpEnergyPurity = -999;
  pfpEnergyComp = -999;
}

void ana::PFPValidation::clearTrueTree(){
  truePdg = -999;
  numTrueHits = -999;
  trueEnergy = -999;
  trueDepositedEnergy = -999;
  for (auto const& fPFParticleLabel: fPFParticleLabels){
    recoPdg[fPFParticleLabel] = -999;
    recoHits[fPFParticleLabel] = -999;
    hitPurity[fPFParticleLabel] = -999;
    energyPurity[fPFParticleLabel] = -999;
    hitComp[fPFParticleLabel] = -999;
    energyComp[fPFParticleLabel] = -999;
  }
}
DEFINE_ART_MODULE(ana::PFPValidation)
