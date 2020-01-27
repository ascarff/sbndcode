////////////////////////////////////////////////////////////////////////
// Class:       SelectionTree
// Module Type: analyzer
// File:        SelectionTree_module.cc
//
// Tom Brooks (tbrooks@fnal.gov)
////////////////////////////////////////////////////////////////////////

// sbndcode includes
#include "sbndcode/RecoUtils/RecoUtils.h"
#include "sbndcode/CosmicId/Utils/CosmicIdUtils.h"
#include "sbndcode/CosmicId/Algs/CosmicIdAlg.h"
#include "sbndcode/Geometry/GeometryWrappers/TPCGeoAlg.h"
#include "sbndcode/CosmicId/Algs/StoppingParticleCosmicIdAlg.h"

// LArSoft includes
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"
#include "lardataobj/AnalysisBase/ParticleID.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "lardataobj/RecoBase/MCSFitResult.h"
#include "larreco/RecoAlg/TrajectoryMCSFitter.h"
#include "larreco/RecoAlg/TrackMomentumCalculator.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "larcoreobj/SummaryData/POTSummary.h"

// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Atom.h"

#include "Pandora/PdgTable.h"

// ROOT includes. Note: To look up the properties of the ROOT classes,
// use the ROOT web site; e.g.,
// <https://root.cern.ch/doc/master/annotated.html>
#include "TVector3.h"
#include "TH1.h"

// C++ includes
#include <map>
#include <vector>
#include <string>
#include <algorithm>

namespace sbnd {

  class SelectionTree : public art::EDAnalyzer {
  public:

    struct BeamTime {
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;

      fhicl::Atom<double> BeamTimeMin {
        Name("BeamTimeMin"),
        Comment("")
      };

      fhicl::Atom<double> BeamTimeMax {
        Name("BeamTimeMax"),
        Comment("")
      };

    };

    // Describes configuration parameters of the module
    struct Config {
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;
 
      // One Atom for each parameter
      fhicl::Atom<art::InputTag> SimModuleLabel {
        Name("SimModuleLabel"),
        Comment("tag of detector simulation data product")
      };

      fhicl::Atom<art::InputTag> GenModuleLabel {
        Name("GenModuleLabel"),
        Comment("tag of generator data product")
      };

      fhicl::Atom<art::InputTag> TpcTrackModuleLabel {
        Name("TpcTrackModuleLabel"),
        Comment("tag of TPC track producer data product")
      };

      fhicl::Atom<art::InputTag> ShowerModuleLabel {
        Name("ShowerModuleLabel"),
        Comment("tag of shower producer data product")
      };

      fhicl::Atom<art::InputTag> PidModuleLabel {
        Name("PidModuleLabel"),
        Comment("tag of PID producer data product")
      };

      fhicl::Atom<art::InputTag> CaloModuleLabel {
        Name("CaloModuleLabel"),
        Comment("tag of calorimetry producer data product")
      };

      fhicl::Atom<art::InputTag> PandoraLabel {
        Name("PandoraLabel"),
        Comment("tag of pandora data product")
      };

      fhicl::Atom<bool> Verbose {
        Name("Verbose"),
        Comment("Print information about what's going on")
      };

      fhicl::Table<CosmicIdAlg::Config> CosIdAlg {
        Name("CosIdAlg"),
      };

      fhicl::Table<trkf::TrajectoryMCSFitter::Config> fitter {
        Name("fitter"),
      };

      fhicl::Table<BeamTime> BeamTimeLimits {
        Name("BeamTimeLimits"),
        Comment("")
      };

      fhicl::Table<StoppingParticleCosmicIdAlg::Config> SPTagAlg {
        Name("SPTagAlg"),
      };

    }; // Inputs

    using Parameters = art::EDAnalyzer::Table<Config>;
 
    // Constructor: configures module
    explicit SelectionTree(Parameters const& config);
 
    // Called once, at start of the job
    virtual void beginJob() override;
 
    // Called for every sub run
    virtual void beginSubRun(const art::SubRun& subrun) override;
 
    // Called once per event
    virtual void analyze(const art::Event& event) override;

    // Called once, at end of the job
    virtual void endJob() override;

    // Reset variables in each loop
    void ResetPfpVars();
    void ResetCosPfpVars();
    void ResetNuMuVars();

    // Apply the proposal selection
    std::pair<std::pair<bool, bool>, recob::Track> ProposalSelection(std::vector<recob::Track> tracks);
    // Apply Rhiannon's selection
    std::pair<std::pair<bool, bool>, recob::Track> RhiSelection(std::vector<recob::Track> tracks, art::FindMany<anab::ParticleID> fmpid);

    std::pair<std::pair<bool, bool>, recob::Track> TomSelection(std::vector<recob::Track> tracks, art::FindMany<anab::ParticleID> fmpid, art::FindManyP<anab::Calorimetry> fmcalo);

    void FillSelectionTree(std::string selection, std::pair<std::pair<bool, bool>, recob::Track> selected, int trueId, std::map<int, simb::MCParticle> particles);

    double AverageDCA(const recob::Track& track);

    double HitEnergy(art::Ptr<recob::Hit> hit);
    double NeutrinoEnergy(std::map<int, double> track_energies, double shower_energy, int id);

    typedef art::Handle< std::vector<recob::PFParticle> > PFParticleHandle;
    typedef std::map< size_t, art::Ptr<recob::PFParticle> > PFParticleIdMap;

  private:

    // fcl file parameters
    art::InputTag fSimModuleLabel;      ///< name of detsim producer
    art::InputTag fGenModuleLabel;      ///< name of detsim producer
    art::InputTag fTpcTrackModuleLabel; ///< name of TPC track producer
    art::InputTag fShowerModuleLabel; ///< name of TPC track producer
    art::InputTag fPidModuleLabel; ///< name of TPC track producer
    art::InputTag fCaloModuleLabel; ///< name of TPC track producer
    art::InputTag fPandoraLabel;
    bool          fVerbose;             ///< print information about what's going on
    double        fBeamTimeMin;
    double        fBeamTimeMax;

    CosmicIdAlg cosIdAlg;
    TPCGeoAlg fTpcGeo;
    // Momentum fitters
    trkf::TrajectoryMCSFitter     fMcsFitter; 
    trkf::TrackMomentumCalculator fRangeFitter;
    StoppingParticleCosmicIdAlg  fStopTagger;
    detinfo::DetectorProperties const* fDetectorProperties;

    std::vector<std::string> selections {"prop", "rhi", "tom"};
    
    // Tree (One entry per reconstructed pfp)
    TTree *fPfpTree;

    //Pfp tree parameters
    bool is_cosmic;         // True origin of PFP is cosmic
    bool is_dirt;           // True origin of PFP is dirt interaction
    bool is_nu;             // True origin of PFP is nu in AV
    int nu_pdg;             // Pdg of neutrino if not cosmic
    bool is_cc;             // Is interaction CC if not cosmic
    int nu_int;             // Interaction type of neutrino if not cosmic
    double vtx_x;
    double vtx_y;
    double vtx_z;
    double p_x;
    double p_y;
    double p_z;
    double time;
    bool cosmic_id;         // ID'd as a cosmic
    int n_tracks;           // Number of reconstructed tracks
    double nu_energy;       // Energy of true neutrino
    bool mu_cont;           // Is true muon contained
    double mu_length;       // True contained length of muon if true numuCC
    double mu_mom;          // True momentum of muon if true numuCC
    double mu_theta;        // True theta of muon if true numuCC
    double mu_phi;          // True phi of muon if true numuCC
    std::map<std::string, bool> selected;      // Selected as numuCC?
    std::map<std::string, bool> in_fv;         // In fiducial volume of selection
    std::map<std::string, int> true_pdg;       // PDG of particle prop_true as muon
    std::map<std::string, bool> true_cont;     // Is selected true particle contained 
    std::map<std::string, double> true_length; // True contained length of selected particle
    std::map<std::string, double> true_mom;    // True momentum of selected particle
    std::map<std::string, double> true_theta;  // True theta of selected particle
    std::map<std::string, double> true_phi;    // True phi of selected particle
    std::map<std::string, bool> reco_cont;     // Is reconstructed track contained
    std::map<std::string, double> reco_length; // Selected muon reco length
    std::map<std::string, double> reco_mom;    // Selected muon reco momentum
    std::map<std::string, double> reco_theta;  // Selected muon reco theta
    std::map<std::string, double> reco_phi;    // Selected muon reco phi
    std::map<std::string, double> reco_nu_e;   // Reconstructed neutrino energy assuming nu_mu CC
    std::map<std::string, double> reco_vtx_x;  // Reconstructed neutrino vertex X
    std::map<std::string, double> reco_vtx_y;  // Reconstructed neutrino vertex Y
    std::map<std::string, double> reco_vtx_z;  // Reconstructed neutrino vertex Z

    // Tree (one entry per numu CC)
    TTree *fNuMuTree;

    // NuMu tree parameters;
    double nu_vtx_x;
    double nu_vtx_y;
    double nu_vtx_z;
    int nu_nu_pdg;
    double nu_p_x;
    double nu_p_y;
    double nu_p_z;
    double nu_nu_energy;
    double nu_mu_length;
    double nu_mu_mom;
    double nu_mu_theta;
    double nu_mu_phi;
    bool nu_mu_cont;

    TTree *fCosPfpTree;
    bool cos_is_cosmic;         // True origin of PFP is cosmic
    bool cos_is_dirt;           // True origin of PFP is dirt interaction
    bool cos_is_nu;             // True origin of PFP is nu in AV
    int cos_nu_pdg;             // Pdg of neutrino if not cosmic
    bool cos_is_cc;             // Is interaction CC if not cosmic
    double cos_vtx_x;
    double cos_vtx_y;
    double cos_vtx_z;
    bool cos_cont;
    double cos_mom;
    double cos_length;
    double cos_theta;
    double cos_phi;

    TTree *fPotTree;
    double pot;

    void GetPFParticleIdMap(const PFParticleHandle &pfParticleHandle, PFParticleIdMap &pfParticleMap);

  }; // class SelectionTree


  // Constructor
  SelectionTree::SelectionTree(Parameters const& config)
    : EDAnalyzer(config)
    , fSimModuleLabel       (config().SimModuleLabel())
    , fGenModuleLabel       (config().GenModuleLabel())
    , fTpcTrackModuleLabel  (config().TpcTrackModuleLabel())
    , fShowerModuleLabel    (config().ShowerModuleLabel())
    , fPidModuleLabel       (config().PidModuleLabel())
    , fCaloModuleLabel      (config().CaloModuleLabel())
    , fPandoraLabel         (config().PandoraLabel())
    , fVerbose              (config().Verbose())
    , fBeamTimeMin          (config().BeamTimeLimits().BeamTimeMin())
    , fBeamTimeMax          (config().BeamTimeLimits().BeamTimeMax())
    , cosIdAlg              (config().CosIdAlg())
    , fMcsFitter            (config().fitter)
    , fStopTagger           (config().SPTagAlg())
  {

  } // SelectionTree()


  void SelectionTree::beginJob()
  {

    fDetectorProperties = lar::providerFrom<detinfo::DetectorPropertiesService>();
    
    ResetPfpVars();

    // Access tfileservice to handle creating and writing histograms
    art::ServiceHandle<art::TFileService> tfs;

    fPfpTree = tfs->make<TTree>("pfps", "pfps");

    fPfpTree->Branch("is_cosmic", &is_cosmic);
    fPfpTree->Branch("is_dirt",   &is_dirt);
    fPfpTree->Branch("is_nu",     &is_nu);
    fPfpTree->Branch("nu_pdg",    &nu_pdg);
    fPfpTree->Branch("is_cc",     &is_cc);
    fPfpTree->Branch("nu_int",    &nu_int);
    fPfpTree->Branch("vtx_x",     &vtx_x);
    fPfpTree->Branch("vtx_y",     &vtx_y);
    fPfpTree->Branch("vtx_z",     &vtx_z);
    fPfpTree->Branch("p_x",       &p_x);
    fPfpTree->Branch("p_y",       &p_y);
    fPfpTree->Branch("p_z",       &p_z);
    fPfpTree->Branch("time",      &time);
    fPfpTree->Branch("cosmic_id", &cosmic_id);
    fPfpTree->Branch("n_tracks",  &n_tracks);
    fPfpTree->Branch("nu_energy", &nu_energy);
    fPfpTree->Branch("mu_cont",   &mu_cont);
    fPfpTree->Branch("mu_length", &mu_length);
    fPfpTree->Branch("mu_mom",    &mu_mom);
    fPfpTree->Branch("mu_theta",  &mu_theta);
    fPfpTree->Branch("mu_phi",    &mu_phi);
    for(auto const& sel : selections){
      fPfpTree->Branch((sel+"_selected").c_str(),   &selected[sel]);
      fPfpTree->Branch((sel+"_in_fv").c_str(),      &in_fv[sel]);
      fPfpTree->Branch((sel+"_true_pdg").c_str(),   &true_pdg[sel]);
      fPfpTree->Branch((sel+"_true_cont").c_str(),  &true_cont[sel]);
      fPfpTree->Branch((sel+"_true_length").c_str(),&true_length[sel]);
      fPfpTree->Branch((sel+"_true_mom").c_str(),   &true_mom[sel]);
      fPfpTree->Branch((sel+"_true_theta").c_str(), &true_theta[sel]);
      fPfpTree->Branch((sel+"_true_phi").c_str(),   &true_phi[sel]);
      fPfpTree->Branch((sel+"_reco_cont").c_str(),  &reco_cont[sel]);
      fPfpTree->Branch((sel+"_reco_length").c_str(),&reco_length[sel]);
      fPfpTree->Branch((sel+"_reco_mom").c_str(),   &reco_mom[sel]);
      fPfpTree->Branch((sel+"_reco_theta").c_str(), &reco_theta[sel]);
      fPfpTree->Branch((sel+"_reco_phi").c_str(),   &reco_phi[sel]);
      fPfpTree->Branch((sel+"_reco_nu_e").c_str(),  &reco_nu_e[sel]);
      fPfpTree->Branch((sel+"_reco_vtx_x").c_str(), &reco_vtx_x[sel]);
      fPfpTree->Branch((sel+"_reco_vtx_y").c_str(), &reco_vtx_y[sel]);
      fPfpTree->Branch((sel+"_reco_vtx_z").c_str(), &reco_vtx_z[sel]);
    }

    fNuMuTree = tfs->make<TTree>("numu", "numu");

    fNuMuTree->Branch("nu_vtx_x", &nu_vtx_x);
    fNuMuTree->Branch("nu_vtx_y", &nu_vtx_y);
    fNuMuTree->Branch("nu_vtx_z", &nu_vtx_z);
    fNuMuTree->Branch("nu_nu_pdg", &nu_nu_pdg);
    fNuMuTree->Branch("nu_p_x",   &nu_p_x);
    fNuMuTree->Branch("nu_p_y",   &nu_p_y);
    fNuMuTree->Branch("nu_p_z",   &nu_p_z);
    fNuMuTree->Branch("nu_nu_energy", &nu_nu_energy);
    fNuMuTree->Branch("nu_mu_length", &nu_mu_length);
    fNuMuTree->Branch("nu_mu_mom", &nu_mu_mom);
    fNuMuTree->Branch("nu_mu_theta", &nu_mu_theta);
    fNuMuTree->Branch("nu_mu_phi", &nu_mu_phi);
    fNuMuTree->Branch("nu_mu_cont", &nu_mu_cont);

    fCosPfpTree = tfs->make<TTree>("cospfps", "cospfps");
    fCosPfpTree->Branch("cos_is_cosmic", &cos_is_cosmic);
    fCosPfpTree->Branch("cos_is_dirt",   &cos_is_dirt);
    fCosPfpTree->Branch("cos_is_nu",     &cos_is_nu);
    fCosPfpTree->Branch("cos_nu_pdg",    &cos_nu_pdg);
    fCosPfpTree->Branch("cos_is_cc",     &cos_is_cc);
    fCosPfpTree->Branch("cos_vtx_x",     &cos_vtx_x);
    fCosPfpTree->Branch("cos_vtx_y",     &cos_vtx_y);
    fCosPfpTree->Branch("cos_vtx_z",     &cos_vtx_z);
    fCosPfpTree->Branch("cos_cont",  &cos_cont);
    fCosPfpTree->Branch("cos_mom",  &cos_mom);
    fCosPfpTree->Branch("cos_length",  &cos_length);
    fCosPfpTree->Branch("cos_theta",  &cos_theta);
    fCosPfpTree->Branch("cos_phi",  &cos_phi);

    fPotTree = tfs->make<TTree>("pots", "pots");
    fPotTree->Branch("pot",  &pot);

    // Initial output
    if(fVerbose) std::cout<<"----------------- Cosmic ID Ana Module -------------------"<<std::endl;

  }// SelectionTree::beginJob()

  // Called for every sub run
  void SelectionTree::beginSubRun(const art::SubRun& subrun){

    art::Handle< sumdata::POTSummary > potHandle;
    if(subrun.getByLabel(fGenModuleLabel, potHandle)){
      const sumdata::POTSummary& potSum = (*potHandle);
      pot = potSum.totpot;
    }

    fPotTree->Fill();

    return;
  }

  void SelectionTree::analyze(const art::Event& event)
  {

    // Fetch basic event info
    if(fVerbose){
      std::cout<<"============================================"<<std::endl
               <<"Run = "<<event.run()<<", SubRun = "<<event.subRun()<<", Event = "<<event.id().event()<<std::endl
               <<"============================================"<<std::endl;
    }

    //----------------------------------------------------------------------------------------------------------
    //                                          GETTING PRODUCTS
    //----------------------------------------------------------------------------------------------------------

    // Get truth info and matching
    art::ServiceHandle<cheat::ParticleInventoryService> pi_serv;
    // Retrieve all the truth info in the events
    auto particleHandle = event.getValidHandle<std::vector<simb::MCParticle>>(fSimModuleLabel);
    // Put them in a map for easier access
    std::map<int, simb::MCParticle> particles;
    std::vector<simb::MCParticle> parts;
    for (auto const& particle: (*particleHandle)){
      // Store particle
      int partId = particle.TrackId();
      particles[partId] = particle;
      parts.push_back(particle);
    }

    art::Handle<std::vector<simb::MCTruth>> genHandle;
    std::vector<art::Ptr<simb::MCTruth>> mctruthList;
    if(event.getByLabel(fGenModuleLabel, genHandle)) art::fill_ptr_vector(mctruthList, genHandle);


    // Get PFParticles from pandora
    PFParticleHandle pfParticleHandle;
    event.getByLabel(fPandoraLabel, pfParticleHandle);
    if( !pfParticleHandle.isValid() ){
      if(fVerbose) std::cout<<"Failed to find the PFParticles."<<std::endl;
      return;
    }
    PFParticleIdMap pfParticleMap;
    this->GetPFParticleIdMap(pfParticleHandle, pfParticleMap);
    // Get PFParticle to track associations
    art::FindManyP< recob::Track > pfPartToTrackAssoc(pfParticleHandle, event, fTpcTrackModuleLabel);
    
    // Get track to hit and colorimetry associations
    auto tpcTrackHandle = event.getValidHandle<std::vector<recob::Track>>(fTpcTrackModuleLabel);
    art::FindManyP<recob::Hit> findManyHits(tpcTrackHandle, event, fTpcTrackModuleLabel);
    art::FindMany<anab::ParticleID> findManyPid(tpcTrackHandle, event, fPidModuleLabel);
    art::FindManyP<anab::Calorimetry> findManyCalo(tpcTrackHandle, event, fCaloModuleLabel);

    // Get shower handle
    auto showerHandle = event.getValidHandle<std::vector<recob::Shower>>(fShowerModuleLabel);
    // Get PFParticle to shower associations
    art::FindManyP< recob::Shower > pfPartToShowerAssoc(pfParticleHandle, event, fShowerModuleLabel);
    // Get shower to hit associations
    art::FindManyP<recob::Hit> findManyHitsShower(showerHandle, event, fShowerModuleLabel);

    //----------------------------------------------------------------------------------------------------------
    //                                      FILLING THE TRUTH TREE
    //----------------------------------------------------------------------------------------------------------

    for (size_t i = 0; i < mctruthList.size(); i++){

      // Get the pointer to the MCTruth object
      art::Ptr<simb::MCTruth> truth = mctruthList.at(i);

      if(truth->Origin() != simb::kBeamNeutrino) continue;

      // Push back all unique neutrino energies
      bool numucc = false;
      if(truth->GetNeutrino().CCNC() == simb::kCC && std::abs(truth->GetNeutrino().Nu().PdgCode()) == 14) numucc = true;

      // Get truth info if numuCC in AV
      if(!numucc) continue;
      geo::Point_t vtx {truth->GetNeutrino().Nu().Vx(), 
                        truth->GetNeutrino().Nu().Vy(), 
                        truth->GetNeutrino().Nu().Vz()};
      if(!fTpcGeo.InFiducial(vtx, 0.)) continue;

      ResetNuMuVars();

      nu_vtx_x = vtx.X();
      nu_vtx_y = vtx.Y();
      nu_vtx_z = vtx.Z();
      nu_nu_pdg = truth->GetNeutrino().Nu().PdgCode();
      nu_p_x = truth->GetNeutrino().Nu().Px();
      nu_p_y = truth->GetNeutrino().Nu().Py();
      nu_p_z = truth->GetNeutrino().Nu().Pz();
      nu_nu_energy = truth->GetNeutrino().Nu().E();

      // Get the primary muon
      std::vector<const simb::MCParticle*> parts = pi_serv->MCTruthToParticles_Ps(truth);
      for(auto const& part : parts){
        if(std::abs(part->PdgCode()) != 13) continue;
        if(part->Mother() != 0) continue;
        if(part->StatusCode() != 1) continue;

        nu_mu_length = fTpcGeo.TpcLength(*part);
        nu_mu_mom = part->P();
        TVector3 start(part->Vx(), part->Vy(), part->Vz());
        TVector3 end(part->EndX(), part->EndY(), part->EndZ());
        nu_mu_theta = (end-start).Theta();
        nu_mu_phi = (end-start).Phi();
        nu_mu_cont = fTpcGeo.IsContained(*part);
      }

      fNuMuTree->Fill();
    }

    //----------------------------------------------------------------------------------------------------------
    //                                      FAKE PDS RECONSTRUCTION
    //----------------------------------------------------------------------------------------------------------
/*
    // Create fake flashes in each tpc
    std::pair<std::vector<double>, std::vector<double>> fakeFlashes = CosmicIdUtils::FakeTpcFlashes(parts);
    std::vector<double> fakeTpc0Flashes = fakeFlashes.first;
    std::vector<double> fakeTpc1Flashes = fakeFlashes.second;
    bool tpc0BeamFlash = CosmicIdUtils::BeamFlash(fakeTpc0Flashes, fBeamTimeMin, fBeamTimeMax);
    bool tpc1BeamFlash = CosmicIdUtils::BeamFlash(fakeTpc1Flashes, fBeamTimeMin, fBeamTimeMax);

    // If there are no flashes in time with the beam then ignore the event
    if(!tpc0BeamFlash && !tpc1BeamFlash) return;
*/
    //----------------------------------------------------------------------------------------------------------
    //                                     FILLING THE SELECTION TREE
    //----------------------------------------------------------------------------------------------------------

    //Loop over the pfparticle map
    std::vector<double> used_nus;
    for (PFParticleIdMap::const_iterator it = pfParticleMap.begin(); it != pfParticleMap.end(); ++it){

      const art::Ptr<recob::PFParticle> pParticle(it->second);
      // Only look for primary particles
      if (!pParticle->IsPrimary()) continue;
      // Check if this particle is identified as the neutrino
      const int pdg(pParticle->PdgCode());
      const bool isNeutrino(std::abs(pdg) == pandora::NU_E || 
                            std::abs(pdg) == pandora::NU_MU || 
                            std::abs(pdg) == pandora::NU_TAU);
      //Find neutrino pfparticle
      if(!isNeutrino) continue;

      ResetPfpVars();

      std::vector<recob::Track> nuTracks;
      std::map<int, double> track_energies;
      double shower_energy = 0;

      // Loop over daughters of pfparticle and do some truth matching
      // Assign labels based on the particle constributing the most hits
      std::vector<art::Ptr<recob::Hit>> all_hits;
      for (const size_t daughterId : pParticle->Daughters()){

        // Get tracks associated with daughter
        art::Ptr<recob::PFParticle> pDaughter = pfParticleMap.at(daughterId);
        const std::vector< art::Ptr<recob::Track> > associatedTracks(pfPartToTrackAssoc.at(pDaughter.key()));
        const std::vector< art::Ptr<recob::Shower> > associatedShowers(pfPartToShowerAssoc.at(pDaughter.key()));

        // Add up track and shower energy
        for(size_t i = 0; i < associatedTracks.size(); i++){
          std::vector<art::Ptr<anab::Calorimetry>> calos = findManyCalo.at(associatedTracks[i]->ID());
          if(calos.size()==0) continue;
          size_t nhits = 0;
          size_t best_plane = 0;
          for( size_t j = calos.size(); j > 0; j--){
            if(calos[j-1]->dEdx().size() > nhits*1.5){
              nhits = calos[j-1]->dEdx().size();
              best_plane = j-1;
            }
          }
          track_energies[associatedTracks[i]->ID()] = calos[best_plane]->KineticEnergy()/1e3;
        }
        for(size_t i = 0; i < associatedShowers.size(); i++){
          std::vector<art::Ptr<recob::Hit>> hits = findManyHitsShower.at(associatedShowers[i]->ID());
          for(size_t j = 0; j < hits.size(); j++){
            // Assume collection plane is best for showers
            if(hits[j]->WireID().Plane == 2){ 
              shower_energy += HitEnergy(hits[j]);
            }
          }
        }

        if(associatedTracks.size() != 1) continue; //TODO check how often this occurs

        // Get the first associated track
        recob::Track tpcTrack = *associatedTracks.front();
        nuTracks.push_back(tpcTrack);

        // Truth match the pfps using all hits associated to all tracks associated to neutrino pfp
        std::vector<art::Ptr<recob::Hit>> hits = findManyHits.at(tpcTrack.ID());
        all_hits.insert(all_hits.end(), hits.begin(), hits.end());

      }

      int trueId = RecoUtils::TrueParticleIDFromTotalRecoHits(all_hits, false);

      // Skip if no corresponding true particle
      if(particles.find(trueId) == particles.end()) continue;

      // Get the origin of the particle
      art::Ptr<simb::MCTruth> truth = pi_serv->TrackIdToMCTruth_P(trueId);
      if(truth->Origin() == simb::kBeamNeutrino){
        // Save neutrino interaction info
        nu_pdg = truth->GetNeutrino().Nu().PdgCode();
        if(truth->GetNeutrino().CCNC() == simb::kCC) is_cc = true;
        nu_int = truth->GetNeutrino().InteractionType();
        nu_energy = truth->GetNeutrino().Nu().E();

        // Avoid double counting neutrinos
        // FIXME if this ever happens need better way of deciding which pfp to keep
        if(std::find(used_nus.begin(), used_nus.end(), nu_energy) != used_nus.end()) continue;
        used_nus.push_back(nu_energy);

        // If neutrino vertex is not inside the TPC then call it a dirt particle
        geo::Point_t vtx {truth->GetNeutrino().Nu().Vx(), 
                          truth->GetNeutrino().Nu().Vy(), 
                          truth->GetNeutrino().Nu().Vz()};
        vtx_x = vtx.X();
        vtx_y = vtx.Y();
        vtx_z = vtx.Z();
        p_x = truth->GetNeutrino().Nu().Px();
        p_y = truth->GetNeutrino().Nu().Py();
        p_z = truth->GetNeutrino().Nu().Pz();
        if(!fTpcGeo.InFiducial(vtx, 0, 0)) is_dirt = true;
        else is_nu = true;

        // If it's a numuCC then save the muon kinematics
        // Get the primary muon
        std::vector<const simb::MCParticle*> parts = pi_serv->MCTruthToParticles_Ps(truth);
        for(auto const& part : parts){
          if(std::abs(part->PdgCode()) != 13) continue;
          if(part->Mother() != 0) continue;
          if(part->StatusCode() != 1) continue;
          TVector3 start(part->Vx(), part->Vy(), part->Vz());
          TVector3 end(part->EndX(), part->EndY(), part->EndZ());
          mu_length = fTpcGeo.TpcLength(*part);
          mu_mom = part->P();
          mu_theta = (end-start).Theta();
          mu_phi = (end-start).Phi();
          mu_cont = fTpcGeo.IsContained(*part);
        }

      }
      //else if(truth->Origin() == simb::kCosmicRay) is_cosmic = true;
      else is_cosmic = true;

      n_tracks = nuTracks.size();

      // Skip any PFPs without any tracks in them // TODO check how many numuCC this misses
      if(n_tracks == 0) continue;

      // Does pfp look like a cosmic?
      //cosmic_id = cosIdAlg.CosmicId(*pParticle, pfParticleMap, event, fakeTpc0Flashes, fakeTpc1Flashes);
      cosmic_id = cosIdAlg.CosmicId(*pParticle, pfParticleMap, event);

      // -------------------------------------- APPLY SELECTIONS ---------------------------------------
      for(auto const& sel : selections){
        if(sel == "prop"){
          std::pair<std::pair<bool, bool>, recob::Track> sel_track = ProposalSelection(nuTracks);
          std::vector<art::Ptr<recob::Hit>> hits = findManyHits.at(sel_track.second.ID());
          int trueId = RecoUtils::TrueParticleIDFromTotalRecoHits(hits, false);
          FillSelectionTree(sel, sel_track, trueId, particles);
          reco_nu_e[sel] = NeutrinoEnergy(track_energies, shower_energy, sel_track.second.ID());
        }
        if(sel == "rhi"){
          std::pair<std::pair<bool, bool>, recob::Track> sel_track = RhiSelection(nuTracks, findManyPid);
          std::vector<art::Ptr<recob::Hit>> hits = findManyHits.at(sel_track.second.ID());
          int trueId = RecoUtils::TrueParticleIDFromTotalRecoHits(hits, false);
          FillSelectionTree(sel, sel_track, trueId, particles);
          reco_nu_e[sel] = NeutrinoEnergy(track_energies, shower_energy, sel_track.second.ID());
        } 
        if(sel == "tom"){
          std::pair<std::pair<bool, bool>, recob::Track> sel_track = TomSelection(nuTracks, findManyPid, findManyCalo);
          std::vector<art::Ptr<recob::Hit>> hits = findManyHits.at(sel_track.second.ID());
          int trueId = RecoUtils::TrueParticleIDFromTotalRecoHits(hits, false);
          FillSelectionTree(sel, sel_track, trueId, particles);
          reco_nu_e[sel] = NeutrinoEnergy(track_energies, shower_energy, sel_track.second.ID());
        } 
      }

      fPfpTree->Fill();

    }  

    // Fill small tree of tracks identified as cosmics
    for (PFParticleIdMap::const_iterator it = pfParticleMap.begin(); it != pfParticleMap.end(); ++it){

      const art::Ptr<recob::PFParticle> pParticle(it->second);
      // Only look for primary particles
      if (!pParticle->IsPrimary()) continue;
      // Check if this particle is identified as the neutrino
      const int pdg(pParticle->PdgCode());
      const bool isNeutrino(std::abs(pdg) == pandora::NU_E || 
                            std::abs(pdg) == pandora::NU_MU || 
                            std::abs(pdg) == pandora::NU_TAU);
      //Find neutrino pfparticle
      if(isNeutrino) continue;

      ResetCosPfpVars();

      const std::vector< art::Ptr<recob::Track> > associatedTracks(pfPartToTrackAssoc.at(pParticle.key()));
      if(associatedTracks.size() != 1) continue; //TODO check how often this occurs

      // Get the first associated track
      recob::Track tpcTrack = *associatedTracks.front();
      cos_cont = fTpcGeo.InFiducial(tpcTrack.End(), 1.5);
      cos_length = tpcTrack.Length();
      cos_mom = 0.;
      if(cos_cont){
        cos_mom = fRangeFitter.GetTrackMomentum(cos_length, 13);
      }
      else{
        recob::MCSFitResult mcsResult = fMcsFitter.fitMcs(tpcTrack);
        cos_mom = mcsResult.bestMomentum();
      }
      cos_theta = tpcTrack.Theta();
      cos_phi = tpcTrack.Phi();

      // Truth match the pfps using all hits associated to all tracks associated to neutrino pfp
      std::vector<art::Ptr<recob::Hit>> hits = findManyHits.at(tpcTrack.ID());
      int trueId = RecoUtils::TrueParticleIDFromTotalRecoHits(hits, false);
      // Skip if no corresponding true particle
      if(particles.find(trueId) == particles.end()) continue;
      // Get the origin of the particle
      art::Ptr<simb::MCTruth> truth = pi_serv->TrackIdToMCTruth_P(trueId);
      if(truth->Origin() == simb::kBeamNeutrino){
        // Save neutrino interaction info
        cos_nu_pdg = truth->GetNeutrino().Nu().PdgCode();
        if(truth->GetNeutrino().CCNC() == simb::kCC) cos_is_cc = true;

        // If neutrino vertex is not inside the TPC then call it a dirt particle
        geo::Point_t vtx {truth->GetNeutrino().Nu().Vx(), 
                          truth->GetNeutrino().Nu().Vy(), 
                          truth->GetNeutrino().Nu().Vz()};
        if(!fTpcGeo.InFiducial(vtx, 0, 0)) cos_is_dirt = true;
        else cos_is_nu = true;
      }
      //else if(truth->Origin() == simb::kCosmicRay) cos_is_cosmic = true;
      else cos_is_cosmic = true;

      fCosPfpTree->Fill();
    }
    
  } // SelectionTree::analyze()


  void SelectionTree::endJob(){

  } // SelectionTree::endJob()

  void SelectionTree::GetPFParticleIdMap(const PFParticleHandle &pfParticleHandle, PFParticleIdMap &pfParticleMap){
      for (unsigned int i = 0; i < pfParticleHandle->size(); ++i){
          const art::Ptr<recob::PFParticle> pParticle(pfParticleHandle, i);
          if (!pfParticleMap.insert(PFParticleIdMap::value_type(pParticle->Self(), pParticle)).second){
              std::cout << "  Unable to get PFParticle ID map, the input PFParticle collection has repeat IDs!" <<"\n";
          }
      }
  }

  // Reset the tree variables
  void SelectionTree::ResetPfpVars(){
    is_cosmic = false;
    is_dirt = false;
    is_nu = false;
    nu_pdg = -99999;
    is_cc = false;
    nu_int = -99999;
    vtx_x = -99999;
    vtx_y = -99999;
    vtx_z = -99999;
    p_x = -99999;
    p_y = -99999;
    p_z = -99999;
    time = -99999;
    cosmic_id = false;
    n_tracks = 0;
    nu_energy = -99999;
    mu_cont = false;
    mu_length = -99999;
    mu_mom = -99999;
    mu_theta = -99999;
    mu_phi = -99999;
    for(auto const& sel : selections){
      selected[sel] = false;
      true_pdg[sel] = -99999;
      true_cont[sel] = false;
      true_length[sel] = -99999;
      true_mom[sel] = -99999;
      true_theta[sel] = -99999;
      true_phi[sel] = -99999;
      reco_cont[sel] = false;
      reco_length[sel] = -99999;
      reco_mom[sel] = -99999;
      reco_theta[sel] = -99999;
      reco_phi[sel] = -99999;
      reco_nu_e[sel] = -99999;
      reco_vtx_x[sel] = -99999;
      reco_vtx_y[sel] = -99999;
      reco_vtx_z[sel] = -99999;
    }
  }

  void SelectionTree::ResetNuMuVars(){
    nu_vtx_x = -99999;
    nu_vtx_y = -99999;
    nu_vtx_z = -99999;
    nu_p_x = -99999;
    nu_p_y = -99999;
    nu_p_z = -99999;
    nu_nu_energy = -99999;
    nu_mu_mom = -99999;
    nu_mu_theta = -99999;
    nu_mu_phi = -99999;
    nu_mu_cont = false;
  }


  void SelectionTree::ResetCosPfpVars(){
    cos_is_cosmic = false;
    cos_is_dirt = false;
    cos_is_nu = false;
    cos_nu_pdg = -99999;
    cos_is_cc = false;
    cos_vtx_x = -99999;
    cos_vtx_y = -99999;
    cos_vtx_z = -99999;
    cos_cont = false;
    cos_mom = -99999;
    cos_length = -99999;
    cos_theta = -99999;
    cos_phi = -99999;
  }

  // Apply the proposal selection
  std::pair<std::pair<bool, bool>, recob::Track> SelectionTree::ProposalSelection(std::vector<recob::Track> tracks){

    bool is_selected = false;

    // Get the longest track and ID as muon
    std::sort(tracks.begin(), tracks.end(), [](auto& left, auto& right){
              return left.Length() > right.Length();});
    recob::Track track = tracks[0];
    double length = track.Length();

    // Check if the track is contained
    bool track_contained = fTpcGeo.InFiducial(track.End(), 1.5);

    // Apply a fiducial volume cut to the vertex (start of track) TODO CPA cut
    bool in_fiducial = fTpcGeo.InFiducial(track.Start(), 16.5, 15., 15., 16.5, 15., 80.);

    // Apply selection criteria
    if(track_contained){
      if(length > 50.) is_selected = true;
    }
    else{
      if(length > 100.) is_selected = true;
    }

    return std::make_pair(std::make_pair(is_selected, in_fiducial), track);
  }

  // Apply rhiannon's selection
  std::pair<std::pair<bool, bool>, recob::Track> SelectionTree::RhiSelection(std::vector<recob::Track> tracks, art::FindMany<anab::ParticleID> fmpid){

    bool is_selected = false;
    bool has_candidate = false;
    recob::Track candidate = tracks[0];

    // Loop over tracks and count how many escape
    int n_escape = 0;
    double longest_escape = 0;
    int longest_i = -99999;
    double longest_first = 0;
    double longest_second = 0;
    for(size_t i = 0; i < tracks.size(); i++){
      if(!fTpcGeo.InFiducial(tracks[i].End(), 1.5)){ //TODO containment def 
        n_escape++;
        //double length = fTpcGeo.LengthInFiducial(tracks[i], 10, 20, 10, 10, 20, 10);
        double length = tracks[i].Length();
        if(length > longest_escape){ 
          longest_escape = length;
          longest_i = i;
        }
      }

      if(tracks[i].Length() > longest_first){
        longest_second = longest_first;
        longest_first = tracks[i].Length();
      }
      else if(tracks[i].Length() > longest_second){
        longest_second = tracks[i].Length();
      }
    }

    // Case 1: 1 escaping track
    if(n_escape == 1){
      // If track longer than 100 cm then ID as muon
      // If more than 1 escaping track > 100 cm then choose longest
      if(longest_escape > 100){
        has_candidate = true;
        candidate = tracks[longest_i];
      }
      // Else don't select ? TODO what if long contained track but short exiting one
    }

    // Case 2: everything else
    else if(n_escape == 0){
      std::vector<std::pair<recob::Track, double>> candidates;
      for(auto const& track : tracks){
        std::vector<const anab::ParticleID*> pids = fmpid.at(track.ID());
        bool is_proton = false;
        double muon_chi = 99999;
        for(size_t i = 0; i < pids.size(); i++){
          // Only use the collection plane
          if(pids[i]->PlaneID().Plane != 2) continue;
          // Run proton chi^2 to cut out stopping protons
          if(pids[i]->Chi2Proton() < 80){ 
            is_proton = true;
            continue;
          }
          // Muon candidates = longest by 1.5x and any with low muon chi^2
          muon_chi = pids[i]->Chi2Muon();
        }
        if(!is_proton && (muon_chi<16 || track.Length() >= 1.5*longest_second)) 
          candidates.push_back(std::make_pair(track, muon_chi));
      }
      // 0 candidates = don't select
      // 1 candidate = select as muon
      if(candidates.size() == 1){
        has_candidate = true;
        candidate = candidates[0].first;
      }
      // >1 candidate = Select longest if it was a candidate or track with lowest muon chi^2
      else if(candidates.size() > 1){
        // Sort by length
        std::sort(candidates.begin(), candidates.end(), [](auto& left, auto& right){
              return left.first.Length() > right.first.Length();});
        if(candidates[0].first.Length() >= 1.5*longest_second){
          has_candidate = true;
          candidate = candidates[0].first;
        }
        else{
          // Sort by muon chi2
          std::sort(candidates.begin(), candidates.end(), [](auto& left, auto& right){
                return left.second < right.second;});
          has_candidate = true;
          candidate = candidates[0].first;
        }
      }
    }

    // Check vertex (start of muon candidate) in FV
    // Fiducial definitions 8.25 cm from X (inc CPA), 15 cm from Y and front, 85 cm from back
    bool in_fiducial = fTpcGeo.InFiducial(candidate.Start(), 8.25, 15., 15., 8.25, 15., 85., 8.25);
    if(has_candidate) is_selected = true;

    return std::make_pair(std::make_pair(is_selected, in_fiducial), candidate);
  }

  // Apply my selection
  std::pair<std::pair<bool, bool>, recob::Track> SelectionTree::TomSelection(std::vector<recob::Track> tracks, art::FindMany<anab::ParticleID> fmpid, art::FindManyP<anab::Calorimetry> fmcalo){

    bool is_selected = false;
    bool has_candidate = false;
    recob::Track candidate = tracks[0];

    // Loop over tracks and count how many escape
    // For contained tracks apply PID cuts to only retain muon-like tracks
    int n_escape = 0;
    double longest_escape = 0;
    std::vector<recob::Track> long_tracks;
    for(size_t i = 0; i < tracks.size(); i++){
      // Find longest escaping track and don't apply track cuts if escaping
      if(!fTpcGeo.InFiducial(tracks[i].End(), 1.5)){
        n_escape++;
        double length = tracks[i].Length();
        if(length > longest_escape){ 
          longest_escape = length;
        }
        long_tracks.push_back(tracks[i]);
        continue;
      }

      // Select if longer than 150 cm
      if(tracks[i].Length() > 100.){
        long_tracks.push_back(tracks[i]);
        continue;
      }

      // Loop over planes (Y->V->U) and choose the next plane's calorimetry if there are 1.5x more points (collection plane more reliable)
      std::vector<art::Ptr<anab::Calorimetry>> calos = fmcalo.at(tracks[i].ID());
      if(calos.size()==0) continue;
      size_t nhits = 0;
      art::Ptr<anab::Calorimetry> calo = calos[0];
      size_t best_plane = 0;
      for( size_t i = calos.size(); i > 0; i--){
        if(calos[i-1]->dEdx().size() > nhits*1.5){
          nhits = calos[i-1]->dEdx().size();
          calo = calos[i-1];
          best_plane = i-1;
        }
      }

      // Get rid of any protons using chi2
      std::vector<const anab::ParticleID*> pids = fmpid.at(tracks[i].ID());
      bool is_proton = false;
      for(size_t i = 0; i < pids.size(); i++){
        // Only use the collection plane
        if(pids[i]->PlaneID().Plane != best_plane) continue;
        // If minimum chi2 is proton then ignore
        if(pids[i]->Chi2Proton() < pids[i]->Chi2Muon() && pids[i]->Chi2Proton() < pids[i]->Chi2Pion()){ 
          is_proton = true;
          continue;
        }
      }
      if(is_proton) continue;

      // Get rid of tracks which don't scatter like muons
      std::vector<float> angles = fMcsFitter.fitMcs(tracks[i], 13).scatterAngles();
      double ave_angle = std::accumulate(angles.begin(), angles.end(), 0)/angles.size();
      if(AverageDCA(tracks[i]) < 0.2 || ave_angle < 30) continue;

      // Get rid of any contained particles which don't stop (most muons do)
      double stop_chi2 = fStopTagger.StoppingChiSq(tracks[i].End(), calos);
      if(stop_chi2 < 1.2) continue;

      // Reject any tracks shorter than 25 cm
      if(tracks[i].Length() < 25) continue;

      // Check momentum reconstruction quality
      double range_mom = fRangeFitter.GetTrackMomentum(tracks[i].Length(), 13);
      double mcs_mom = fMcsFitter.fitMcs(tracks[i], 13).bestMomentum();
      double mom_diff = (mcs_mom - range_mom)/range_mom;
      double mom_diff_limit = 0.5 + std::exp(-(tracks[i].Length()-15.)/10.);
      if(mom_diff > mom_diff_limit) continue;

      long_tracks.push_back(tracks[i]);
    }

    std::sort(long_tracks.begin(), long_tracks.end(), [](auto& left, auto& right){
              return left.Length() > right.Length();});

    // Case 1: 1 escaping track
    if(n_escape == 1 && long_tracks.size() > 0){
      // If escaping track is longest and length > 50 cm then ID as muon
      if(longest_escape == long_tracks[0].Length() && longest_escape > 50){
        has_candidate = true;
        candidate = long_tracks[0];
      }
    }

    // Case 2: All tracks contained
    else if(n_escape == 0 && long_tracks.size() > 0){
      // Select the longest track
      has_candidate = true;
      candidate = long_tracks[0];
    }

    // Check vertex (start of muon candidate) in FV
    // Fiducial definition 50 cm from back, 10 cm from left, right and bottom, 15 cm from front, 20 cm from top 
    // 5 cm either side of CPA, 2.5 cm either side of APA gap
    bool in_fiducial = fTpcGeo.InFiducial(candidate.Start(), 10., 10., 15., 10., 20., 50., 5., 2.5);
    if(has_candidate) is_selected = true;

    return std::make_pair(std::make_pair(is_selected, in_fiducial), candidate);

  }

  //------------------------------------------------------------------------------------------------------------------------------------------
    
  void SelectionTree::FillSelectionTree(std::string selection, std::pair<std::pair<bool, bool>, recob::Track> sel_track, int trueId, std::map<int, simb::MCParticle> particles){

    selected[selection] = sel_track.first.first;
    in_fv[selection] = sel_track.first.second;

    // Calculate kinematic variables for prop_sel_track prop_track
    reco_cont[selection] = fTpcGeo.InFiducial(sel_track.second.End(), 1.5);
    reco_length[selection] = sel_track.second.Length();
    reco_mom[selection] = 0.;
    if(reco_cont[selection]){
      reco_mom[selection] = fRangeFitter.GetTrackMomentum(reco_length[selection], 13);
    }
    else{
      recob::MCSFitResult mcsResult = fMcsFitter.fitMcs(sel_track.second);
      reco_mom[selection] = mcsResult.bestMomentum();
    }
    reco_theta[selection] = sel_track.second.Theta();
    reco_phi[selection] = sel_track.second.Phi();
    reco_vtx_x[selection] = sel_track.second.Start().X();
    reco_vtx_y[selection] = sel_track.second.Start().Y();
    reco_vtx_z[selection] = sel_track.second.Start().Z();

    // Get the true kinematic variables
    if(particles.find(trueId) != particles.end()){
      TVector3 start(particles[trueId].Vx(), particles[trueId].Vy(), particles[trueId].Vz());
      TVector3 end(particles[trueId].EndX(), particles[trueId].EndY(), particles[trueId].EndZ());
      true_length[selection] = fTpcGeo.TpcLength(particles[trueId]);
      true_mom[selection] = particles[trueId].P();
      true_theta[selection] = (end-start).Theta();
      true_phi[selection] = (end-start).Phi();
      true_cont[selection] = fTpcGeo.IsContained(particles[trueId]);
      true_pdg[selection] = particles[trueId].PdgCode();
    }

  }

 double SelectionTree::AverageDCA(const recob::Track& track){

    TVector3 start = track.Vertex<TVector3>();
    TVector3 end = track.End<TVector3>();
    double denominator = (end - start).Mag();
    size_t npts = track.NumberTrajectoryPoints();
    double aveDCA = 0;
    int usedPts = 0;
    for(size_t i = 0; i < npts; i++){
      TVector3 point = track.LocationAtPoint<TVector3>(i);
      if(!track.HasValidPoint(i)) continue;
      aveDCA += (point - start).Cross(point - end).Mag()/denominator;
      usedPts++;
    }   
    return aveDCA/usedPts;
  } 

  double SelectionTree::HitEnergy(art::Ptr<recob::Hit> hit){

    double ADCtoEl = 0.02354; //FIXME from calorimetry_sbnd.fcl
    double time = hit->PeakTime();
    double timetick = fDetectorProperties->SamplingRate()*1e-3;
    double presamplings = fDetectorProperties->TriggerOffset();
    time -= presamplings;
    time = time*timetick;
    double tau = fDetectorProperties->ElectronLifetime();
    double correction = std::exp(time/tau);
    return fDetectorProperties->ModBoxCorrection((hit->Integral()/ADCtoEl)*correction)/1e3; //[GeV]
    
  }

  double SelectionTree::NeutrinoEnergy(std::map<int, double> track_energies, double shower_energy, int id){
    double nu_e = 0;
    if(shower_energy>0 && shower_energy<5) nu_e = shower_energy;
    for(auto const& kv : track_energies){
      if(kv.first != id && kv.second>0 && kv.second<5) nu_e += kv.second;
    }
    return nu_e;
  }

  DEFINE_ART_MODULE(SelectionTree)
} // namespace sbnd

