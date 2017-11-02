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
#include "lardata/DetectorInfoServices/DetectorClocksService.h"
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

  // Comparison routne for using lower/upper_bound to search TDCIDE vecotrs.
  bool TDCIDETimeCompare(const sim::TDCIDE&, const sim::TDCIDE&);

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
      
      fhicl::Atom<double> BinSize {
        Name("BinSize"),
        Comment("dx [cm] used for the dE/dx calculation")
        };
    }; // Config
 
    using Parameters = art::EDAnalyzer::Table<Config>;
 
    // Constructor: configures module
    explicit TrackStitching(Parameters const& config);
 
    // Called once, at start of the job
    virtual void beginJob() override;
 
    // Called at the start of each run
    virtual void beginRun(const art::Run& run) override;
 
    // Called once per event
    virtual void analyze(const art::Event& event) override;

  private:

    // fcl file parameters
    art::InputTag fSimulationProducerLabel; ///< name of detsim producer
    art::InputTag fHitProducerLabel;        ///< name of hit producer
    art::InputTag fTrackProducerLabel;      ///< name of the track producer
    int fSelectedPDG;                       ///< PDG code of particle
    double fBinSize;                        ///< Bin size of dE/dx hist

    // Pointers to histograms
    TH1D* fPDGCodeHist;     ///< PDG code of all particles
    TH1D* fMomentumHist;    ///< momentum [GeV] of all selected particles
    TH1D* fTrackLengthHist; ///< true length [cm] of all selected particles

    // The n-tuples
    TTree* fSimulationNtuple;     ///< tuple with simulated data
    TTree* fReconstructionNtuple; ///< tuple with reconstructed data

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
    double fStartPE[4];   ///< (Px,Py,Pz,E) at the true start of the particle
    double fEndPE[4];     ///< (Px,Py,Pz,E) at the true end of the particle
    
    /// Number of dE/dx bins in a given track.
    int fSimNdEdxBins;
    
    /// The vector that will be used to accumulate dE/dx values as a function of range.
    std::vector<double> fSimdEdxBins;
    /// @}

    /// @name Variables used in the reconstruction n-tuple
    /// @{
    int fRecoPDG;       ///< PDG ID of the particle being processed
    int fRecoTrackID;   ///< GEANT ID of the particle being processed
    
    /// Number of dE/dx bins in a given track.
    int fRecoNdEdxBins;
    
    /// The vector that will be used to accumulate dE/dx values as a function of range.
    std::vector<double> fRecodEdxBins;
    /// @}

    // Other variables shared between different methods.
    geo::GeometryCore const* fGeometryService;   ///< pointer to Geometry provider
    detinfo::DetectorClocks const* fTimeService; ///< pointer to detector clock time service provider
    double fElectronsToGeV;                      ///< conversion factor
    int fTriggerOffset;                          ///< (unit of ticks) time of neutrino event

  }; // class TrackStitching

  // Constructor
  TrackStitching::TrackStitching(Parameters const& config)
    : EDAnalyzer(config)
    , fSimulationProducerLabel(config().SimulationLabel())
    , fHitProducerLabel       (config().HitLabel())
    , fTrackProducerLabel     (config().TrackLabel())
    , fSelectedPDG            (config().PDGcode())
    , fBinSize                (config().BinSize())
  {
    // Get a pointer to the geometry service provider
    fGeometryService = lar::providerFrom<geo::Geometry>();
    // Get a pointer to the detector clock service
    fTimeService = lar::providerFrom<detinfo::DetectorClocksService>();
    // Access to detector properties
    const detinfo::DetectorProperties* detprop = lar::providerFrom<detinfo::DetectorPropertiesService>();
    fTriggerOffset = detprop->TriggerOffset();
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

    // Define n-tuples
    fSimulationNtuple = tfs->make<TTree>("TrackStitchingSimulation",    "TrackStitchingSimulation");
    fReconstructionNtuple = tfs->make<TTree>("TrackStitchingReconstruction","TrackStitchingReconstruction");

    // Define branches of simulation n-tuple
    fSimulationNtuple->Branch("Event",       &fEvent,          "Event/I");
    fSimulationNtuple->Branch("SubRun",      &fSubRun,         "SubRun/I");
    fSimulationNtuple->Branch("Run",         &fRun,            "Run/I");
    fSimulationNtuple->Branch("TrackID",     &fSimTrackID,     "TrackID/I");
    fSimulationNtuple->Branch("PDG",         &fSimPDG,         "PDG/I");
    fSimulationNtuple->Branch("StartXYZT",   fStartXYZT,       "StartXYZT[4]/D");
    fSimulationNtuple->Branch("EndXYZT",     fEndXYZT,         "EndXYZT[4]/D");
    fSimulationNtuple->Branch("StartPE",     fStartPE,         "StartPE[4]/D");
    fSimulationNtuple->Branch("EndPE",       fEndPE,           "EndPE[4]/D");
    fSimulationNtuple->Branch("NdEdx",       &fSimNdEdxBins,   "NdEdx/I");
    fSimulationNtuple->Branch("dEdx",        &fSimdEdxBins);

    // Define branches of reconstruction n-tuple
    fReconstructionNtuple->Branch("Event",   &fEvent,          "Event/I");
    fReconstructionNtuple->Branch("SubRun",  &fSubRun,         "SubRun/I");
    fReconstructionNtuple->Branch("Run",     &fRun,            "Run/I");
    fReconstructionNtuple->Branch("TrackID", &fRecoTrackID,    "TrackID/I");
    fReconstructionNtuple->Branch("PDG",     &fRecoPDG,        "PDG/I");
    fReconstructionNtuple->Branch("NdEdx",   &fRecoNdEdxBins,  "NdEdx/I");
    fReconstructionNtuple->Branch("dEdx",    &fRecodEdxBins);

  } // TrackStitchingbeginJob

  void TrackStitching::beginRun(const art::Run&)
  {
    art::ServiceHandle<sim::LArG4Parameters> larParameters;
    fElectronsToGeV = 1./larParameters->GeVToElectrons();
  } // TrackStitching::beginRun

  void TrackStitching::analyze(const art::Event& event)
  {
    // Fetch basic event info
    fEvent  = event.id().event();
    fRun    = event.run();
    fSubRun = event.subRun();

    // Define handle to point to a vector of MCParticles
    auto particleHandle = event.getValidHandle<std::vector<simb::MCParticle>>(fSimulationProducerLabel);

    // Get all the simulated channels
    auto simChannelHandle = event.getValidHandle<std::vector<sim::SimChannel>>(fSimulationProducerLabel);

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
      const TLorentzVector& momentumEnd   = particle.Momentum(last);

      // Fill histogram with starting momentum
      fMomentumHist->Fill( momentumStart.P() );

      // Fill arrays with 4-values
      positionStart.GetXYZT( fStartXYZT );
      positionEnd.GetXYZT( fEndXYZT );
      momentumStart.GetXYZT( fStartPE );
      momentumEnd.GetXYZT( fEndPE );

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

      // Determine the number of dE/dx bins for the n-tuple.
      fSimNdEdxBins = int( trackLength / fBinSize ) + 1;

      // Initialize the vector of dE/dx bins to be empty.
      fSimdEdxBins.clear();

      // To look at the energy deposited by this particle's track,
      // we loop over the SimChannel objects in the event.
      for ( auto const& channel : (*simChannelHandle) ) {
        // Get the numeric ID associated with this channel. 
        auto const channelNumber = channel.Channel();
            
        // Only include collection plane
        if ( fGeometryService->SignalType( channelNumber ) != geo::kCollection ) continue;
            
        // Each channel has a map inside it that connects a time
        // slice to energy deposits in the detector.
        auto const& timeSlices = channel.TDCIDEMap();
        
        // For every time slice in this channel:
        for ( auto const& timeSlice : timeSlices ) {
          // Each entry in a map is a pair<first,second>.
          auto const& energyDeposits = timeSlice.second;
              
          // Loop over the energy deposits.
          for ( auto const& energyDeposit : energyDeposits ) {
            // Check if the track that deposited the energy matches the track of the particle.
            if ( energyDeposit.trackID != fSimTrackID ) continue;

            // Get the (x,y,z) of the energy deposit.
            TVector3 location( energyDeposit.x,
                               energyDeposit.y,
                               energyDeposit.z );
          
            // Distance from the start of the track.
            const double distance = ( location - positionStart.Vect() ).Mag();
                
            // Into which bin of the dE/dx array do we add the energy?
            const unsigned int bin = (unsigned int)( distance / fBinSize );
          
            // Is the dE/dx array big enough to include this bin?
            if ( fSimdEdxBins.size() < bin+1 ) {
              //  Increase the array size to accomodate
              //  the new bin, padding it with zeros.
              fSimdEdxBins.resize( bin+1 , 0. );
            }

            // Add the energy deposit to that bin.
            fSimdEdxBins[bin] += energyDeposit.numElectrons * fElectronsToGeV;

          } // For each energy deposit
        } // For each time slice
      } // For each SimChannel
        
      // Write to n-tuple
      fSimulationNtuple->Fill();
        
    } // loop over all particles in the event. 
    
    // Get the hits
    art::Handle< std::vector<recob::Hit> > hitHandle;
    if (!event.getByLabel(fHitProducerLabel, hitHandle)) return;

    // Fill map with track ID as the key, to hold the vectors of dE/dx information.
    std::map< int, std::vector<double> > dEdxMap;

    // For every Hit:
    for ( auto const& hit : (*hitHandle) ) {
      // The channel associated with this hit.
      auto hitChannelNumber = hit.Channel();

      // Only use collection plane
      if ( fGeometryService->SignalType( hitChannelNumber ) != geo::kCollection ) continue;

      LOG_DEBUG("TrackStitching")
        << "Hit in collection plane"
        << std::endl;

      typedef sim::SimChannel::StoredTDC_t TDC_t;

      TDC_t start_tdc    = fTimeService->TPCTick2TDC( hit.StartTick() );
      TDC_t end_tdc      = fTimeService->TPCTick2TDC( hit.EndTick()   );
      TDC_t hitStart_tdc = fTimeService->TPCTick2TDC( hit.PeakTime() - 3.*hit.SigmaPeakTime() );
      TDC_t hitEnd_tdc   = fTimeService->TPCTick2TDC( hit.PeakTime() + 3.*hit.SigmaPeakTime() );

      start_tdc = std::max(start_tdc, hitStart_tdc);
      end_tdc   = std::min(end_tdc,   hitEnd_tdc  );

      // Search the SimChannels for matching channel number, then look at the tracks inside the channel.
    
      for ( auto const& channel : (*simChannelHandle) ) {

        auto simChannelNumber = channel.Channel();
        if ( simChannelNumber != hitChannelNumber ) continue;

        LOG_DEBUG("TrackStitching")
          << "SimChannel number = " << simChannelNumber
          << std::endl;

        // The time slices in this channel.
        auto const& timeSlices = channel.TDCIDEMap();
        // We want to look over the range of time slices in this
        // channel that correspond to the range of hit times. 
        sim::TDCIDE startTime;
        sim::TDCIDE endTime;
        startTime.first = start_tdc;
        endTime.first   = end_tdc;

        // Find a pointer to the first channel with time >= start_tdc.
        auto const startPointer = std::lower_bound( timeSlices.begin(), timeSlices.end(), startTime, TDCIDETimeCompare);

        // From that time slice, find the last channel with time < end_tdc.
        auto const endPointer   = std::upper_bound( startPointer,       timeSlices.end(), endTime,   TDCIDETimeCompare);

        // Did we find anything? If not, skip. 
        if ( startPointer == timeSlices.end() || startPointer == endPointer ) continue;

        LOG_DEBUG("TrackStitching")
          << "Time slice start = " << (*startPointer).first
          << std::endl;

        // Loop over the channel times we found that match the hit times.
        for ( auto slicePointer = startPointer; slicePointer != endPointer; slicePointer++) {

          auto const timeSlice = *slicePointer;
          //auto time = timeSlice.first;                

          // Loop over the energy deposits.
          auto const& energyDeposits = timeSlice.second;
          for ( auto const& energyDeposit : energyDeposits ) {

            auto search = particleMap.find( energyDeposit.trackID );
    
            // Did we find this track ID in the particle map?
            if ( search == particleMap.end() ) continue;

            // "search" points to a pair in the map: <track ID, MCParticle*>
            int trackID = (*search).first;
            const simb::MCParticle& particle = *((*search).second);
        
            // Is this a primary particle, with a PDG code that matches the user input?
            if ( particle.Process() != "primary" || particle.PdgCode() != fSelectedPDG ) continue;

            // Determine the dE/dx of this particle.
            const TLorentzVector& positionStart = particle.Position(0);
            TVector3 location( energyDeposit.x, energyDeposit.y, energyDeposit.z );
            double distance = ( location - positionStart.Vect() ).Mag();
            unsigned int bin = int( distance / fBinSize );
            double energy = energyDeposit.numElectrons * fElectronsToGeV;

            auto& track_dEdX = dEdxMap[trackID];

            if ( track_dEdX.size() < bin+1 ) {
              // Increase the vector size, padding it with zeroes.
              track_dEdX.resize( bin+1, 0 ); 
            }
            
            // Add the energy to the dE/dx bins for this track.
            track_dEdX[bin] += energy;
            
          } // loop over energy deposits
        } // loop over time slices
      } // for each SimChannel
    } // for each Hit

    // We have a map of dE/dx vectors. Write each one of them to the reconstruction n-tuple. 
    for ( const auto& dEdxEntry : dEdxMap ) {

      // Here, the map entries are <first,second>=<track ID, dE/dx vector>
      fRecoTrackID = dEdxEntry.first;

      fRecoPDG = particleMap[fRecoTrackID]->PdgCode();

      // Get the number of bins for this track.
      const std::vector<double>& dEdx = dEdxEntry.second;
      fRecoNdEdxBins = dEdx.size();

      // Copy this track's dE/dx information.
      fRecodEdxBins = dEdx;

      fReconstructionNtuple->Fill();
    }

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

  // Define a comparison function to use in std::upper_bound and std::lower_bound searches above.
  bool TDCIDETimeCompare( const sim::TDCIDE& lhs, const sim::TDCIDE& rhs )
  {
    return lhs.first < rhs.first;
  }

} // local namespace


