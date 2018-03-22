////////////////////////////////////////////////////////////////////////
// Class:       PandizzleTreeMaker
// Plugin Type: analyzer (art v2_10_03)
// File:        PandizzleTreeMaker_module.cc
//
// Generated at Wed Mar 21 12:07:24 2018 by Dominic Brailsford using cetskelgen
// from cetlib version v3_02_00.
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
#include "canvas/Persistency/Common/FindManyP.h"
#include "lardataobj/RecoBase/Hit.h"

//Custom
#include "Pandizzle.h"
#include "sbndcode/RecoUtils/RecoUtils.h"

class PandizzleTreeMaker;


class PandizzleTreeMaker : public art::EDAnalyzer {
public:
  explicit PandizzleTreeMaker(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  PandizzleTreeMaker(PandizzleTreeMaker const &) = delete;
  PandizzleTreeMaker(PandizzleTreeMaker &&) = delete;
  PandizzleTreeMaker & operator = (PandizzleTreeMaker const &) = delete;
  PandizzleTreeMaker & operator = (PandizzleTreeMaker &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;

private:

  sbnd::Pandizzle fPandizzle;

  // Declare member data here.
  std::string fPFParticleModuleLabel;
  std::string fTrackModuleLabel;
  std::string fShowerModuleLabel;
  std::string fPIDModuleLabel;
  std::string fVertexModuleLabel;
  std::string fLArGeantModuleLabel;

};


PandizzleTreeMaker::PandizzleTreeMaker(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p),
  fPandizzle(p),
  fPFParticleModuleLabel       (p.get<std::string>("PFParticleModuleLabel")),
  fTrackModuleLabel        (p.get<std::string>("TrackModuleLabel")),
  fShowerModuleLabel        (p.get<std::string>("ShowerModuleLabel")),
  fPIDModuleLabel        (p.get<std::string>("PIDModuleLabel")),
  fVertexModuleLabel     (p.get<std::string>("VertexModuleLabel")),
  fLArGeantModuleLabel   (p.get<std::string>("LArGeantModuleLabel"))
{}

void PandizzleTreeMaker::analyze(art::Event const & e)
{

  //MCParticle
  art::Handle<std::vector<simb::MCParticle> > mcParticleListHandle;
  std::vector<art::Ptr<simb::MCParticle> > mcParticleList;
  if (e.getByLabel(fLArGeantModuleLabel, mcParticleListHandle)) art::fill_ptr_vector(mcParticleList, mcParticleListHandle);


  //PFP
  art::Handle<std::vector<recob::PFParticle> > pfParticleListHandle;
  std::vector<art::Ptr<recob::PFParticle> > pfParticleList;
  if (e.getByLabel(fPFParticleModuleLabel, pfParticleListHandle)) art::fill_ptr_vector(pfParticleList, pfParticleListHandle);

  ////Track
  //art::Handle<std::vector<recob::Track> > trackListHandle;
  //std::vector<art::Ptr<recob::Track> > trackList;
  //if (e.getByLabel(fTrackModuleLabel, trackListHandle)) art::fill_ptr_vector(trackList, trackListHandle);

  ////Shower
  //art::Handle<std::vector<recob::Shower> > showerListHandle;
  //std::vector<art::Ptr<recob::Shower> > showerList;
  //if (e.getByLabel(fShowerModuleLabel, showerListHandle)) art::fill_ptr_vector(showerList, showerListHandle);


  //art::FindManyP<recob::Track> fmTrackFromPFP(pfParticleListHandle,e,fTrackModuleLabel);
  //art::FindManyP<recob::Shower> fmShowerFromPFP(pfParticleListHandle,e,fShowerModuleLabel);

  //art::FindManyP<recob::Vertex> fmVertexFromPFP(pfParticleListHandle,e,fPFParticleModuleLabel);

  //art::FindManyP<anab::MVAPIDResult> fmPIDFromTrack(trackListHandle,e,fTrackModuleLabel);
  //art::FindManyP<anab::MVAPIDResult> fmPIDFromShower(showerListHandle,e,fShowerModuleLabel);

  //art::FindManyP<recob::Hit> fmHitFromTrack(trackListHandle,e,fTrackModuleLabel);
  //art::FindManyP<recob::Hit> fmHitFromShower(showerListHandle,e,fShowerModuleLabel);



  std::cout<<"Event: " << e.event() <<"  NPFP: " << pfParticleList.size() << std::endl;
  for (unsigned int i_pfp = 0; i_pfp < pfParticleList.size(); i_pfp++){
    art::Ptr<recob::PFParticle> pfParticle = pfParticleList[i_pfp];
    fPandizzle.Assess(pfParticle,e);
    //art::Ptr<recob::Track> track;
    //art::Ptr<recob::Shower> shower;
    //std::vector<art::Ptr<recob::Vertex> > vertices;
    //std::vector<art::Ptr<recob::Hit> > hits;
    //if (fmTrackFromPFP.at(pfParticle.key()).size() > 0){
    //  track = fmTrackFromPFP.at(pfParticle.key())[0];
    //  hits = fmHitFromTrack.at(track.key());
    //}
    //else if (fmShowerFromPFP.at(pfParticle.key()).size() > 0) {
    //  shower = fmShowerFromPFP.at(pfParticle.key())[0];
    //  hits = fmHitFromShower.at(shower.key());
    //}

    //vertices = fmVertexFromPFP.at(pfParticle.key());

    //int g4id = RecoUtils::TrueParticleIDFromTotalRecoHits(hits);
    //art::Ptr<simb::MCParticle> particle;
    //for (unsigned int i_mcp = 0; i_mcp < mcParticleList.size(); i_mcp++){
    //  if (mcParticleList[i_mcp]->TrackId()==g4id){
    //    particle=mcParticleList[i_mcp];
    //    break;
    //  }
    //}
    //int truepdg = -9999;
    //if (particle.isAvailable()) truepdg = particle->PdgCode();
    //std::cout<<"PDG: " << pfParticle->PdgCode() << " ID: " << pfParticle->Self() << "  Parent: " << pfParticle->Parent() << "  IsPrim: " << pfParticle->IsPrimary() << "  IsTrack: " << track.isAvailable() << " IsShower: " << shower.isAvailable() << "  NVertices: " << vertices.size() << " NHits: " << hits.size() << " TruePDG: " << truepdg << std::endl;


    //bnd::Pandizzle pandizzler(pfParticle,pfParticleList,fmTrackFromPFP, fmShowerFromPFP, fmPIDFromTrack, fmPIDFromShower);
  }
}

DEFINE_ART_MODULE(PandizzleTreeMaker)
