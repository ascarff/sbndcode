////////////////////////////////////////////////////////////////////////
// Class:       CheatT0Tagger
// Plugin Type: producer (art v3_04_00)
// File:        CheatT0Tagger_module.cc
//
// Generated at Tue Mar 24 15:57:11 2020 by Edward Tyley using cetskelgen
// from cetlib version v3_09_00.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "lardata/Utilities/AssociationUtil.h"

#include "larsim/MCCheater/ParticleInventoryService.h"
#include "lardataobj/AnalysisBase/T0.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Hit.h"
#include "larreco/RecoAlg/MCRecoUtils/RecoUtils.h"

#include <memory>

namespace cheat {
  class CheatT0Tagger;
}


class cheat::CheatT0Tagger : public art::EDProducer {
  public:
    explicit CheatT0Tagger(fhicl::ParameterSet const& p);
    // The compiler-generated destructor is fine for non-base
    // classes without bare pointers or other resource use.

    // Plugins should not be copied or assigned.
    CheatT0Tagger(CheatT0Tagger const&) = delete;
    CheatT0Tagger(CheatT0Tagger&&) = delete;
    CheatT0Tagger& operator=(CheatT0Tagger const&) = delete;
    CheatT0Tagger& operator=(CheatT0Tagger&&) = delete;

    // Required functions.
    void produce(art::Event& e) override;

  private:

    // Declare member data here.
    std::string fPandoraLabel;

    art::ServiceHandle<cheat::ParticleInventoryService> particleInventory;

};


cheat::CheatT0Tagger::CheatT0Tagger(fhicl::ParameterSet const& p)
  : EDProducer{p}  // ,
  // More initializers here.
{
  // Call appropriate produces<>() functions here.
  // Call appropriate consumes<>() for any products to be retrieved by this module.
  produces<std::vector<anab::T0> >();
  produces<art::Assns<recob::PFParticle, anab::T0> >();

  fPandoraLabel = p.get<std::string>("PandoraLabel");
}

void cheat::CheatT0Tagger::produce(art::Event& evt)
{

  auto t0Collection   = std::make_unique<std::vector<anab::T0> >();
  auto pfpT0Assn      = std::make_unique<art::Assns<recob::PFParticle, anab::T0> >();
  art::PtrMaker<anab::T0> t0PtrMaker{evt};

  // Implementation of required member function here.
  std::map<int,const simb::MCParticle*> trueParticles;
  const sim::ParticleList& particles = particleInventory->ParticleList();
  for (auto const& particleIt: particles){
    const simb::MCParticle* particle = particleIt.second;
    trueParticles[particle->TrackId()] = particle;
  }

  art::Handle<std::vector<recob::PFParticle> > pfpHandle;
  std::vector<art::Ptr<recob::PFParticle> > pfps;
  if(evt.getByLabel(fPandoraLabel, pfpHandle))
  {art::fill_ptr_vector(pfps, pfpHandle);}

  art::Handle<std::vector<recob::Cluster> > clusterHandle;
  std::vector<art::Ptr<recob::Cluster> > clusters;
  if(evt.getByLabel(fPandoraLabel, clusterHandle))
  {art::fill_ptr_vector(clusters, clusterHandle);}

  art::FindManyP<recob::Cluster> fmPFPCluster(pfpHandle, evt, fPandoraLabel);
  art::FindManyP<recob::Hit> fmClusterHit(clusterHandle, evt, fPandoraLabel);

  if (!fmPFPCluster.isValid() || !fmClusterHit.isValid())
    return;

  for (auto const& pfp: pfps){

    std::vector<art::Ptr<recob::Hit> > pfpHits;
    const std::vector< art::Ptr< recob::Cluster> >& clusters = fmPFPCluster.at(pfp.key());
    for (const auto& cluster: clusters){
      const std::vector< art::Ptr< recob::Hit> >& hits = fmClusterHit.at(cluster.key());
      pfpHits.insert(pfpHits.end(), hits.begin(), hits.end());
    }
    int trueParticleId = RecoUtils::TrueParticleIDFromTotalTrueEnergy(pfpHits);

    if (trueParticleId!=-99999){
      const simb::MCParticle* trueParticle = trueParticles.at(trueParticleId);
      double trueT0 = trueParticle->T();

      anab::T0 pfpT0(trueT0, 2, trueParticleId);

      t0Collection->push_back(pfpT0);
      art::Ptr<anab::T0> pfpT0Ptr = t0PtrMaker(t0Collection->size()-1);
      pfpT0Assn->addSingle(pfp, pfpT0Ptr);

    }
  }
  evt.put(std::move(t0Collection));
  evt.put(std::move(pfpT0Assn));
}

DEFINE_ART_MODULE(cheat::CheatT0Tagger)
