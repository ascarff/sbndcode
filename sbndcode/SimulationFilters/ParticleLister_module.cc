/////////////////////////////////////////////////////////////////////////
// Class:       ParticleLister
// Module Type: filter
// File:        ParticleLister_module.cc
//
// Prints out a list of the truth-level particles in MC (simb::MCParticle).
// Since this functions as a filter that simply passes all events, it can 
// simply be placed in-line in a simulation chain following the largeant 
// stage in the fhicl file - for example:
//
// simulate: [ rns, generator, largeant, particlelister ]
//
// By default, only the first 20 events are shown.
//  
// April 2020
// W. Foreman
////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <algorithm>

//Art Framework Includes
#include "art/Framework/Core/EDFilter.h" 
#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Principal/Event.h" 

//LArSoft Includes
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "larcore/Geometry/AuxDetGeometry.h"
#include "nusimdata/SimulationBase/MCTruth.h"

//ROOT Includes
#include "TGeoManager.h"
#include "TVector3.h"
#include "TLorentzVector.h"

namespace filt{

  class ParticleLister : public art::EDFilter {
    public:
      explicit ParticleLister(fhicl::ParameterSet const & p);
      void reconfigure(fhicl::ParameterSet const & p);
      virtual bool filter(art::Event & e) override;
 
    private:
      geo::GeometryCore   *fGeometry;
      std::string         fSimProducerLabel;
      int                 fMaxEvents;

      int                 eventCounter;
  };


  ParticleLister::ParticleLister(fhicl::ParameterSet const & pset)
  : EDFilter(pset)
  {
    // Read in fhicl parameters
    this->reconfigure(pset);
    
    // Get a pointer to the geometry service provider
    fGeometry = &*(art::ServiceHandle<geo::Geometry>());
  }


  void ParticleLister::reconfigure(fhicl::ParameterSet const& pset){
    fSimProducerLabel       = pset.get< std::string > ("SimProducer","largeant");
    fMaxEvents              = pset.get< int >         ("MaxEvents",20);
    eventCounter            = 0; 
  }


  bool ParticleLister::filter(art::Event & e){

    eventCounter++;

    if( eventCounter < fMaxEvents ) {

      // Get MCParticles from event
      art::Handle< std::vector<simb::MCParticle> > particleHandle;
      e.getByLabel(fSimProducerLabel, particleHandle);
   
      // Loop through particles and look for primary muon
      for( auto const& particle : (*particleHandle) ){
        int trackId = particle.TrackId();
        int pdg     = particle.PdgCode(); 
  
        // A particle has a trajectory, consisting of a set of
        // 4-positions and 4-mommenta.
        size_t  numTrajPoints       = particle.NumberTrajectoryPoints();
        int	    last	        = numTrajPoints - 1;
        const TLorentzVector& loc0  = particle.Position(0);
        const TLorentzVector& loc   = particle.Position(last);
        double dL                   = (loc0.Vect()-loc.Vect()).Mag();
        //float P0                    = particle.P(0)*1000.;
        //float Pf                    = particle.P(last)*1000.;
        float E0                    = particle.E(0)*1000.;
        float Ef                    = particle.E(last)*1000.;
        float K0                    = (particle.E(0)-particle.Mass())*1000.;
        float Kf                    = (particle.E(last)-particle.Mass())*1000.;
        //bool isFromPbar = isParticleDescendedFrom(particleHandle,trackId,1);
        std::string proc            = particle.Process().c_str();
          
        printf("  %3i PDG: %10i   dL=%6.1fcm   E0=%9.3f  Ef=%9.3f  KE0=%9.3f  KEf=%9.3f  T=%8.2f-%8.2f   mother=%3i %22s   Ndaught=%i\n",
            trackId,
            pdg,
            dL,
            E0,
            Ef,
            K0,
            Kf,
            particle.T(0),
            particle.T(last),
            particle.Mother(),
            particle.Process().c_str(),
            particle.NumberDaughters()
            );
          
      } // end particle loop
    }//endif max events
    
    return true;
  }

  DEFINE_ART_MODULE(ParticleLister)

}
