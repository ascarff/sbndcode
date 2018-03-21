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

//Custom
#include "Pandizzle.h"

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

  // Declare member data here.
  std::string fPFParticleModuleLabel;
  std::string fTrackModuleLabel;
  std::string fShowerModuleLabel;
  std::string fPIDModuleLabel;

};


PandizzleTreeMaker::PandizzleTreeMaker(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p),
  fPFParticleModuleLabel       (p.get<std::string>("PFParticleModuleLabel")),
  fTrackModuleLabel        (p.get<std::string>("TrackModuleLabel")),
  fShowerModuleLabel        (p.get<std::string>("ShowerModuleLabel")),
  fPIDModuleLabel        (p.get<std::string>("PIDModuleLabel"))
{}

void PandizzleTreeMaker::analyze(art::Event const & e)
{

  //PFP
  art::Handle<std::vector<recob::PFParticle> > pfParticleListHandle;
  std::vector<art::Ptr<recob::PFParticle> > pfParticleList;
  if (e.getByLabel(fPFParticleModuleLabel, pfParticleListHandle)) art::fill_ptr_vector(pfParticleList, pfParticleListHandle);

  //Track
  art::Handle<std::vector<recob::Track> > trackListHandle;
  std::vector<art::Ptr<recob::Track> > trackList;
  if (e.getByLabel(fTrackModuleLabel, trackListHandle)) art::fill_ptr_vector(trackList, trackListHandle);

  //Shower
  art::Handle<std::vector<recob::Shower> > showerListHandle;
  std::vector<art::Ptr<recob::Shower> > showerList;
  if (e.getByLabel(fShowerModuleLabel, showerListHandle)) art::fill_ptr_vector(showerList, showerListHandle);


  art::FindManyP<recob::Track> fmTrackFromPFP(pfParticleListHandle,e,fTrackModuleLabel);
  art::FindManyP<recob::Shower> fmShowerFromPFP(pfParticleListHandle,e,fShowerModuleLabel);
  art::FindManyP<anab::MVAPIDResult> fmPIDFromTrack(trackListHandle,e,fTrackModuleLabel);
  art::FindManyP<anab::MVAPIDResult> fmPIDFromShower(showerListHandle,e,fShowerModuleLabel);



  for (unsigned int i_pfp = 0; i_pfp < pfParticleList.size(); i_pfp++){
    art::Ptr<recob::PFParticle> pfParticle = pfParticleList[i_pfp];
    sbnd::Pandizzle pandizzler(pfParticle,pfParticleList,fmTrackFromPFP, fmShowerFromPFP, fmPIDFromTrack, fmPIDFromShower);
  }
}

DEFINE_ART_MODULE(PandizzleTreeMaker)
