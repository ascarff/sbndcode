////////////////////////////////////////////////////////////////////////
// Class:       SCEValidation
// Plugin Type: analyzer (art v3_04_00)
// File:        SCEValidation_module.cc
//
// Generated at Mon Mar  9 10:24:13 2020 by Edward Tyley using cetskelgen
// from cetlib version v3_09_00.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "art_root_io/TFileService.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"

//LArSoft Includes
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/TrackingTypes.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/AnalysisBase/T0.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"
#include "larreco/RecoAlg/MCRecoUtils/RecoUtils.h"

//Root Includes
#include "TMath.h"
#include "TTree.h"
#include <vector>
#include <iostream>

namespace ana {
  class SCEValidation;
}


class ana::SCEValidation : public art::EDAnalyzer {
  public:
    explicit SCEValidation(fhicl::ParameterSet const& p);
    // The compiler-generated destructor is fine for non-base
    // classes without bare pointers or other resource use.

    // Plugins should not be copied or assigned.
    SCEValidation(SCEValidation const&) = delete;
    SCEValidation(SCEValidation&&) = delete;
    SCEValidation& operator=(SCEValidation const&) = delete;
    SCEValidation& operator=(SCEValidation&&) = delete;

    // Required functions.
    void analyze(art::Event const& e) override;
    void beginJob();

    void resetT0Tree();
    void resetTrackTree();
    void resetCaloTree();

    template <class T>
      void initTree(TTree* Tree, std::string branchName,
          std::map<std::string, T>& Metric,
          std::vector<std::string> fPFParticleLabels);

  private:

    // Declare member data here.
    std::string fHitLabel;

    std::vector<std::string> fPFParticleLabels, fTrackLabels, fT0Labels, fCaloLabels;

    art::ServiceHandle<art::TFileService> tfs;
    art::ServiceHandle<cheat::ParticleInventoryService> particleInventory;

    TTree* t0Tree;
    TTree* trackTree;
    TTree* caloTree;

    std::string t0TreeLabel;
    std::vector<float> t0TimeVec;

    std::string trackTreeLabel;
    float trackStartX, trackStartY, trackStartZ;
    float trackEndX, trackEndY, trackEndZ;
    float trackTrueT0;
    std::map<std::string, float> trackT0Map;

    std::string caloLabel;
    std::vector<float> calodEdxVec, calodQdxVec, caloRangeVec, caloPitchVec;
    std::vector<float> caloXVec, caloYVec, caloZVec;
};


ana::SCEValidation::SCEValidation(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
  fHitLabel            = p.get<std::string>("HitLabel");
  fPFParticleLabels    = p.get<std::vector<std::string> >("PFParticleLabels");
  fTrackLabels         = p.get<std::vector<std::string> >("TrackLabels");
  fT0Labels            = p.get<std::vector<std::string> >("T0Labels");
  fCaloLabels          = p.get<std::vector<std::string> >("CaloLabels");
}

void ana::SCEValidation::beginJob()
{
  t0Tree = tfs->make<TTree>("t0Tree", "Tree with event wide metrics");
  trackTree = tfs->make<TTree>("trackTree", "Tree with event wide metrics");
  caloTree = tfs->make<TTree>("caloTree", "Tree with event wide metrics");

  t0Tree->Branch("t0Label", &t0TreeLabel);
  t0Tree->Branch("t0Times", &t0TimeVec);

  trackTree->Branch("trackLabel", &trackTreeLabel);
  trackTree->Branch("trackStartX", &trackStartX);
  trackTree->Branch("trackStartY", &trackStartY);
  trackTree->Branch("trackStartZ", &trackStartZ);
  trackTree->Branch("trackEndX", &trackEndX);
  trackTree->Branch("trackEndY", &trackEndY);
  trackTree->Branch("trackEndZ", &trackEndZ);
  trackTree->Branch("trackTrueT0", &trackTrueT0);

  initTree(trackTree, "trackT0", trackT0Map, fT0Labels);

  caloTree->Branch("caloLabel", &caloLabel);
  caloTree->Branch("calodEdxVec", &calodEdxVec);
  caloTree->Branch("calodQdxVec", &calodQdxVec);
  caloTree->Branch("caloPitchVec", &caloPitchVec);
  caloTree->Branch("caloRangeVec", &caloRangeVec);
  caloTree->Branch("caloXVec", &caloXVec);
  caloTree->Branch("caloYVec", &caloYVec);
  caloTree->Branch("caloZVec", &caloZVec);
}

void ana::SCEValidation::analyze(art::Event const& evt)
{
  // Implementation of required member function here.
  // Get the true g4 particles and make a map form trackId
  std::map<int,const simb::MCParticle*> trueParticles;
  const sim::ParticleList& particles = particleInventory->ParticleList();
  for (auto const& particleIt: particles){
    const simb::MCParticle* particle = particleIt.second;
    trueParticles[particle->TrackId()] = particle;
  }
  // Get all the T0
  for (auto const& fT0Label: fT0Labels){

    resetT0Tree();

    art::Handle<std::vector<anab::T0> > t0Handle;
    std::vector<art::Ptr<anab::T0> > t0s;
    if(evt.getByLabel(fT0Label, t0Handle))
    {art::fill_ptr_vector(t0s, t0Handle);}

    t0TreeLabel = fT0Label;
    for (art::Ptr<anab::T0> t0: t0s){
      t0TimeVec.push_back(t0->Time());
    }
    t0Tree->Fill();
  }

  // Get all the Tracks
  // for (auto const& fTrackLabel: fTrackLabels){
  for (unsigned int i=0; i<fTrackLabels.size(); i++){

    std::string fTrackLabel = fTrackLabels.at(i);
    std::string fPFParticleLabel = fPFParticleLabels.at(i);

    art::Handle<std::vector<recob::Hit> > hitHandle;
    std::vector<art::Ptr<recob::Hit> > allHits;
    if(evt.getByLabel(fHitLabel,hitHandle))
    {art::fill_ptr_vector(allHits, hitHandle);}

    art::Handle<std::vector<recob::Track> > trackHandle;
    std::vector<art::Ptr<recob::Track> > tracks;
    if(evt.getByLabel(fTrackLabel, trackHandle))
    {art::fill_ptr_vector(tracks, trackHandle);}

    art::Handle<std::vector<recob::PFParticle> > pfpHandle;
    std::vector<art::Ptr<recob::PFParticle> > pfps;
    if(evt.getByLabel(fPFParticleLabel, pfpHandle))
    {art::fill_ptr_vector(pfps, pfpHandle);}

    trackTreeLabel = fTrackLabel;

    art::FindManyP<recob::Hit> fmTrackHit(trackHandle, evt, fTrackLabel);
    if (!fmTrackHit.isValid())
      continue;

    for (art::Ptr<recob::Track> track: tracks){

      resetTrackTree();

      recob::tracking::Point_t trackStart = track->Start();
      recob::tracking::Point_t trackEnd = track->End();

      trackStartX = trackStart.X();
      trackStartY = trackStart.Y();
      trackStartZ = trackStart.Z();
      trackEndX = trackEnd.X();
      trackEndY = trackEnd.Y();
      trackEndZ = trackEnd.Z();

      for (auto const& fT0Label: fT0Labels){
        art::FindManyP<anab::T0> fmTrackT0(trackHandle, evt, fT0Label);
        if (fmTrackT0.isValid()){
          const std::vector< art::Ptr< anab::T0 > >& T0s = fmTrackT0.at(track.key());
          if (T0s.size()!=1)
            continue;
          trackT0Map[fT0Label] = (T0s.front())->Time();
        } else {
          art::FindManyP<recob::PFParticle> fmTrackPFP(trackHandle, evt, fTrackLabel);
          art::FindManyP<anab::T0> fmPFPT0(pfpHandle, evt, fT0Label);

          if (!fmTrackPFP.isValid())
            continue;

          const std::vector< art::Ptr< recob::PFParticle > >& pfps = fmTrackPFP.at(track.key());
          if (pfps.size()!=1)
            continue;

          art::Ptr< recob::PFParticle > pfp = pfps.front();

          if (!fmPFPT0.isValid())
            continue;
          const std::vector< art::Ptr< anab::T0 > >& T0s = fmPFPT0.at(pfp.key());
          if (T0s.size()!=1)
            continue;

          trackT0Map[fT0Label] = (T0s.front())->Time();

        }
      }

      const std::vector<art::Ptr<recob::Hit> >& trackHits = fmTrackHit.at(track.key());

      int trueParticleId = RecoUtils::TrueParticleIDFromTotalTrueEnergy(trackHits);

      if (trueParticleId!=-99999){
        const simb::MCParticle* trueParticle = trueParticles.at(trueParticleId);
        trackTrueT0 = trueParticle->T();
      }
      trackTree->Fill();
    }
  }

  for (auto const& fCaloLabel: fCaloLabels){


    art::Handle<std::vector<anab::Calorimetry> > caloHandle;
    std::vector<art::Ptr<anab::Calorimetry> > allCalos;
    if(evt.getByLabel(fCaloLabel,caloHandle))
    {art::fill_ptr_vector(allCalos, caloHandle);}

    caloLabel = fCaloLabel;

    for (auto const& calo: allCalos) {

      resetCaloTree();

      calodEdxVec = calo->dEdx();
      calodQdxVec = calo->dQdx();
      caloPitchVec = calo->TrkPitchVec();
      caloRangeVec = calo->ResidualRange();

      for (auto const& point: calo->XYZ()){
        caloXVec.push_back(point.X());
        caloYVec.push_back(point.Y());
        caloZVec.push_back(point.Z());
      }
      caloTree->Fill();
    }
  }
}

void ana::SCEValidation::resetT0Tree(){
  t0TreeLabel = "";
  t0TimeVec.clear();
}

void ana::SCEValidation::resetTrackTree(){
  trackStartX = -99999;
  trackStartY = -99999;
  trackStartZ = -99999;
  trackEndX = -99999;
  trackEndY = -99999;
  trackEndZ = -99999;
  trackTrueT0 = -9999999999;
  for (auto const& fT0Label: fT0Labels){
    trackT0Map[fT0Label] = -9999999999;
  }
}

void ana::SCEValidation::resetCaloTree(){

  calodEdxVec.clear();
  calodQdxVec.clear();
  caloPitchVec.clear();
  caloRangeVec.clear();
  caloXVec.clear();
  caloYVec.clear();
  caloZVec.clear();
}

template <class T>
void ana::SCEValidation::initTree(TTree* Tree, std::string branchName,
    std::map<std::string, T>& Metric, std::vector<std::string> fPFParticleLabels){

  for (auto const& fPFParticleLabel: fPFParticleLabels){
    std::string branchString = branchName + "_" + fPFParticleLabel;
    const char* branchChar   = branchString.c_str();
    Tree->Branch(branchChar, &Metric[fPFParticleLabel], 32000, 0);
  }
}
DEFINE_ART_MODULE(ana::SCEValidation)
