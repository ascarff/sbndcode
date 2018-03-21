//STL
#include <iostream>
//ROOT
//ART
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/FindManyP.h"

//LARSOFT
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Track.h"

//CUSTOM


namespace sbnd{
  class Pandizzle {
    public:
      Pandizzle(const art::Ptr<recob::PFParticle> pfp, const std::vector<art::Ptr<recob::PFParticle> > & pfp_vector, const art::FindManyP<recob::Track> & fmTrackFromPFP);
      void Test();
    private:
  };
}
