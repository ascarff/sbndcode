#include "Pandizzle.h"

/*
sbnd::Pandizzle::Pandizzle::Pandizzle(const art::Ptr<recob::PFParticle> pfp, const std::vector<art::Ptr<recob::PFParticle> > & pfp_vector, const art::FindManyP<recob::Track> & fmTrackFromPFP, const art::FindManyP<recob::Shower> & fmShowerFromPFP, const art::FindManyP<anab::MVAPIDResult> & fmPIDFromTrack, const art::FindManyP<anab::MVAPIDResult> & fmPIDFromShower){
}
*/
sbnd::Pandizzle::Pandizzle::Pandizzle(fhicl::ParameterSet const & p)
  :
  fPFParticleModuleLabel       (p.get<std::string>("PFParticleModuleLabel")),
  fTrackModuleLabel        (p.get<std::string>("TrackModuleLabel")),
  fShowerModuleLabel        (p.get<std::string>("ShowerModuleLabel")),
  fPIDModuleLabel        (p.get<std::string>("PIDModuleLabel")),
  fVertexModuleLabel     (p.get<std::string>("VertexModuleLabel")),
  fLArGeantModuleLabel   (p.get<std::string>("LArGeantModuleLabel"))
{}

void sbnd::Pandizzle::Assess(const art::Ptr<recob::PFParticle> pfparticle, const art::Event & event){
  return;
}


void sbnd::Pandizzle::Test(){
  std::cout<<"Test function runs"<<std::endl;
  return;
}
