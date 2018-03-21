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
  std::string fPFParticleLabel;

};


PandizzleTreeMaker::PandizzleTreeMaker(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p),
  fPFParticleLabel       (p.get<std::string>("PFParticleLabel"))
 // More initializers here.
{}

void PandizzleTreeMaker::analyze(art::Event const & e)
{

  art::Handle<std::vector<recob::PFParticle> > pfParticleListHandle;
  std::vector<art::Ptr<recob::PFParticle> > pfParticleList;
  if (e.getByLabel(fPFParticleLabel, pfParticleListHandle)) art::fill_ptr_vector(pfParticleList, pfParticleListHandle);

  for (unsigned int i_pfp = 0; i_pfp < pfParticleList.size(); i_pfp++){
    art::Ptr<recob::PFParticle> pfParticle = pfParticleList[i_pfp];
    sbnd::Pandizzle pandizzler(pfParticle,pfParticleList);
  }
}

DEFINE_ART_MODULE(PandizzleTreeMaker)
