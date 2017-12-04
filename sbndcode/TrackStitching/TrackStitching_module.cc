////////////////////////////////////////////////////////////////////////
// Class:       TrackStitching
// Module Type: analyzer
// File:        TrackStitching_module.cc
//
// Tom Brooks (tbrooks@fnal.gov)
////////////////////////////////////////////////////////////////////////

// sbndcode includes
#include "sbndcode/RecoUtils/RecoUtils.h"

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
#include "larsim/MCCheater/BackTracker.h"

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

  // Utility function to determine if a true particle crosses the cathode
  bool CrossesCathode(simb::MCParticle const& particle);

  // Temp utility function to determine if a true particle hits a CRT
  bool HitsCRT(simb::MCParticle const& particle);

  // Temp utility function to determine if true particle triggers the PDS
  bool HitsPDS(simb::MCParticle const& particle, double lengthLimit);

  // Utility function to determine if tracks should be stitched
  int MatchPoints(recob::Track const& track1, recob::Track const& track2, double distLimit, double &cosResult, double &difResult);

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
      
      fhicl::Atom<art::InputTag> TrackLabel {
        Name("TrackLabel"),
        Comment("tag of the input data product with reconstructed tracks")
      };

      fhicl::Atom<double> StitchAngle {
        Name("StitchAngle"),
        Comment("minimum angle to stitch tracks between TPCs (unit = degrees)")
      };

      fhicl::Atom<double> DeltaX {
        Name("DeltaX"),
        Comment("maximum difference in absolute x positions (unit = cm)")
      };
      
    }; // Config
 
    using Parameters = art::EDAnalyzer::Table<Config>;
 
    // Constructor: configures module
    explicit TrackStitching(Parameters const& config);
 
    // Called once, at start of the job
    virtual void beginJob() override;
 
    // Called once per event
    virtual void analyze(const art::Event& event) override;

    // Called once, at end of the job
    virtual void endJob() override;

    // Function to match pairs of tracks in two TPCs without any timing info
    std::vector< std::pair< const recob::Track*, const recob::Track* >> StitchWithoutT0(std::vector<const recob::Track*> tracks1, 
                                                                                        std::vector<const recob::Track*> tracks2);

    // Function to match pairs of tracks in two TPCs using timing info
    std::vector< std::pair< const recob::Track*, const recob::Track* >> StitchWithT0(std::vector<const recob::Track*> tracks1,
                                                                                     std::vector<const recob::Track*> tracks2,
                                                                                     std::vector<double> times);
    // Function to evaluate performance of track stitching across TPCs
    std::vector<int> Evaluate(std::vector< std::pair< const recob::Track*, const recob::Track* >> trueMatches,
                              std::vector< std::pair< const recob::Track*, const recob::Track* >> foundMatches);

  private:

    // fcl file parameters
    art::InputTag         fSimulationProducerLabel; ///< name of detsim producer
    art::InputTag         fTrackProducerLabel;      ///< name of the track producer
    double                fStitchAngle;             ///< minimum stitching angle between tracks
    double                fDeltaX;                  ///< maxmum difference in absolute x positions
    

    // Pointers to histograms
    TH1D* fCorrectAngleHist;    ///< Angle between correctly stitched tracks
    TH1D* fIncorrectAngleHist;  ///< Angle between incorrectly stitched tracks
    TH1D* fMissedAngleHist;     ///< Angle between tracks that should have been stitched but weren't
    TH1D* fCorrectDeltaXHist;   ///< Difference between min x positions for correctly stitched tracks
    TH1D* fIncorrectDeltaXHist; ///< Difference between min x positions for incorrectly stitched tracks
    TH1D* fMissedDeltaXHist;    ///< Difference between min x positions for missed tracks
    TH1D* fMissedMinLenHist;    ///< Shortest track length for missed tracks
    TH1D* fStartT;

    // The n-tuples
    TTree* fStitchingNtuple;     ///< tuple with info about stitching

    /// @name the variables that will go into both n-tuples.
    /// @{
    int fEvent;  ///< event number
    int fRun;    ///< run number
    int fSubRun; ///< sub-run number
    /// @}

    // Other variables shared between different methods.
    geo::GeometryCore const* fGeometryService;                     ///< pointer to Geometry provider
    detinfo::DetectorProperties const* fDetectorProperties; ///< pointer to detector properties provider

    // Counters for text output
    int nCathodeCrossers = 0;
    int nStitched        = 0;
    int nCorrect         = 0;
    int nIncorrect       = 0;
    int nMissed          = 0;
  }; // class TrackStitching

  // Constructor
  TrackStitching::TrackStitching(Parameters const& config)
    : EDAnalyzer(config)
    , fSimulationProducerLabel(config().SimulationLabel())
    , fTrackProducerLabel     (config().TrackLabel())
    , fStitchAngle            (config().StitchAngle())
    , fDeltaX                 (config().DeltaX())
  {
    // Get a pointer to the geometry service provider
    fGeometryService = lar::providerFrom<geo::Geometry>();
    fDetectorProperties = lar::providerFrom<detinfo::DetectorPropertiesService>();
    
  }

  void TrackStitching::beginJob()
  {
    // Access tfileservice to handle creating and writing histograms
    art::ServiceHandle<art::TFileService> tfs;

    // Define histograms
    fCorrectAngleHist    = tfs->make<TH1D>("correctang",   ";Angle between tracks (rad);", 180, 0, 180);
    fIncorrectAngleHist  = tfs->make<TH1D>("incorrectang", ";Angle between tracks (rad);", 180, 0, 180);
    fMissedAngleHist     = tfs->make<TH1D>("missedang",    ";Angle between tracks (rad);", 180, 0, 180);
    fCorrectDeltaXHist   = tfs->make<TH1D>("correctdx",    ";#Delta x (cm);",              100, 0, 3);
    fIncorrectDeltaXHist = tfs->make<TH1D>("incorrectdx",  ";#Delta x (cm);",              100, 0, 3);
    fMissedDeltaXHist    = tfs->make<TH1D>("misseddx",     ";#Delta x (cm);",              100, 0, 3);
    fMissedMinLenHist    = tfs->make<TH1D>("missedlen",    ";Min track length (cm);",      100, 0, 200);
    fStartT = tfs->make<TH1D>("start","",100,-60000,60000);

    // Define n-tuples
    fStitchingNtuple = tfs->make<TTree>("TrackStitching", "TrackStitching");

    // Define branches of simulation n-tuple
    fStitchingNtuple->Branch("Event",           &fEvent,            "Event/I");
    fStitchingNtuple->Branch("SubRun",          &fSubRun,           "SubRun/I");
    fStitchingNtuple->Branch("Run",             &fRun,              "Run/I");

    std::cout<<"Drift velocity      = "<<fDetectorProperties->DriftVelocity()<<" cm/us"<<std::endl
             <<"Max drift distance  = "<<2.0*fGeometryService->DetHalfWidth()<<" cm"<<std::endl
             <<"Readout window size = "<<fDetectorProperties->ReadOutWindowSize()<<" ticks"<<std::endl
             <<"Max drift time      = "<<4.0*fGeometryService->DetHalfWidth()/fDetectorProperties->DriftVelocity()<<" ticks"<<std::endl;

  } // TrackStitchingbeginJob

  void TrackStitching::analyze(const art::Event& event)
  {

    std::vector<double> vCrtTimes; // Vector of true start times from cosmic rat trackers (units = ticks)
    std::vector<double> vPdsTimes; // Vector of true start times from photon detection system (units = ticks)

    // Fetch basic event info
    fEvent  = event.id().event();
    fRun    = event.run();
    fSubRun = event.subRun();

    //-----------------------------------------------------------------------------------------------------------------
    //--------------------------------------- RETRIEVE TRUTH INFORMATION ----------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------

    // Get true particles
    // Define handle to point to a vector of MCParticles
    auto particleHandle = event.getValidHandle<std::vector<simb::MCParticle>>(fSimulationProducerLabel);
    std::map<int, simb::MCParticle> particles;
    // Loop over the true particles
    for (auto const& particle : (*particleHandle) ){
      int partId = particle.TrackId();
      particles[partId] = particle;

      double startTimeTicks = (particle.T()*10e-9)/(0.5*10e-6);
      double readoutWindow  = (double)fDetectorProperties->ReadOutWindowSize();
      double driftTimeTicks = 4.0*fGeometryService->DetHalfWidth()/fDetectorProperties->DriftVelocity();

      // If charged particle crosses CRTs and is within reconstructable window add the start time to a vector
      if (HitsCRT(particle) && startTimeTicks > -driftTimeTicks && startTimeTicks < readoutWindow){
        vCrtTimes.push_back(startTimeTicks);
      }
      // If charged particle has more than 5 cm inside AV and is within reconstructable window add the start time to a vector
      if (HitsPDS(particle, 5) && startTimeTicks > -driftTimeTicks && startTimeTicks < readoutWindow){
        vPdsTimes.push_back(startTimeTicks);
      }
    }

    //std::cout<<"Number of CRT times = "<<vCrtTimes.size()<<", Number of PDS times = "<<vPdsTimes.size()<<std::endl;
     
    //------------------------------------------------------------------------------------------------------------------
    //------------------------------------------- SEPARATE TRACKS BY TPC -----------------------------------------------
    //------------------------------------------------------------------------------------------------------------------

    // Get tracks from the event
    auto trackHandle = event.getValidHandle<std::vector<recob::Track>>(fTrackProducerLabel);
    // Get track to hit associations
    art::FindManyP<recob::Hit> findManyHits(trackHandle, event, fTrackProducerLabel);
    if ( ! findManyHits.isValid() ) {
      mf::LogError("TrackStitching")  
        << "findManyHits recob::Hit for recob::Track failed;"
        << " track label='" << fTrackProducerLabel << "'";
    }

    std::vector<const recob::Track*> tracksInTpc1;
    std::vector<const recob::Track*> tracksInTpc2;
    // Loop over tracks
    for (auto const& track : (*trackHandle) ){
      // Put tracks in a map according to which tpc the hits are in
      std::vector< art::Ptr<recob::Hit> > hits = findManyHits.at(track.ID());
      bool inTpc1 = false;
      bool inTpc2 = false;
      for (auto const& hit : hits){
        if (hit->WireID().TPC == 0) inTpc1 = true;
        if (hit->WireID().TPC == 1) inTpc2 = true;
      }
      // Hits all in first tpc
      if (inTpc1 && !inTpc2){
        tracksInTpc1.push_back(&track);
      }
      // Hits all in second tpc
      else if (inTpc2 && !inTpc1){
        tracksInTpc2.push_back(&track);
      }
      // If reconstructed track crossed the cathode (has hits in both tpcs) don't put in map
    }
    
    //-------------------------------------------------------------------------------------------------------------------
    //----------------------------------------- DO THE TRACK STITCHING --------------------------------------------------
    //-------------------------------------------------------------------------------------------------------------------
    // There are 3 distinct ways that tracks can be messed up when they cross the cathode plane depending on their true start times
    // Case 1 (-drift time < t < 0): Tracks shifted towards anodes
    // Case 2 (0 < t < dt): Tracks shifted into other TPC, crossing points reconstructed (dt = readout window - drift time)
    // Case 3 (dt < t < readout window): Tracks shifted into other TPC, crossing points not reconstructed

    // Find all the pairs of tracks which should be stitched
    std::vector< std::pair<const recob::Track*, const recob::Track*>> trueMatches;
    // Loop over tracks in first TPC
    for (auto const& track1 : tracksInTpc1){
      std::vector< art::Ptr<recob::Hit> > tpc1Hits = findManyHits.at(track1->ID());
      int tpc1TrueId = RecoUtils::TrueParticleIDFromTotalTrueEnergy(tpc1Hits);

      // Loop over tracks in second TPC
      for (auto const& track2 : tracksInTpc2){
        std::vector< art::Ptr<recob::Hit> > tpc2Hits = findManyHits.at(track2->ID());
        int tpc2TrueId = RecoUtils::TrueParticleIDFromTotalTrueEnergy(tpc2Hits);
        
        if (tpc1TrueId == tpc2TrueId && CrossesCathode(particles[tpc1TrueId])){
          nCathodeCrossers++;
          std::cout<<"Track 1 ID = "<<track1->ID()<<" Track 2 ID = "<<track2->ID()<<std::endl;
          //std::cout<<(particles[tpc1TrueId].T()*10e-9)/(0.5*10e-6)<<" hits CRT? "<<HitsCRT(particles[tpc1TrueId])<<", length = "<<particles[tpc1TrueId].Trajectory().TotalLength()<<std::endl;
          trueMatches.push_back(std::make_pair(track1, track2));
        }

      } // End of loop over TPC 2 tracks
    } // End of loop over TPC 1 tracks

    auto matches = StitchWithoutT0(tracksInTpc1, tracksInTpc2);
    auto CrtMatches = StitchWithT0(tracksInTpc1, tracksInTpc2, vCrtTimes);
    std::vector<int> noT0Results = Evaluate(trueMatches, CrtMatches);
    nStitched += CrtMatches.size();
    nCorrect += noT0Results[0];
    nIncorrect += noT0Results[1];
    nMissed += noT0Results[2];

    fStitchingNtuple->Fill();

  } // TrackStitching::analyze()

  void TrackStitching::endJob(){
    // Output some variables
    std::cout<<"Total number of cathode crossers           = "<<nCathodeCrossers<<std::endl
             <<"Total number of stitched reco tracks       = "<<nStitched<<std::endl
             <<"Number of correctly stitched reco tracks   = "<<nCorrect<<std::endl
             <<"Number of incorrectly stitched reco tracks = "<<nIncorrect<<std::endl
             <<"Number of missed reco tracks               = "<<nMissed<<std::endl;
  }

  // Function to stitch tracks across TPCs without timing info
  std::vector<std::pair<const recob::Track*, const recob::Track*>> TrackStitching::StitchWithoutT0(std::vector<const recob::Track*> tracks1, 
                                                                                                   std::vector<const recob::Track*> tracks2){

    std::vector< std::pair< const recob::Track*, const recob::Track* >> matches;

    //double cosLimit = cos(TMath::Pi() * fStitchAngle / 180.0);

    //double readoutWindow  = (double)fDetectorProperties->ReadOutWindowSize();
    //double driftTimeTicks = 4.0*fGeometryService->DetHalfWidth()/fDetectorProperties->DriftVelocity();
    //double dx = ((readoutWindow-driftTimeTicks)*0.5)*fDetectorProperties->DriftVelocity();

    // Loop over tracks in first TPC
    for (auto const& track1 : tracks1){

      //double lowestCos = -1.;
      //const recob::Track* matchCandidate = track1; // ugly, needed to avoid warnings

      // Loop over tracks in second TPC
      for (auto const& track2 : tracks2){
        
        // Check if start/end matched
        double currentCos;
        double currentDif;
        int matchResult = MatchPoints((*track1), (*track2), fDeltaX, currentCos, currentDif);

        // If they do check if matched points = dx
        /*if (std::abs(std::abs(track1->Vertex().X())-dx) < distLimit || std::abs(std::abs(track1->End().X())-dx) < distLimit){
          // If they do record candidate track and angle
          if (currentCos < lowestCos){ lowestCos = currentCos; matchCandidate = track2; }
        } */    

        // Else check if angle < threshold
        if (matchResult != 0 && TMath::ACos(currentCos)*180./TMath::Pi() < fStitchAngle){
          matches.push_back(std::make_pair(track1, track2));
          // If it is record candidate track and threshold (could add length check)
          //if (currentCos > lowestCos){ lowestCos = currentCos; matchCandidate = track2; }
        }

      } // End of loop over TPC 2 tracks
      // Match with best candidate (lowest cos theta) add to vector
      //if (lowestCos != -1.) matches.push_back(std::make_pair(track1, matchCandidate));
    } // End of loop over TPC 1 tracks

    return matches;

  } // TrackStitching::StitchWithoutT0()

  // Function to match pairs of tracks in two TPCs using timing info
  std::vector<std::pair<const recob::Track*, const recob::Track*>> TrackStitching::StitchWithT0(std::vector<const recob::Track*> tracks1,
                                                                                                std::vector<const recob::Track*> tracks2,
                                                                                                std::vector<double> times){
    std::vector< std::pair< const recob::Track*, const recob::Track* >> matches;

    double cosLimit = cos(TMath::Pi() * fStitchAngle / 180.0);

    double readoutWindow  = (double)fDetectorProperties->ReadOutWindowSize();
    double driftVelocity = fDetectorProperties->DriftVelocity();
    double driftTimeTicks = 4.0*fGeometryService->DetHalfWidth()/driftVelocity;
    double dt = readoutWindow - driftTimeTicks;
    double dx = (dt*0.5)*driftVelocity-2.6;

    std::cout<<"Times:"<<std::endl;
    std::cout<<"dx = "<<dx<<std::endl;
    for (double const& time : times){
      double distance = (time*0.5)*driftVelocity;
      std::cout<<"time = "<<time<<", distance = "<<distance-2.6<<std::endl;
    }
    std::cout<<"Track in TPC 1:"<<std::endl;
    for (auto const& track1 : tracks1){
      std::cout<<"ID = "<<track1->ID()<<", vtx="<<track1->Vertex().X()<<", end = "<<track1->End().X()<<std::endl;
    }
    std::cout<<"Tracks in TPC 2:"<<std::endl;
    for (auto const& track2 : tracks2){
      std::cout<<"ID = "<<track2->ID()<<", vtx="<<track2->Vertex().X()<<", end = "<<track2->End().X()<<std::endl; 
    }

    bool isDxUsed = false;

    // Loop over vector of times
    for (double const& time : times){
      std::vector<std::pair<const recob::Track*, TVector3>> track1Candidates;
      std::vector<std::pair<const recob::Track*, TVector3>> track2Candidates;
      // If the time < readout - drift, set time = time (ticks)
      double distance = (time*0.5)*driftVelocity - 2.6; // BS correction factor (geometry problem I think)
      // If the time > readout - drift set time = dt, however this should only be done once to avoid double counting
      if (time > dt && !isDxUsed){ distance = dx; isDxUsed = true;}
      else if (time > dt && isDxUsed) continue;

      // Loop over tracks in first TPC
      for (auto const& track1 : tracks1){
        // If the start or end of the track matches the time, add to a vector
        if (std::abs(track1->Vertex().X()-distance) < fDeltaX) track1Candidates.push_back(std::make_pair(track1, track1->VertexDirection()));
        if (std::abs(track1->End().X()-distance) < fDeltaX)    track1Candidates.push_back(std::make_pair(track1, track1->EndDirection()));
      }

      // Loop over tracks in second TPC
      for (auto const& track2 : tracks2){
        // If the start or end of the track matches the time, add to a vector
        if (std::abs(track2->Vertex().X()+distance) < fDeltaX) track2Candidates.push_back(std::make_pair(track2, track2->VertexDirection()));
        if (std::abs(track2->End().X()+distance) < fDeltaX)    track2Candidates.push_back(std::make_pair(track2, track2->EndDirection()));
      }

      if (track1Candidates.size() == 0 || track2Candidates.size() == 0) continue;
      // If there is one track in each vector match tracks
      if (track1Candidates.size() == 1 && track2Candidates.size() == 1){ 
        double angle = track1Candidates[0].second.Dot(track2Candidates[0].second);
        if (angle > cosLimit) matches.push_back(std::make_pair(track1Candidates[0].first, track2Candidates[0].first));
        //std::cout<<track1Candidates[0].first->Vertex().X()<<" "<<track2Candidates[0].first->Vertex().X()<<" "<<track1Candidates[0].first->End().X()<<" "<<track2Candidates[0].first->End().X()<<std::endl;}
      }
      // If there is more than one, match angles, then match tracks
      else {
        for (size_t i = 0; i < track1Candidates.size(); i++){
          double bestAngle = -1.;
          double best_j = 0;
          for (size_t j = 0; j < track2Candidates.size(); j++){
            double angle = track1Candidates[i].second.Dot(track2Candidates[j].second);
            if (angle > bestAngle){ bestAngle = angle; best_j = j; }
          }
          if (bestAngle > cosLimit) matches.push_back(std::make_pair(track1Candidates[i].first, track2Candidates[best_j].first));
          //std::cout<<track1Candidates[i].first->Vertex().X()<<" "<<track2Candidates[best_j].first->Vertex().X()<<" "<<track1Candidates[i].first->End().X()<<" "<<track2Candidates[best_j].first->End().X()<<std::endl;}
          //std::cout<<track1Candidates[i].first->ID()<<" "<<track2Candidates[best_j].first->ID()<<" "<<track1Candidates[i].second.X()<<std::endl;}
        }
      }
        
    }

    // Remove any duplicate matches, can occur when both cathode and anode are crossed
    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());

    return matches;
 
  } // TrackStitching::StitchWithT0()

  // Function to evaluate the performance of track matching algorithms
  // If I need to add to histograms I might be able to 
  std::vector<int> TrackStitching::Evaluate(std::vector< std::pair< const recob::Track*, const recob::Track* >> trueMatches,
                            std::vector< std::pair< const recob::Track*, const recob::Track* >> foundMatches){

    // Calculate the number of correct matches by looping over both sets and recording the number of matches
    int nCorrect = 0; 
    int nIncorrect = 0;
    int nMissed = 0;
    for (auto const& foundMatch : foundMatches){
      int foundTrack1Id = foundMatch.first->ID();
      int foundTrack2Id = foundMatch.second->ID();
      bool isCorrect = false;
      for (auto const& trueMatch : trueMatches){
        int trueTrack1Id = trueMatch.first->ID();
        int trueTrack2Id = trueMatch.second->ID();
        if (foundTrack1Id == trueTrack1Id && foundTrack2Id == trueTrack2Id){ 
          nCorrect++;
          isCorrect = true;
          std::cout<<"correct: "<<foundTrack1Id<<" "<<foundTrack2Id<<std::endl;
          //Calculate any variables and fill histograms
          double currentCos;
          double currentDif;
          MatchPoints((*trueMatch.first), (*trueMatch.second), fDeltaX, currentCos, currentDif);
          fCorrectAngleHist->Fill(TMath::ACos(currentCos)*180./TMath::Pi());
          fCorrectDeltaXHist->Fill(currentDif);
        }
      }
      if (!isCorrect){
        //Calculate any variables and fill histograms for incorrect tracks
        nIncorrect++;
        std::cout<<"incorrect: "<<foundTrack1Id<<" "<<foundTrack2Id<<std::endl;
        double currentCos;
        double currentDif;
        MatchPoints((*foundMatch.first), (*foundMatch.second), fDeltaX, currentCos, currentDif);
        fIncorrectAngleHist->Fill(TMath::ACos(currentCos)*180./TMath::Pi());
        fIncorrectDeltaXHist->Fill(currentDif);
      }
    }

    for (auto const& trueMatch : trueMatches){
      int trueTrack1Id = trueMatch.first->ID();
      int trueTrack2Id = trueMatch.second->ID();
      bool isCorrect = false;
      for (auto const& foundMatch : foundMatches){
        int foundTrack1Id = foundMatch.first->ID();
        int foundTrack2Id = foundMatch.second->ID();
        if (foundTrack1Id == trueTrack1Id && foundTrack2Id == trueTrack2Id){ isCorrect = true; }
      }
      if (!isCorrect){
        //Calculate any variables and fill histograms for missed tracks
        nMissed++;
        std::cout<<"miss: "<<trueTrack1Id<<" "<<trueTrack2Id<<std::endl;
        double currentCos;
        double currentDif;
        MatchPoints((*trueMatch.first), (*trueMatch.second), fDeltaX, currentCos, currentDif);
        fMissedAngleHist->Fill(TMath::ACos(currentCos)*180./TMath::Pi());
        fMissedDeltaXHist->Fill(currentDif);
        fMissedMinLenHist->Fill(trueMatch.first->Length()); fMissedMinLenHist->Fill(trueMatch.second->Length()); 
      }
    }
    // Calculate the number of incorrect matches by recording the number of pairs not in true vector
    //int nIncorrect = foundMatches.size() - nCorrect;
    // Calculate the number of missed matches by recording the number of pairs not in the found vector
    //int nMissed = trueMatches.size() - nCorrect;

    std::vector<int> results{nCorrect, nIncorrect, nMissed};
    return results;

  } // TrackStitching::Evaluate()

  DEFINE_ART_MODULE(TrackStitching)
} // namespace sbnd

// Back to our local namespace.
namespace {

  // Define a local function to determine if true track crosses cathode
  bool CrossesCathode(simb::MCParticle const& particle){
    size_t numTrajPoints = particle.NumberTrajectoryPoints();
    auto mcTrajectory = particle.Trajectory();
    bool inTpc1 = false;
    bool inTpc2 = false;
    // Loop over particle trajectory
    for (size_t traj_i = 0; traj_i < numTrajPoints; traj_i++){
      if (RecoUtils::IsInsideTPC(mcTrajectory.Position(traj_i).Vect(), 0.0) && mcTrajectory.X(traj_i) < 0) inTpc1 = true;
      if (RecoUtils::IsInsideTPC(mcTrajectory.Position(traj_i).Vect(), 0.0) && mcTrajectory.X(traj_i) > 0) inTpc2 = true;
      // If particle has two traj points inside tpc either side of cathode assume it's crossed
      if (inTpc1 && inTpc2) return true;
    }
    return false;
  } // CrossesCathode()

  // Function to check if particle is charged and crosses the TPC boundary
  bool HitsCRT(const simb::MCParticle& part){

    // Check particle is charged first
    int pdg = std::abs(part.PdgCode());
    if (!(pdg == 13 || pdg == 11 || pdg == 2212 || pdg == 321 || pdg == 211)) return false;

    // Check if particle has points both inside and outside the active volume
    bool outsideAV = false;
    bool insideAV  = false;

    // Get geometry.
    art::ServiceHandle<geo::Geometry> geom;
    // Get active volume boundary. SBND specific
    double xmin = -2.0 * geom->DetHalfWidth();
    double xmax = 2.0 * geom->DetHalfWidth();
    double ymin = -geom->DetHalfHeight();
    double ymax = geom->DetHalfHeight();
    double zmin = 0.;
    double zmax = geom->DetLength();

    // Loop over trajectory points
    int nTrajPoints = part.NumberTrajectoryPoints();
    for (int traj_i = 0; traj_i < nTrajPoints; traj_i++){
      TVector3 trajPoint(part.Vx(traj_i), part.Vy(traj_i), part.Vz(traj_i));
      // Check if point is within reconstructable volume
      if (trajPoint[0] >= xmin && trajPoint[0] <= xmax && trajPoint[1] >= ymin && trajPoint[1] <= ymax && trajPoint[2] >= zmin && trajPoint[2] <= zmax){
        insideAV = true;
      }
      else outsideAV = true;
    }

    if(insideAV && outsideAV) return true;
    return false;

  } // HitsCRT()

  // Function to check if particle is charged and has a certain length inside TPC
  bool HitsPDS(const simb::MCParticle& part, double lengthLimit){

    // Check particle is charged first
    int pdg = std::abs(part.PdgCode());
    if (!(pdg == 13 || pdg == 11 || pdg == 2212 || pdg == 321 || pdg == 211)) return false;

    // Calculate the length of the track inside the TPC
    double length = 0.;
    bool first  = true;
    TVector3 displacement;

    // Get geometry.
    art::ServiceHandle<geo::Geometry> geom;
    // Get active volume boundary. SBND specific
    double xmin = -2.0 * geom->DetHalfWidth();
    double xmax = 2.0 * geom->DetHalfWidth();
    double ymin = -geom->DetHalfHeight();
    double ymax = geom->DetHalfHeight();
    double zmin = 0.;
    double zmax = geom->DetLength();

    // Loop over trajectory points
    int nTrajPoints = part.NumberTrajectoryPoints();
    for (int traj_i = 0; traj_i < nTrajPoints; traj_i++){
      TVector3 trajPoint(part.Vx(traj_i), part.Vy(traj_i), part.Vz(traj_i));
      // Check if point is within reconstructable volume
      if (trajPoint[0] >= xmin && trajPoint[0] <= xmax && trajPoint[1] >= ymin && trajPoint[1] <= ymax && trajPoint[2] >= zmin && trajPoint[2] <= zmax){
        if(!first) {
          displacement -= trajPoint;
          length += displacement.Mag();
        }
        first = false;
        displacement = trajPoint;
      }
    }

    if (length > lengthLimit) return true;
    return false;

  } // HitsPDS()

  // Function to determine if track starts/ends match
  int MatchPoints(recob::Track const& track1, recob::Track const& track2, double distLimit, double &cosResult, double &diffResult){

    // RETURN CODES:
    // 0 = not matched;
    // 11 = start matched with start
    // 12 = start matched with end
    // 21 = end matched with start
    // 22 = end matched with end

    double cos = 1.;
    double diff = -1.;
    int code = 0;

    // Compare start and end x positions
    double difference = std::abs(std::abs(track1.Vertex().X())-std::abs(track2.Vertex().X()));
    if (difference < distLimit && track1.Vertex().X()*track2.Vertex().X() < 0.0){
      distLimit = difference;
      cos = track1.VertexDirection().Dot(track2.VertexDirection());
      diff = difference;
      code = 11;
    }
    difference = std::abs(std::abs(track1.Vertex().X())-std::abs(track2.End().X()));
    if (difference < distLimit && track1.Vertex().X()*track2.End().X() < 0.0){
      distLimit = difference;
      cos = track1.VertexDirection().Dot(track2.EndDirection());
      diff = difference;
      code = 12;
    }
    difference = std::abs(std::abs(track1.End().X())-std::abs(track2.Vertex().X()));
    if (difference < distLimit && track1.End().X()*track2.Vertex().X() < 0.0){
      distLimit = difference;
      cos = track1.EndDirection().Dot(track2.VertexDirection());
      diff = difference;
      code = 21;
    }
    difference = std::abs(std::abs(track1.End().X())-std::abs(track2.End().X()));
    if (difference < distLimit && track1.End().X()*track2.End().X() < 0.0){
      cos = track1.EndDirection().Dot(track2.EndDirection());
      diff = difference;
      code = 22;
    }

    cosResult = cos;
    diffResult = diff;
    return code;

  } // MatchPoints()
 
} // local namespace


