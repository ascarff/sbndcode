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
#include "Math/Minimizer.h"
#include "Math/Functor.h"
#include "Math/Factory.h"

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

  // Function to calculate the distance between CPA crossing points for two lines
  double distance(const double *input);

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
        Comment("maximum angle to stitch tracks between TPCs (unit = degrees)")
      };

      fhicl::Atom<double> DeltaX {
        Name("DeltaX"),
        Comment("maximum difference between start/end point and the cutoff (unit = cm)")
      };

      fhicl::Atom<double> DeltaT {
        Name("DeltaT"),
        Comment("maximum difference calculated T0 and CRT T0 (unit = ticks)")
      };

      fhicl::Atom<bool> Verbose {
        Name("Verbose"),
        Comment("Print information about what's going on")
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

    // Function to match pairs of incomplete tracks
    std::vector< std::pair< const recob::Track*, const recob::Track* >> StitchIncomplete(std::vector<const recob::Track*> tracks1,
                                                                                         std::vector<const recob::Track*> tracks2,
                                                                                         std::vector<std::pair<std::string, double>>& matchTimes,
                                                                                         std::vector<double> crtTimes);

    // Function to evaluate performance of track stitching across TPCs
    std::vector<int> Evaluate(std::vector< std::pair< const recob::Track*, const recob::Track* >> trueMatches,
                              std::vector< std::pair< const recob::Track*, const recob::Track* >> foundMatches);

    // Utility function to match start/end points of two tracks
    void MatchPoints(recob::Track const& track1, recob::Track const& track2, double &cosResult, double &diffResult1, double &diffResult2);

  private:

    // fcl file parameters
    art::InputTag         fSimulationProducerLabel; ///< name of detsim producer
    art::InputTag         fTrackProducerLabel;      ///< name of the track producer
    double                fStitchAngle;             ///< maximum stitching angle between tracks
    double                fDeltaX;                  ///< maximum difference in start/end x positions and cutogg
    double                fDeltaT;                  ///< maximum difference between calculated T0 and CRT T0
    bool                  fVerbose;                 ///< print information about what's going on
    

    // Pointers to histograms
    TH1D* fCorrectAngleHist;    ///< Angle between correctly stitched tracks
    TH1D* fIncorrectAngleHist;  ///< Angle between incorrectly stitched tracks
    TH1D* fMissedAngleHist;     ///< Angle between tracks that should have been stitched but weren't
    TH1D* fCorrectDeltaXHist;   ///< Difference between min x positions for correctly stitched tracks
    TH1D* fIncorrectDeltaXHist; ///< Difference between min x positions for incorrectly stitched tracks
    TH1D* fMissedDeltaXHist;    ///< Difference between min x positions for missed tracks
    TH1D* fDifferenceHist;      ///< Difference between calculated T0 and true T0

    // Other variables shared between different methods.
    geo::GeometryCore const* fGeometryService;              ///< pointer to Geometry provider
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
    , fDeltaT                 (config().DeltaT())
    , fVerbose                (config().Verbose())
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
    fCorrectAngleHist    = tfs->make<TH1D>("correctang",   ";Angle between tracks (deg);", 180, 0, 180);
    fIncorrectAngleHist  = tfs->make<TH1D>("incorrectang", ";Angle between tracks (deg);", 180, 0, 180);
    fMissedAngleHist     = tfs->make<TH1D>("missedang",    ";Angle between tracks (deg);", 180, 0, 180);
    fCorrectDeltaXHist   = tfs->make<TH1D>("correctdx",    ";#Delta x (cm);",              100, 0, 3);
    fIncorrectDeltaXHist = tfs->make<TH1D>("incorrectdx",  ";#Delta x (cm);",              100, 0, 3);
    fMissedDeltaXHist    = tfs->make<TH1D>("misseddx",     ";#Delta x (cm);",              100, 0, 3);
    fDifferenceHist      = tfs->make<TH1D>("difference",   ";Difference (ticks);",         100, -100, 100);

    // Initial output
    std::cout<<"----------------- Track Stitching Module -------------------"<<std::endl
             <<"Useful detector numbers:"<<std::endl
             <<"Drift velocity      = "<<fDetectorProperties->DriftVelocity()<<" cm/us"<<std::endl
             <<"Max drift distance  = "<<2.0*fGeometryService->DetHalfWidth()<<" cm"<<std::endl
             <<"Readout window size = "<<fDetectorProperties->ReadOutWindowSize()<<" ticks"<<std::endl
             <<"Max drift time      = "<<4.0*fGeometryService->DetHalfWidth()/fDetectorProperties->DriftVelocity()<<" ticks"<<std::endl
             <<"------------------------------------------------------------"<<std::endl;

  } // TrackStitchingbeginJob

  void TrackStitching::analyze(const art::Event& event)
  {

    // Vector of true start times from cosmic ray trackers (units = ticks)
    std::vector<double> vCrtTimes; 

    // Fetch basic event info
    double fEvent  = event.id().event();
    double fRun    = event.run();
    double fSubRun = event.subRun();

    if(fVerbose){
      std::cout<<"============================================"<<std::endl
               <<"Run = "<<fRun<<", SubRun = "<<fSubRun<<", Event = "<<fEvent<<std::endl
               <<"============================================"<<std::endl;
    }

    // Detector properties
    double readoutWindow  = (double)fDetectorProperties->ReadOutWindowSize();
    double driftTimeTicks = 4.0*fGeometryService->DetHalfWidth()/fDetectorProperties->DriftVelocity();
    double dt = readoutWindow - driftTimeTicks;

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

      // If charged particle crosses CRTs and is within reconstructable window add the start time to a vector
      if (HitsCRT(particle) && startTimeTicks > dt && startTimeTicks < readoutWindow){
        vCrtTimes.push_back(startTimeTicks);
      }
    }

    if(fVerbose){
      std::cout<<"Number of CRT times = "<<vCrtTimes.size()<<std::endl;
    }
     
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
      // Uncomment if you want all tracks
      // Hits all in first tpc
      if (inTpc1 && !inTpc2){
        tracksInTpc1.push_back(&track);
      }
      // Hits all in second tpc
      else if (inTpc2 && !inTpc1){
        tracksInTpc2.push_back(&track);
      }
      // If reconstructed track crossed the cathode (has hits in both tpcs) don't put in vector
    }
    
    //-------------------------------------------------------------------------------------------------------------------
    //----------------------------------------- DO THE TRACK STITCHING --------------------------------------------------
    //-------------------------------------------------------------------------------------------------------------------
    // There are 3 distinct ways that tracks can be messed up when they cross the cathode plane depending on their true start times
    // Case 1 (-drift time < t < 0): Tracks shifted towards anodes
    // Case 2 (0 < t < dt): Tracks shifted into other TPC, crossing points reconstructed (dt = readout window - drift time)
    // Case 3 (dt < t < readout window): Tracks shifted into other TPC, crossing points not reconstructed
    // Only dealing with case 3, first two solved by larreco module

    // Find all the pairs of tracks which should be stitched
    std::vector< std::pair<const recob::Track*, const recob::Track*>> trueMatches;
    std::vector<std::pair<std::string, double>> trueTimes;
    // Loop over tracks in first TPC
    for (auto const& track1 : tracksInTpc1){
      std::vector< art::Ptr<recob::Hit> > tpc1Hits = findManyHits.at(track1->ID());
      int tpc1TrueId = RecoUtils::TrueParticleIDFromTotalTrueEnergy(tpc1Hits);

      // Loop over tracks in second TPC
      for (auto const& track2 : tracksInTpc2){
        std::vector< art::Ptr<recob::Hit> > tpc2Hits = findManyHits.at(track2->ID());
        int tpc2TrueId = RecoUtils::TrueParticleIDFromTotalTrueEnergy(tpc2Hits);
        
        if (tpc1TrueId == tpc2TrueId && CrossesCathode(particles[tpc1TrueId]) && (particles[tpc1TrueId].T()*10e-9)/(0.5*10e-6) > dt){
          std::cout<<"Track IDs = "<<track1->ID()<<" "<<track2->ID()<<", True time = "<<(particles[tpc1TrueId].T()*10e-9)/(0.5*10e-6)<<std::endl;
          trueTimes.push_back(std::make_pair(std::to_string(track1->ID())+" + "+std::to_string(track2->ID()), (particles[tpc1TrueId].T()*10e-9)/(0.5*10e-6)));
          nCathodeCrossers++;
          trueMatches.push_back(std::make_pair(track1, track2));
        }

      } // End of loop over TPC 2 tracks
    } // End of loop over TPC 1 tracks

    // Find pairs of matched tracks
    std::vector<std::pair<std::string, double>> matchTimes;
    auto matches = StitchIncomplete(tracksInTpc1, tracksInTpc2, matchTimes, vCrtTimes);

    // Evaluate the performance of the track stitching
    std::vector<int> Results = Evaluate(trueMatches, matches);
    nStitched += matches.size();
    nCorrect += Results[0];
    nIncorrect += Results[1];
    nMissed += Results[2];

    // Record the difference between the calculated T0 for correctly stitched tracks and the true times
    if(fVerbose){
      std::cout<<"Correctly stitched tracks T0 estimation:"<<std::endl;
    }
    for (auto const& trueTime : trueTimes){
      for (auto const& matchTime : matchTimes){
        if (trueTime.first == matchTime.first){
          if(fVerbose){
            std::cout<<"Truth("<<trueTime.first<<") = "<<trueTime.second<<", reco("<<matchTime.first<<") = "<<matchTime.second<<", diff = "<<matchTime.second-trueTime.second<<std::endl;
          }
          fDifferenceHist->Fill(matchTime.second-trueTime.second);
        }
      }
    }

  } // TrackStitching::analyze()

  void TrackStitching::endJob(){

    // Output the results of the evaluation
    std::cout<<std::endl
             <<"Total number of cathode crossers           = "<<nCathodeCrossers<<std::endl
             <<"Total number of stitched reco tracks       = "<<nStitched<<std::endl
             <<"Number of correctly stitched reco tracks   = "<<nCorrect<<std::endl
             <<"Number of incorrectly stitched reco tracks = "<<nIncorrect<<std::endl
             <<"Number of missed reco tracks               = "<<nMissed<<std::endl;

  } // TrackStitching::endJob()

  // Function to match pairs of incomplete tracks
  std::vector<std::pair<const recob::Track*, const recob::Track*>> TrackStitching::StitchIncomplete(std::vector<const recob::Track*> tracks1,
                                                                                                    std::vector<const recob::Track*> tracks2,
                                                                                                    std::vector<std::pair<std::string, double>>& matchTimes,
                                                                                                    std::vector<double> crtTimes){
    // Vector of pairs for results
    std::vector< std::pair< const recob::Track*, const recob::Track* >> matches;

    // Convert matching angle limit to radians
    double cosLimit = cos(TMath::Pi() * fStitchAngle / 180.0);

    // Get detector properties
    double readoutWindow  = (double)fDetectorProperties->ReadOutWindowSize();
    double driftVelocity = fDetectorProperties->DriftVelocity();
    double driftTimeTicks = 4.0*fGeometryService->DetHalfWidth()/driftVelocity;
    double dt = readoutWindow - driftTimeTicks;
    double dx = (dt*0.5)*driftVelocity-2.6;

    // Match candidates in each TPC
    std::vector<std::pair<const recob::Track*, TVector3>> track1Candidates;
    std::vector<std::pair<const recob::Track*, TVector3>> track2Candidates;

    // Loop over tracks in first TPC
    for (auto const& track1 : tracks1){
      // If the start or end of the track matches the cutoff, add to a vector
      if (std::abs(track1->Vertex().X() - dx) < fDeltaX) track1Candidates.push_back(std::make_pair(track1, track1->VertexDirection()));
      if (std::abs(track1->End().X() - dx) < fDeltaX)    track1Candidates.push_back(std::make_pair(track1, track1->EndDirection()));
    }
    // Loop over tracks in second TPC
    for (auto const& track2 : tracks2){
      // If the start or end of the track matches the cutoff, add to a vector
      if (std::abs(track2->Vertex().X() + dx) < fDeltaX) track2Candidates.push_back(std::make_pair(track2, track2->VertexDirection()));
      if (std::abs(track2->End().X() + dx) < fDeltaX)    track2Candidates.push_back(std::make_pair(track2, track2->EndDirection()));
    }

    // If either vector has 0 candidates then there are no matches
    if (track1Candidates.size() == 0 || track2Candidates.size() == 0){}

    // If there is one track in each vector match tracks
    else if (track1Candidates.size() == 1 && track2Candidates.size() == 1){ 
      double angle = track1Candidates[0].second.Dot(track2Candidates[0].second);
      if (angle > cosLimit) matches.push_back(std::make_pair(track1Candidates[0].first, track2Candidates[0].first));
    }

    // If there is more than one in TPC 2 then find the best match in TPC 2
    else if (track1Candidates.size() == 1 && track2Candidates.size() > 1){ 
      for (size_t i = 0; i < track1Candidates.size(); i++){
        double bestAngle = -1.;
        double best_j = 0;
        for (size_t j = 0; j < track2Candidates.size(); j++){
          double angle = track1Candidates[i].second.Dot(track2Candidates[j].second);
          if (angle > bestAngle){ bestAngle = angle; best_j = j; }
        }
        if (bestAngle > cosLimit) matches.push_back(std::make_pair(track1Candidates[i].first, track2Candidates[best_j].first));
      }
    }

    // If there is more than one in TPC 1 then find the best match in TPC 1
    else if (track2Candidates.size() == 1 && track1Candidates.size() > 1){ 
      for (size_t i = 0; i < track2Candidates.size(); i++){
        double bestAngle = -1.;
        double best_j = 0;
        for (size_t j = 0; j < track1Candidates.size(); j++){
          double angle = track2Candidates[i].second.Dot(track1Candidates[j].second);
          if (angle > bestAngle){ bestAngle = angle; best_j = j; }
        }
        if (bestAngle > cosLimit) matches.push_back(std::make_pair(track1Candidates[best_j].first, track2Candidates[i].first));
      }
    }

    // If there is more than one track in both TPCs, evaluate all possible pairs and find the best matches
    else {
      // Calculate all possible pairs
      std::vector<std::pair<double,std::pair<const recob::Track*, const recob::Track*>>> allPairs;
      for (size_t i = 0; i < track1Candidates.size(); i++){
        for (size_t j = 0; j < track2Candidates.size(); j++){
          double angle = track1Candidates[i].second.Dot(track2Candidates[j].second);
          if (angle > cosLimit)
            allPairs.push_back(std::make_pair(angle, std::make_pair(track1Candidates[i].first, track2Candidates[j].first)));
        }
      }
      // Sort by angle
      std::sort(allPairs.rbegin(), allPairs.rend());
      // Record ID's of used tracks
      std::vector<int> usedTracks;
      for (size_t pair_i = 0; pair_i < allPairs.size(); pair_i++){
        // For each pair check that neither of the tracks have been used
        bool used = false;
        for (int usedID : usedTracks){
          if (allPairs[pair_i].second.first->ID() == usedID || allPairs[pair_i].second.second->ID() == usedID) used = true;
        }
        if (!used){ 
          matches.push_back(allPairs[pair_i].second);
          usedTracks.push_back(allPairs[pair_i].second.first->ID());
          usedTracks.push_back(allPairs[pair_i].second.second->ID());
        }
      }
    } 

    // Remove any duplicate matches, can occur when both cathode and anode are crossed
    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());

    // ------------------------------ T0 Estimation ---------------------------------
    // Loop over matches and try to estimate the start times 
    for (size_t i = 0; i < matches.size(); i++){

      TVector3 start1 = matches[i].first->Vertex();
      TVector3 end1 = matches[i].first->End();
      TVector3 start2 = matches[i].second->Vertex();
      TVector3 end2 = matches[i].second->End();

      // Shift tracks back into their own tpc
      start1.SetX(start1.X() - dx);
      end1.SetX(end1.X() - dx);
      start2.SetX(start2.X() + dx);
      end2.SetX(end2.X() + dx);

      // Minimize the distance between CPA crossing points of the two tracks
      ROOT::Math::Minimizer *min = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");
      min->SetMaxFunctionCalls(100000);
      min->SetMaxIterations(10000);
      min->SetTolerance(0.01);
      ROOT::Math::Functor f(&distance, 13);
      double step = 1;
      double variable[13] = {0, start1.X(), start1.Y(), start1.Z(), end1.X(), end1.Y(), end1.Z(), start2.X(), start2.Y(), start2.Z(), end2.X(), end2.Y(), end2.Z()};

      // Fix all variables in minimization except from dx
      min->SetFunction(f);
      min->SetVariable(0, "dx", variable[0], step);
      min->SetVariableLowerLimit(0, 0.0);
      min->SetFixedVariable(1, "sx1", variable[1]);
      min->SetFixedVariable(2, "sy1", variable[2]);
      min->SetFixedVariable(3, "sz1", variable[3]);
      min->SetFixedVariable(4, "ex1", variable[4]);
      min->SetFixedVariable(5, "ey1", variable[5]);
      min->SetFixedVariable(6, "ez1", variable[6]);
      min->SetFixedVariable(7, "sx2", variable[7]);
      min->SetFixedVariable(8, "sy2", variable[8]);
      min->SetFixedVariable(9, "sz2", variable[9]);
      min->SetFixedVariable(10, "ex2", variable[10]);
      min->SetFixedVariable(11, "ey2", variable[11]);
      min->SetFixedVariable(12, "ez2", variable[12]);
      // Do the minimization
      min->Minimize();
      // Get the results
      const double *output = min->X();
      // Shift is from a Gaussian fit to the original results
      double matchTime = dt + (output[0]/(0.5*driftVelocity))-30.3;
      std::cout<<"Track ID = "<<matches[i].first->ID()<<" "<<matches[i].second->ID()<<", Minimized time = "<<matchTime<<std::endl;

      // Loop over the CRT times
      bool correctMatch = false;
      for (size_t j = 0; j < crtTimes.size(); j++){
        // Calculate the difference between the calculated times and the CRT times
        double diff = std::abs(crtTimes[j] - matchTime);
        if (diff < fDeltaT) correctMatch = true;
      }
      // If there are no matches within a limit then remove the matched pair
      if (!correctMatch){ 
        matches.erase(matches.begin() + i);
        continue;
      }

      // Record the calculated T0's
      matchTimes.push_back(std::make_pair(std::to_string(matches[i].first->ID())+" + "+std::to_string(matches[i].second->ID()), matchTime));
    }

    return matches;
  } // TrackStitching::StitchIncomplete()

  // Function to evaluate the performance of track matching algorithms
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
          if(fVerbose){
            std::cout<<"Correct match: Track 1 ID = "<<foundTrack1Id<<", Track 2 ID = "<<foundTrack2Id<<std::endl;
          }
          //Calculate any variables and fill histograms
          double currentCos;
          double currentDif1;
          double currentDif2;
          MatchPoints((*trueMatch.first), (*trueMatch.second), currentCos, currentDif1, currentDif2);
          fCorrectAngleHist->Fill(TMath::ACos(currentCos)*180./TMath::Pi());
          fCorrectDeltaXHist->Fill(currentDif1);
          fCorrectDeltaXHist->Fill(currentDif2);
        }
      }

      // Calculate the number of incorrect matches by recording the number of pairs not in true vector
      if (!isCorrect){
        //Calculate any variables and fill histograms for incorrect tracks
        nIncorrect++;
        if(fVerbose){
          std::cout<<"Incorrect match: Track 1 ID = "<<foundTrack1Id<<", Track 2 ID = "<<foundTrack2Id<<std::endl;
        }
        double currentCos;
        double currentDif1;
        double currentDif2;
        MatchPoints((*foundMatch.first), (*foundMatch.second), currentCos, currentDif1, currentDif2);
        fIncorrectAngleHist->Fill(TMath::ACos(currentCos)*180./TMath::Pi());
        fIncorrectDeltaXHist->Fill(currentDif1);
        fIncorrectDeltaXHist->Fill(currentDif2);
      }
    }

    // Calculate the number of missed matches by recording the number of pairs not in the found vector
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
        if(fVerbose){
          std::cout<<"Missed match: Track 1 ID = "<<trueTrack1Id<<", Track 2 ID = "<<trueTrack2Id<<std::endl;
        }
        double currentCos;
        double currentDif1;
        double currentDif2;
        MatchPoints((*trueMatch.first), (*trueMatch.second), currentCos, currentDif1, currentDif2);
        fMissedAngleHist->Fill(TMath::ACos(currentCos)*180./TMath::Pi());
        fMissedDeltaXHist->Fill(currentDif1);
        fMissedDeltaXHist->Fill(currentDif2); 
      }
    }

    // Fill results vector
    std::vector<int> results{nCorrect, nIncorrect, nMissed};
    return results;

  } // TrackStitching::Evaluate()

  // Function to determine if track starts/ends match
  void TrackStitching::MatchPoints(recob::Track const& track1, recob::Track const& track2, double &cosResult, double &diffResult1, double &diffResult2){


    // Get detector properties
    double readoutWindow  = (double)fDetectorProperties->ReadOutWindowSize();
    double driftVelocity = fDetectorProperties->DriftVelocity();
    double driftTimeTicks = 4.0*fGeometryService->DetHalfWidth()/driftVelocity;
    double dt = readoutWindow - driftTimeTicks;
    double dx = (dt*0.5)*driftVelocity-2.6;
    // Convert matching angle limit to radians
    double cosLimit = cos(TMath::Pi() * fStitchAngle / 180.0);

    double cosComp = 1.;
    double cos = 1.;
    double diff1 = -1.;
    double diff2 = -1.;

    // Compare start and end x positions
    double difference1 = std::abs(std::abs(track1.Vertex().X())-dx);
    double difference2 = std::abs(std::abs(track2.Vertex().X())-dx);
    cosComp = track1.VertexDirection().Dot(track2.VertexDirection());
    if (difference1 < fDeltaX && difference2 < fDeltaX && cosComp > cosLimit){
      cos = cosComp;
      cosLimit = cos;
      diff1 = difference1;
      diff2 = difference2;
    }
    difference2 = std::abs(std::abs(track2.End().X())-dx);
    cosComp = track1.VertexDirection().Dot(track2.EndDirection());
    if (difference2 < fDeltaX && difference1 < fDeltaX && cosComp > cosLimit){
      cos = cosComp;
      cosLimit = cos;
      diff1 = difference1;
      diff2 = difference2;
    }
    difference1 = std::abs(std::abs(track1.End().X())-dx);
    if (difference1 < fDeltaX && difference2 < fDeltaX && cosComp > cosLimit){
      cos = cosComp;
      cosLimit = cos;
      diff1 = difference1;
      diff2 = difference2;
    }
    difference2 = std::abs(std::abs(track2.End().X())-dx);
    if (difference1 < fDeltaX && difference2 < fDeltaX && cosComp > cosLimit){
      cos = cosComp;
      diff1 = difference1;
      diff2 = difference2;
    }

    cosResult = cos;
    diffResult1 = diff1;
    diffResult2 = diff2;

  } // TrackStitching::MatchPoints()


  DEFINE_ART_MODULE(TrackStitching)
} // namespace sbnd

// Back to local namespace.
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

  // Function to calculate the distance between CPA crossing points for two lines
  double distance(const double *input){
    double dx = input[0];
    TVector3 start1(input[1], input[2], input[3]); 
    TVector3 end1(input[4], input[5], input[6]);
    TVector3 start2(input[7], input[8], input[9]);
    TVector3 end2(input[10], input[11], input[12]);

    // Initial CPA crossing points
    double y01 = (-start1.X() * (end1.Y() - start1.Y())) / (end1.X() - start1.X()) + start1.Y();
    double z01 = (-start1.X() * (end1.Z() - start1.Z())) / (end1.X() - start1.X()) + start1.Z();
    double y02 = (-start2.X() * (end2.Y() - start2.Y())) / (end2.X() - start2.X()) + start2.Y();
    double z02 = (-start2.X() * (end2.Z() - start2.Z())) / (end2.X() - start2.X()) + start2.Z();

    // Gradients for straight lines in 3D
    double m1 = (end1.Y() - start1.Y()) / (end1.X() - start1.X());
    double l1 = (end1.Z() - start1.Z()) / (end1.X() - start1.X());
    double m2 = (end2.Y() - start2.Y()) / (end2.X() - start2.X());
    double l2 = (end2.Z() - start2.Z()) / (end2.X() - start2.X());

    // Distance between CPA crossing points
    return TMath::Sqrt(pow((y01 + dx * m1) - (y02 -dx * m2), 2) + pow((z01 + dx * l1) - (z02 - dx * l2), 2));
  } // distance()
 
} // local namespace


