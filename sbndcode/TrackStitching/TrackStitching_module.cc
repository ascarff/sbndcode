////////////////////////////////////////////////////////////////////////
// Class:       TrackStitching
// Module Type: analyzer
// File:        TrackStitching_module.cc
//
// Tom Brooks (tbrooks@fnal.gov)
////////////////////////////////////////////////////////////////////////

// LArSoft includes
#include "lardataobj/Simulation/SimChannel.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larsim/Simulation/LArG4Parameters.h"

// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Utilities/Exception.h"

// Utility libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Atom.h"
#include "cetlib/pow.h" // cet::sum_of_squares()

// ROOT includes. Note: To look up the properties of the ROOT classes,
// use the ROOT web site; e.g.,
// <https://root.cern.ch/doc/master/annotated.html>
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector3.h"

// C++ includes
#include <map>
#include <vector>
#include <string>
#include <cmath>

namespace {
  // Local namespace for local functions
  // Declare here, define later

  // Utility function to get diagonal of the detector
  double DetectorDiagonal(geo::GeometryCore const& geom);

}

namespace sbnd {
  class TrackStitching : public art::EDAnalyzer {
  public:

    // Describes configuration parameters of the module
    struct Config {
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;
 
      // One Atom for each parameter
      fhicl::Atom<art::InputTag> SimulationLabel {
        Name("SimulationLabel"),
        Comment("tag of detector simulation data product")
      };
 
      fhicl::Atom<art::InputTag> HitLabel {
        Name("HitLabel"),
        Comment("tag of the input data product with reconstructed hits")
      };
      
      fhicl::Atom<art::InputTag> TrackLabel {
        Name("TrackLabel"),
        Comment("tag of the input data product with reconstructed tracks")
        };
      
      fhicl::Atom<int> PDGcode {
        Name("PDGcode"),
        Comment("particle type (PDG ID) of the primary particle to be selected")
        };
      
    }; // Config
 
    using Parameters = art::EDAnalyzer::Table<Config>;
 
    // Constructor: configures module
    explicit TrackStitching(Parameters const& config);
 
    // Called once, at start of the job
    virtual void beginJob() override;
 
    // Called once per event
    virtual void analyze(const art::Event& event) override;

  private:

    // fcl file parameters
    art::InputTag fSimulationProducerLabel; ///< name of detsim producer
    art::InputTag fHitProducerLabel;        ///< name of hit producer
    art::InputTag fTrackProducerLabel;      ///< name of the track producer
    int fSelectedPDG;                       ///< PDG code of particle

    // Pointers to histograms
    TH1D* fPDGCodeHist;     ///< PDG code of all particles
    TH1D* fMomentumHist;    ///< momentum [GeV] of all selected particles
    TH1D* fTrackLengthHist; ///< true length [cm] of all selected particles
    TH1D* fDistToWall;      ///< distance to TPC boundary

    // The n-tuples
    TTree* fSimulationNtuple;     ///< tuple with simulated data

    /// @name the variables that will go into both n-tuples.
    /// @{
    int fEvent;  ///< event number
    int fRun;    ///< run number
    int fSubRun; ///< sub-run number
    /// @}

    /// @name The variables that will go into the simulation n-tuple.
    /// @{
    int fSimPDG;       ///< PDG ID of the particle being processed
    int fSimTrackID;   ///< GEANT ID of the particle being processed
    
    // Arrays for 4-vectors: (x,y,z,t) and (Px,Py,Pz,E).
    double fStartXYZT[4]; ///< (x,y,z,t) of the true start of the particle
    double fEndXYZT[4];   ///< (x,y,z,t) of the true end of the particle
    /// @}

    // Other variables shared between different methods.
    geo::GeometryCore const* fGeometryService;   ///< pointer to Geometry provider
  }; // class TrackStitching

  // Constructor
  TrackStitching::TrackStitching(Parameters const& config)
    : EDAnalyzer(config)
    , fSimulationProducerLabel(config().SimulationLabel())
    , fHitProducerLabel       (config().HitLabel())
    , fTrackProducerLabel     (config().TrackLabel())
    , fSelectedPDG            (config().PDGcode())
  {
    // Get a pointer to the geometry service provider
    fGeometryService = lar::providerFrom<geo::Geometry>();
    
  }

  void TrackStitching::beginJob()
  {
    // Get the detector length
    const double detectorLength = DetectorDiagonal(*fGeometryService);

    // Access tfileservice to handle creating and writing histograms
    art::ServiceHandle<art::TFileService> tfs;

    // Define histograms
    fPDGCodeHist     = tfs->make<TH1D>("pdgcodes", ";PDG Code;", 5000, -2500, 2500);
    fMomentumHist    = tfs->make<TH1D>("mom",     ";particle Momentum (GeV);",    100, 0.,    10.);
    fTrackLengthHist = tfs->make<TH1D>("length",  ";particle track length (cm);", 200, 0, detectorLength);
    fDistToWall      = tfs->make<TH1D>("dist", ";distance to wall (cm);", 200, 0, detectorLength);

    // Define n-tuples
    fSimulationNtuple = tfs->make<TTree>("TrackStitchingSimulation",    "TrackStitchingSimulation");

    // Define branches of simulation n-tuple
    fSimulationNtuple->Branch("Event",       &fEvent,          "Event/I");
    fSimulationNtuple->Branch("SubRun",      &fSubRun,         "SubRun/I");
    fSimulationNtuple->Branch("Run",         &fRun,            "Run/I");
    fSimulationNtuple->Branch("TrackID",     &fSimTrackID,     "TrackID/I");
    fSimulationNtuple->Branch("PDG",         &fSimPDG,         "PDG/I");
    fSimulationNtuple->Branch("StartXYZT",   fStartXYZT,       "StartXYZT[4]/D");
    fSimulationNtuple->Branch("EndXYZT",     fEndXYZT,         "EndXYZT[4]/D");

  } // TrackStitchingbeginJob

  void TrackStitching::analyze(const art::Event& event)
  {
    // Fetch basic event info
    fEvent  = event.id().event();
    fRun    = event.run();
    fSubRun = event.subRun();

    // Define handle to point to a vector of MCParticles
    auto particleHandle = event.getValidHandle<std::vector<simb::MCParticle>>(fSimulationProducerLabel);

    // Put MCParticles in a map for easier searching
    std::map<int, const simb::MCParticle* > particleMap;
    for (auto const& particle : (*particleHandle) ){
      fSimTrackID = particle.TrackId();
      // Use track ID as key for map
      particleMap[fSimTrackID] = &particle;

      // Histogram of PDG code
      fSimPDG = particle.PdgCode();
      fPDGCodeHist->Fill( fSimPDG );

      // Only use primary particles with matching PGD codes
      if ( particle.Process() != "primary" || fSimPDG != fSelectedPDG ) continue;

      // Get particle trajectory
      const size_t numberTrajectoryPoints = particle.NumberTrajectoryPoints();
      const int last = numberTrajectoryPoints - 1;
      const TLorentzVector& positionStart = particle.Position(0);
      const TLorentzVector& positionEnd   = particle.Position(last);
      const TLorentzVector& momentumStart = particle.Momentum(0);

      // Fill histogram with starting momentum
      fMomentumHist->Fill( momentumStart.P() );

      // Fill arrays with 4-values
      positionStart.GetXYZT( fStartXYZT );
      positionEnd.GetXYZT( fEndXYZT );

      // Track length from polar coordinate view of 4-vectors
      const double trackLength = ( positionEnd - positionStart ).Rho();

      // Print info to log file
      LOG_DEBUG("TrackStitching")
        << "Track length: " << trackLength << " cm";

      // Fill histogram with track length
      fTrackLengthHist->Fill( trackLength );

      LOG_DEBUG("TrackStitching")
        << "track ID=" << fSimTrackID 
        << " (PDG ID: " << fSimPDG << ") "
        << trackLength << " cm long, momentum " 
        << momentumStart.P() << " GeV/c, has " 
        << numberTrajectoryPoints << " trajectory points";
        
      // Write to n-tuple
      fSimulationNtuple->Fill();
        
    } // loop over all particles in the event. 
    
    // Get the hits
    art::Handle< std::vector<recob::Hit> > hitHandle;
    if (!event.getByLabel(fHitProducerLabel, hitHandle)) return;

    const art::FindManyP<simb::MCTruth> findManyTruth(particleHandle, event, fSimulationProducerLabel);

    if ( ! findManyTruth.isValid() ) {
      mf::LogError("TrackStitching") << "findManyTruth simb::MCTruth for simb::MCParticle failed!";
    }
    
    size_t particle_index = 0; // look at first particle in particleHandle's vector.
    auto const& truth = findManyTruth.at( particle_index );

    // Make sure there's no problem. 
    if ( truth.empty() ) {
      mf::LogError("TrackStitching")  
        << "Particle ID=" << particleHandle->at( particle_index ).TrackId()
        << " has no primary!";
    }

    mf::LogInfo("TrackStitching")  
      << "Particle ID=" << particleHandle->at( particle_index ).TrackId()
      << " primary PDG code=" << truth[0]->GetParticle(0).PdgCode();

    art::Handle< std::vector<recob::Track> > trackHandle;

    if (!event.getByLabel(fTrackProducerLabel, trackHandle)) return;

    const art::FindManyP<recob::Hit> findManyHits(trackHandle, event, fTrackProducerLabel);

    if ( ! findManyHits.isValid() ) {
      mf::LogError("TrackStitching")  
        << "findManyHits recob::Hit for recob::Track failed;"
        << " track label='" << fTrackProducerLabel << "'";
    }

    for ( size_t track_index = 0; track_index != trackHandle->size(); track_index++ ) {
      auto const& hits = findManyHits.at( track_index );

      mf::LogInfo("TrackStitching")  
        << "Track ID=" << trackHandle->at( track_index ).ID()
        << " has " << hits.size() << " hits";
    }

  } // TrackStitching::analyze()

  DEFINE_ART_MODULE(TrackStitching)
} // namespace sbnd

// Back to our local namespace.
namespace {

  // Define a local function to calculate the detector diagonal.
  double DetectorDiagonal(geo::GeometryCore const& geom) {
    const double length = geom.DetLength();
    const double width = 2. * geom.DetHalfWidth();
    const double height = 2. * geom.DetHalfHeight();
    
    return std::sqrt(cet::sum_of_squares(length, width, height));
  } // DetectorDiagonal()

} // local namespace


