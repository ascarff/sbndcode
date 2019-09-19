////////////////////////////////////////////////////////////////////////
// Class:       MichelElectron
// Plugin Type: analyzer (art v3_02_06)
// File:        MichelElectron_module.cc
//
// Generated at Thu Sep 19 12:15:48 2019 by Iker De-icaza-astiz using cetskelgen
// from cetlib version v3_07_02.
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


#include "larcore/Geometry/Geometry.h"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardataobj/RecoBase/OpHit.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"

class MichelElectron;


class MichelElectron : public art::EDAnalyzer {
public:
  explicit MichelElectron(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  MichelElectron(MichelElectron const&) = delete;
  MichelElectron(MichelElectron&&) = delete;
  MichelElectron& operator=(MichelElectron const&) = delete;
  MichelElectron& operator=(MichelElectron&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

  // Selected optional functions.
  void beginJob() override;
  void endJob() override;
  void reconfigure(fhicl::ParameterSet const & p);

private:

  // Declare member data here.
  /******************************************
   *  LABELS                                *
   ******************************************/
  std::string TruthLabel, G4Label, ParticleLabel, HitFinderLabel, RecoTrackLabel, RecoShowerLabel, RecoPIDLabel, RecoCaloLabel, photonLabel ; 

  int event_id;
};


MichelElectron::MichelElectron(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
  TruthLabel = p.get<std::string>("TruthLabel");
  G4Label = p.get<std::string>("G4Label");
  ParticleLabel = p.get<std::string>("PFParticleModule","pandora");
  HitFinderLabel = p.get<std::string>("HitFinderModule","linecluster");
  RecoTrackLabel = p.get<std::string>("RecoTrackLabel","pandoraTrack");
  RecoShowerLabel = p.get<std::string>("RecoShowerLabel","emshower");
  RecoCaloLabel = p.get<std::string>("RecoCaloLabel","pandoraCalo");
  RecoPIDLabel = p.get<std::string>("RecoPIDLabel","pandoraPid");

  photonLabel = p.get<std::string>("photonLabel","g4Label");
  
  this->reconfigure(p);
}

void MichelElectron::analyze(art::Event const& e)
{
  // Implementation of required member function here.

  event_id = e.id().event();

  if( !e.isRealData()){

    art::Handle<std::vector<raw::OpDetWaveform>> wvFormsHandle;

    e.getByLabel(photonLabel, wvFormsHandle ) ;

  }
}

void MichelElectron::beginJob()
{
  // Implementation of optional member function here.

  event_id = -999;
}

void MichelElectron::endJob()
{
  // Implementation of optional member function here.
}

void MichelElectron::reconfigure(fhicl::ParameterSet const & p)
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(MichelElectron)
