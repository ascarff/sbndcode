//STL
#include <iostream>
//ROOT
//ART
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

//LARSOFT
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/Vertex.h"
#include "lardataobj/AnalysisBase/MVAPIDResult.h"
#include "lardataobj/RecoBase/Hit.h"

//CUSTOM
#include "sbndcode/RecoUtils/RecoUtils.h"


namespace sbnd{
  class Pandizzle {
    public:
      Pandizzle(fhicl::ParameterSet const &p);
      //Pandizzle(const art::Ptr<recob::PFParticle> pfp, const std::vector<art::Ptr<recob::PFParticle> > & pfp_vector, const art::FindManyP<recob::Track> & fmTrackFromPFP, const art::FindManyP<recob::Shower> & fmShowerFromPFP, const art::FindManyP<anab::MVAPIDResult> & fmPIDFromTrack, const art::FindManyP<anab::MVAPIDResult> & fmPIDFromShower);
      void Assess(const art::Ptr<recob::PFParticle> pfparticle, const art::Event & event);
      void Reset();
      bool IsTrackLike() { return fIsTrackLike; };
      bool IsShowerLike() { return fIsShowerLike; };
      double GetLength() { return fLength; };
      std::vector<art::Ptr<recob::Hit> > GetHits() { return fHits; };
    private:
      void AssessAsTrack(const art::Ptr<recob::PFParticle> pfparticle, const art::Ptr<recob::Track> track, const art::Event & event);
      void AssessAsShower(const art::Ptr<recob::PFParticle> pfparticle, const art::Ptr<recob::Shower> shower, const art::Event & event);
      bool fIsTrackLike;
      bool fIsShowerLike;
      double fLength;

      std::vector<art::Ptr<recob::Hit> > fHits;

      //fcl labels
      std::string fPFParticleModuleLabel;
      std::string fTrackModuleLabel;
      std::string fShowerModuleLabel;
      std::string fPIDModuleLabel;
      std::string fVertexModuleLabel;
      std::string fLArGeantModuleLabel;

  };
}
