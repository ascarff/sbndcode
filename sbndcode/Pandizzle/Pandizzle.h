//STL
#include <iostream>
//ROOT
//ART
#include "canvas/Persistency/Common/Ptr.h"

//LARSOFT
#include "lardataobj/RecoBase/PFParticle.h"

//CUSTOM


namespace sbnd{
  class Pandizzle {
    public:
      Pandizzle(const art::Ptr<recob::PFParticle> pfp, std::vector<art::Ptr<recob::PFParticle> > const & pfp_vector);
      void Test();
    private:
  };
}
