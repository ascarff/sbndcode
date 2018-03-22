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
{
  Reset();
}

void sbnd::Pandizzle::Assess(const art::Ptr<recob::PFParticle> pfparticle, const art::Event & event){
  //Reset everything
  Reset();

  //Get the containers
  //PFP
  art::Handle<std::vector<recob::PFParticle> > pfParticleListHandle;
  std::vector<art::Ptr<recob::PFParticle> > pfParticleList;
  if (event.getByLabel(fPFParticleModuleLabel, pfParticleListHandle)) art::fill_ptr_vector(pfParticleList, pfParticleListHandle);

  ////Track
  //art::Handle<std::vector<recob::Track> > trackListHandle;
  //std::vector<art::Ptr<recob::Track> > trackList;
  //if (e.getByLabel(fTrackModuleLabel, trackListHandle)) art::fill_ptr_vector(trackList, trackListHandle);

  ////Shower
  //art::Handle<std::vector<recob::Shower> > showerListHandle;
  //std::vector<art::Ptr<recob::Shower> > showerList;
  //if (e.getByLabel(fShowerModuleLabel, showerListHandle)) art::fill_ptr_vector(showerList, showerListHandle);


  //Get the assns
  art::FindManyP<recob::Track> fmTrackFromPFP(pfParticleListHandle,event,fTrackModuleLabel);
  art::FindManyP<recob::Shower> fmShowerFromPFP(pfParticleListHandle,event,fShowerModuleLabel);

  /*
  art::FindManyP<anab::MVAPIDResult> fmPIDFromTrack(trackListHandle,e,fTrackModuleLabel);
  art::FindManyP<anab::MVAPIDResult> fmPIDFromShower(showerListHandle,e,fShowerModuleLabel);

  art::FindManyP<recob::Hit> fmHitFromTrack(trackListHandle,e,fTrackModuleLabel);
  art::FindManyP<recob::Hit> fmHitFromShower(showerListHandle,e,fShowerModuleLabel);
  */


  art::Ptr<recob::Track> track;
  art::Ptr<recob::Shower> shower;
  //std::vector<art::Ptr<recob::Hit> > hits;

  fIsTrackLike = false;
  fIsShowerLike = false;

  if (fmTrackFromPFP.at(pfparticle.key()).size() > 0){
    track = fmTrackFromPFP.at(pfparticle.key())[0];
    //hits = fmHitFromTrack.at(track.key());
    fIsTrackLike=true;
  }
  if (fmShowerFromPFP.at(pfparticle.key()).size() > 0) {
    shower = fmShowerFromPFP.at(pfparticle.key())[0];
    //hits = fmHitFromShower.at(shower.key());
    fIsShowerLike=true;
  }

  if (!fIsTrackLike && !fIsShowerLike){
    std::cout<<"Pandizzle::Assess - Found a PFP which is neither track-like or shower-like.  There is nothing to do here.  Abort." << std::endl;
    return;
  } 
  else if (fIsTrackLike && fIsShowerLike){
    std::cout<<"Pandizzle::Assess - Found a PFP which is both track-like and shower-like.  There is nothing to do here.  Abort." << std::endl;
    return;
  }
  else if (fIsTrackLike && !fIsShowerLike){
    AssessAsTrack(pfparticle,track,event);
  }
  else{
    AssessAsShower(pfparticle,shower,event);
  }


  return;
}

void sbnd::Pandizzle::Reset(){
  int kDefInt = -9999;
  double kDefDoub = (double)(kDefInt);

  fLength = kDefDoub;
  fIsTrackLike = false;
  fIsShowerLike = false;
  fHits.clear();
  return;
}


void sbnd::Pandizzle::AssessAsTrack(const art::Ptr<recob::PFParticle> pfparticle, const art::Ptr<recob::Track> track, const art::Event & event){
  //Calculate the length
  fLength = RecoUtils::CalculateTrackLength(track);

  //Get the hits
  art::Handle<std::vector<recob::Track> > trackListHandle;
  std::vector<art::Ptr<recob::Track> > trackList;
  if (event.getByLabel(fTrackModuleLabel, trackListHandle)) art::fill_ptr_vector(trackList, trackListHandle);
  art::FindManyP<recob::Hit> fmHitFromTrack(trackListHandle,event,fTrackModuleLabel);
  fHits = fmHitFromTrack.at(track.key());

  return;
}

void sbnd::Pandizzle::AssessAsShower(const art::Ptr<recob::PFParticle> pfparticle, const art::Ptr<recob::Shower> shower, const art::Event & event){
  //Get the length
  fLength = 0;

  //Get the hits
  art::Handle<std::vector<recob::Shower> > showerListHandle;
  std::vector<art::Ptr<recob::Shower> > showerList;
  if (event.getByLabel(fShowerModuleLabel, showerListHandle)) art::fill_ptr_vector(showerList, showerListHandle);
  art::FindManyP<recob::Hit> fmHitFromShower(showerListHandle,event,fShowerModuleLabel);
  fHits = fmHitFromShower.at(shower.key());

  return;
}

