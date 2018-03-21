//STL
#include <iostream>
//ROOT
//ART
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/FindManyP.h"

//LARSOFT
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/AnalysisBase/MVAPIDResult.h"

//CUSTOM


namespace sbnd{
  class Pandizzle {
    public:
      Pandizzle(const art::Ptr<recob::PFParticle> pfp, const std::vector<art::Ptr<recob::PFParticle> > & pfp_vector, const art::FindManyP<recob::Track> & fmTrackFromPFP, const art::FindManyP<recob::Shower> & fmShowerFromPFP, const art::FindManyP<anab::MVAPIDResult> & fmPIDFromTrack, const art::FindManyP<anab::MVAPIDResult> & fmPIDFromShower);
      void Test();
    private:
  };
}
