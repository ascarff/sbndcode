////////////////////////////////////////////////////////////////////////
// Class:       XSecTreeCosmic
// Module Type: analyzer
// File:        XSecTreeCosmic_module.cc
//
// Analysis module for selecting cross section distributions from truth
//
// Tom Brooks (tbrooks@fnal.gov)
////////////////////////////////////////////////////////////////////////

// sbndcode includes
#include "sbndcode/Geometry/GeometryWrappers/TPCGeoAlg.h"

// LArSoft includes
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"

// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Utilities/Exception.h"
#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
///#include "larcoreobj/SummaryData/POTSummary.h"
#include "larreco/RecoAlg/TrackMomentumCalculator.h"

// Utility libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Atom.h"

// ROOT includes. Note: To look up the properties of the ROOT classes,
// use the ROOT web site; e.g.,
// <https://root.cern.ch/doc/master/annotated.html>
#include "TTree.h"
#include "TVector3.h"
#include "TLorentzVector.h"
#include "TRandom2.h"

// C++ includes
#include <map>
#include <vector>
#include <string>

namespace sbnd {

  class XSecTreeCosmic : public art::EDAnalyzer {
  public:

    // Describes configuration parameters of the module
    struct Config {
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;
 
      // One Atom for each parameter
      fhicl::Atom<art::InputTag> GenModuleLabel {
        Name("GenModuleLabel"),
        Comment("tag of generator level data product")
      };

      fhicl::Atom<art::InputTag> G4ModuleLabel {
        Name("G4ModuleLabel"),
        Comment("tag of geant4 level data product")
      };

      fhicl::Atom<bool> Verbose {
        Name("Verbose"),
        Comment("Print information about what's going on")
      };

      fhicl::Atom<double> WallCut {
        Name("WallCut"),
        Comment("Fiducial cut from all walls but back [cm]")
      };

      fhicl::Atom<double> BackCut {
        Name("BackCut"),
        Comment("Fiducial cut from all back wall [cm]")
      };

      fhicl::Atom<double> MinContainedLength {
        Name("MinContainedLength"),
        Comment("Minimum length of longest particle if contained [cm]")
      };

      fhicl::Atom<double> MinExitingLength {
        Name("MinExitingLength"),
        Comment("Minimum length of longest particle if exiting [cm]")
      };

      fhicl::Atom<double> ElectronThreshold {
        Name("ElectronThreshold"),
        Comment("Momentum threshold for reconstructing electronss [GeV]")
      };

      fhicl::Atom<double> MuonThreshold {
        Name("MuonThreshold"),
        Comment("Momentum threshold for reconstructing muons [GeV]")
      };

      fhicl::Atom<double> Pi0Threshold {
        Name("Pi0Threshold"),
        Comment("Momentum threshold for reconstructing pi0s [GeV]")
      };

      fhicl::Atom<double> PhotonThreshold {
        Name("PhotonThreshold"),
        Comment("Momentum threshold for reconstructing photons [GeV]")
      };

      fhicl::Atom<double> PionThreshold {
        Name("PionThreshold"),
        Comment("Momentum threshold for reconstructing charged pions [GeV]")
      };

      fhicl::Atom<double> ProtonThreshold {
        Name("ProtonThreshold"),
        Comment("Momentum threshold for reconstructing protons [GeV]")
      };

      fhicl::Atom<double> ElectronEff {
        Name("ElectronEff"),
        Comment("Efficiency for reconstructing electrons [%]")
      };

      fhicl::Atom<double> MuonEff {
        Name("MuonEff"),
        Comment("Efficiency for reconstructing muons [%]")
      };

      fhicl::Atom<double> Pi0Eff {
        Name("Pi0Eff"),
        Comment("Efficiency for reconstructing pi0s [%]")
      };

      fhicl::Atom<double> PhotonEff {
        Name("PhotonEff"),
        Comment("Efficiency for reconstructing photons [%]")
      };

      fhicl::Atom<double> PionEff {
        Name("PionEff"),
        Comment("Efficiency for reconstructing pions [%]")
      };

      fhicl::Atom<double> ProtonEff {
        Name("ProtonEff"),
        Comment("Efficiency for reconstructing protons [%]")
      };
      
      fhicl::Atom<double> PionPidEff {
        Name("PionPidEff"),
        Comment("PID efficiency for pions [%]")
      };

      fhicl::Atom<double> ProtonPidEff {
        Name("ProtonPidEff"),
        Comment("PID efficiency for protons [%]")
      };

    }; // Config
 
    using Parameters = art::EDAnalyzer::Table<Config>;
 
    // Constructor: configures module
    explicit XSecTreeCosmic(Parameters const& config);
 
    // Called once, at start of the job
    virtual void beginJob() override;
 
    // Called for every sub run
    virtual void beginSubRun(const art::SubRun& subrun) override;

    // Called once per event
    virtual void analyze(const art::Event& event) override;

    // Called once, at end of the job
    virtual void endJob() override;

    // Reset all counters/variables to their null values
    void ResetVars();

    // Give us a list of the stable, primary particles that we're interested in
    std::vector<const simb::MCParticle*> InterestingParticles(std::vector<const simb::MCParticle*> particles);

    // Apply reconstruction efficiencies to true particles
    std::vector<const simb::MCParticle*> RecoParticles(std::vector<const simb::MCParticle*> particles);

    // Smear momentum for electrons from TRACS performance
    double SmearElectronMomentum(double momentum);

    // Smear momentum for exiting particles using MCS based method
    double SmearMcsMomentum(double momentum);

    // Smear momentum for contained particles using range based method
    double SmearRangeMomentum(double momentum);

    // Calculate the visible energy as neutrino energy estimator
    double VisibleEnergy(std::vector<const simb::MCParticle*> particles);

    // Calculate transverse variables (https://link.aps.org/accepted/10.1103/PhysRevC.94.015503)
    TVector3 DeltaPT(TVector3 mu_mom, TVector3 pr_mom);

    // Inclusive charged current selection
    bool IsCCInc(geo::Point_t vertex, std::vector<const simb::MCParticle*> reco_particles);

  private:

    // fcl file parameters
    art::InputTag fGenModuleLabel;      ///< name of gen producer
    art::InputTag fG4ModuleLabel;       ///< name of g4 producer
    bool          fVerbose;             ///< print information about what's going on

    double        fWallCut;
    double        fBackCut;

    double fMinContainedLength;
    double fMinExitingLength;

    double fElectronThreshold;
    double fMuonThreshold;
    double fPi0Threshold;
    double fPhotonThreshold;
    double fPionThreshold;
    double fProtonThreshold;

    double fElectronEff;
    double fMuonEff;
    double fPi0Eff;
    double fPhotonEff;
    double fPionEff;
    double fProtonEff;

    double fPionPidEff;
    double fProtonPidEff;

    TPCGeoAlg fTPCGeo;
    trkf::TrackMomentumCalculator fRangeFitter;

    TRandom2 *fRandom;

    // Global variables
    int lep_j;
    int longest_j;

    // Tree
    TTree *fXSecTreeCosmic;
    TTree *fMetaDataTree;

    // XSec tree true neutrino vertex
    double vtx_x;
    double vtx_y;
    double vtx_z;

    // XSec true tree parameters
    bool true_particles_contained;
    bool true_lep_contained;
    int true_cc;
    int true_nu_pdg;
    int true_int_type;
    unsigned int true_n_pipm;
    unsigned int true_n_pi0;
    unsigned int true_n_phot;
    unsigned int true_n_elec;
    unsigned int true_n_pr;
    double true_nu_energy;
    double true_lep_mom;
    double true_lep_theta;
    double true_pr1_mom;
    double true_pr1_theta;
    double true_lep_pr1_angle;
    double true_pipm1_mom;
    double true_pipm1_theta;
    double true_lep_pipm1_angle;
    double true_delta_pt;
    double true_delta_alphat;
    double true_delta_phit;


//added Electron and Photon variables to pushback
  std::vector<double> Electron_momentum;
  std::vector<std::string> Electron_process;
  std::vector<double> Electron_px;
  std::vector<double> Electron_py;
  std::vector<double> Electron_pz;
  std::vector<double> Electron_theta;
  std::vector<double> Electron_phi;
  std::vector<double> Electron_time;
  std::vector<int> Electron_PDG;
  std::vector<double> Electron_Vx;
  std::vector<double> Electron_Vy;
  std::vector<double> Electron_Vz;
  std::vector<double> Electron_KE;
  std::vector<double> Electron_trackid;
  std::vector<int> Electron_Mother;
  std::vector<double> pi0_KE;
  std::vector<double> Photon_KE;
  std::vector<int> Photon_PDG;

  std::vector<double> fpairInvM;
  std::vector<double> fpairE;
  std::vector<double> fpairAngle;
  std::vector<double> fsepAngle;
  std::vector<double> fpairAsym;
  std::vector<double> fKE_low; std::vector<double> fKE_high;

/*
//New variables added as doubles:
  double fpairInvM;
  double fpairE;
  double fpairAngle;
  double fsepAngle;
  double fpairAsym;
  float fKE_low;   float fKE_high;
*/

    // XSec smearing + efficiency tree parameters
    bool smeareff_particles_contained;
    bool smeareff_lep_contained;
    int smeareff_cc;
    int smeareff_nu_pdg;
    int smeareff_int_type;
    unsigned int smeareff_n_pipm;
    unsigned int smeareff_n_pi0;
    unsigned int smeareff_n_pr;
    double smeareff_nu_energy;
    double smeareff_lep_mom;
    double smeareff_lep_theta;
    double smeareff_pr1_mom;
    double smeareff_pr1_theta;
    double smeareff_lep_pr1_angle;
    double smeareff_pipm1_mom;
    double smeareff_pipm1_theta;
    double smeareff_lep_pipm1_angle;
    double smeareff_delta_pt;
    double smeareff_delta_alphat;
    double smeareff_delta_phit;

    // XSec reco tree parameters
    bool reco_particles_contained;
    bool reco_lep_contained;
    int reco_cc;
    int reco_nu_pdg;
    int reco_int_type;
    unsigned int reco_n_pipm;
    unsigned int reco_n_pi0;
    unsigned int reco_n_pr;
    double reco_nu_energy;
    double reco_lep_mom;
    double reco_lep_theta;
    double reco_pr1_mom;
    double reco_pr1_theta;
    double reco_lep_pr1_angle;
    double reco_pipm1_mom;
    double reco_pipm1_theta;
    double reco_lep_pipm1_angle;
    double reco_delta_pt;
    double reco_delta_alphat;
    double reco_delta_phit;

    // MetaData tree parameters
    double pot;

  }; // class XSecTreeCosmic


  // Constructor
  XSecTreeCosmic::XSecTreeCosmic(Parameters const& config)
    : EDAnalyzer(config)
    , fGenModuleLabel       (config().GenModuleLabel())
    , fG4ModuleLabel        (config().G4ModuleLabel())
    , fVerbose              (config().Verbose())
    , fWallCut              (config().WallCut())
    , fBackCut              (config().BackCut())
    , fMinContainedLength   (config().MinContainedLength())
    , fMinExitingLength     (config().MinExitingLength())
    , fMuonThreshold        (config().MuonThreshold())
    , fPi0Threshold         (config().Pi0Threshold())
    , fPhotonThreshold      (config().PhotonThreshold())
    , fPionThreshold        (config().PionThreshold())
    , fProtonThreshold      (config().ProtonThreshold())
    , fMuonEff              (config().MuonEff())
    , fPi0Eff               (config().Pi0Eff())
    , fPhotonEff            (config().PhotonEff())
    , fPionEff              (config().PionEff())
    , fProtonEff            (config().ProtonEff())
    , fPionPidEff           (config().PionPidEff())
    , fProtonPidEff         (config().ProtonPidEff())
  {

  }


  void XSecTreeCosmic::beginJob()
  {
    // Access tfileservice to handle creating and writing histograms
    art::ServiceHandle<art::TFileService> tfs;
    // Define histograms
    fXSecTreeCosmic = tfs->make<TTree>("interaction", "xsec tree");

    // True neutrino vertex
    fXSecTreeCosmic->Branch("vtx_x", &vtx_x, "vtx_x/D");
    fXSecTreeCosmic->Branch("vtx_y", &vtx_y, "vtx_y/D");
    fXSecTreeCosmic->Branch("vtx_z", &vtx_z, "vtx_z/D");

    // True selection and kinematic variables
    fXSecTreeCosmic->Branch("true_particles_contained", &true_particles_contained, "true_particles_contained/O");
    fXSecTreeCosmic->Branch("true_lep_contained", &true_lep_contained, "true_lep_contained/O");
    fXSecTreeCosmic->Branch("true_cc", &true_cc, "true_cc/I");
    fXSecTreeCosmic->Branch("true_nu_pdg", &true_nu_pdg, "true_nu_pdg/I");
    fXSecTreeCosmic->Branch("true_int_type", &true_int_type, "true_int_type/I");
    fXSecTreeCosmic->Branch("true_n_pipm", &true_n_pipm, "true_n_pipm/i");
    fXSecTreeCosmic->Branch("true_n_pi0", &true_n_pi0, "true_n_pi0/i");
    fXSecTreeCosmic->Branch("true_n_phot", &true_n_phot, "true_n_phot/i");
    fXSecTreeCosmic->Branch("true_n_elec", &true_n_elec, "true_n_elec/i");
    fXSecTreeCosmic->Branch("true_n_pr", &true_n_pr, "true_n_pr/i");
    fXSecTreeCosmic->Branch("true_nu_energy", &true_nu_energy, "true_nu_energy/D");
    fXSecTreeCosmic->Branch("true_lep_mom", &true_lep_mom, "true_lep_mom/D");
    fXSecTreeCosmic->Branch("true_lep_theta", &true_lep_theta, "true_lep_theta/D");
    fXSecTreeCosmic->Branch("true_pr1_mom", &true_pr1_mom, "true_pr1_mom/D");
    fXSecTreeCosmic->Branch("true_pr1_theta", &true_pr1_theta, "true_pr1_theta/D");
    fXSecTreeCosmic->Branch("true_lep_pr1_angle", &true_lep_pr1_angle, "true_lep_pr1_angle/D");
    fXSecTreeCosmic->Branch("true_pipm1_mom", &true_pipm1_mom, "true_pipm1_mom/D");
    fXSecTreeCosmic->Branch("true_pipm1_theta", &true_pipm1_theta, "true_pipm1_theta/D");
    fXSecTreeCosmic->Branch("true_lep_pipm1_angle", &true_lep_pipm1_angle, "true_lep_pipm1_angle/D");
    fXSecTreeCosmic->Branch("true_delta_pt", &true_delta_pt, "true_delta_pt/D");
    fXSecTreeCosmic->Branch("true_delta_alphat", &true_delta_alphat, "true_delta_alphat/D");
    fXSecTreeCosmic->Branch("true_delta_phit", &true_delta_phit, "true_delta_phit/D");


  // Truth Level Things for Electrons and Photons
  fXSecTreeCosmic->Branch("Electron_momentum","std::vector<double>",&Electron_momentum);
  fXSecTreeCosmic->Branch("Electron_xmomentum","std::vector<double>",&Electron_px);
  fXSecTreeCosmic->Branch("Electron_ymomentum","std::vector<double>",&Electron_py);
  fXSecTreeCosmic->Branch("Electron_zmomentum","std::vector<double>",&Electron_pz);
  fXSecTreeCosmic->Branch("Electron_theta","std::vector<double>",&Electron_theta);
  fXSecTreeCosmic->Branch("Electron_phi","std::vector<double>",&Electron_phi);
  fXSecTreeCosmic->Branch("Electron_Mother","std::vector<int>",&Electron_Mother);
  fXSecTreeCosmic->Branch("Electron_PDG","std::vector<int>",&Electron_PDG);
  fXSecTreeCosmic->Branch("Electron_time","std::vector<double>",&Electron_time);
  fXSecTreeCosmic->Branch("Electron_Vx","std::vector<double>",&Electron_Vx);
  fXSecTreeCosmic->Branch("Electron_Vy","std::vector<double>",&Electron_Vy);
  fXSecTreeCosmic->Branch("Electron_Vz","std::vector<double>",&Electron_Vz);
  fXSecTreeCosmic->Branch("Electron_process","std::vector<std::string>",&Electron_process);
  fXSecTreeCosmic->Branch("Electron_KE","std::vector<double>",&Electron_KE);
  fXSecTreeCosmic->Branch("pi0_KE","std::vector<double>",&pi0_KE);
  fXSecTreeCosmic->Branch("Photon_PDG","std::vector<int>",&Photon_PDG);
  fXSecTreeCosmic->Branch("Photon_KE","std::vector<double>",&Photon_KE);

/*
//Tree Branches copied over from WireHitsAnalysis_module.cc to use in plotscript.cc
	fXSecTreeCosmic->Branch("pairInvM",&fpairInvM,"pairInvM/d");
	fXSecTreeCosmic->Branch("pairE",&fpairE,"pairE/d");
	fXSecTreeCosmic->Branch("pairAngle",&fpairAngle,"pairAngle/d");
	fXSecTreeCosmic->Branch("sepAngle",&fsepAngle,"sepAngle/d");
	fXSecTreeCosmic->Branch("pairAsym",&fpairAsym,"pairAsym/d");
  fXSecTreeCosmic->Branch("KE_low",&fKE_low,"KE_low/f");
  fXSecTreeCosmic->Branch("KE_high",&fKE_high,"KE_high/f");
*/

  fXSecTreeCosmic->Branch("fpairInvM","std::vector<double>",&fpairInvM);
  fXSecTreeCosmic->Branch("fpairE","std::vector<double>",&fpairE);
  fXSecTreeCosmic->Branch("fpairAngle","std::vector<double>",&fpairAngle);
  fXSecTreeCosmic->Branch("fsepAngle","std::vector<double>",&fsepAngle);
  fXSecTreeCosmic->Branch("fpairAsym","std::vector<double>",&fpairAsym);
  fXSecTreeCosmic->Branch("fKE_low","std::vector<double>",&fKE_low);
  fXSecTreeCosmic->Branch("fKE_high","std::vector<double>",&fKE_high);

    // Smeared and efficiency applied kinematic variables
    fXSecTreeCosmic->Branch("smeareff_particles_contained", &smeareff_particles_contained, "smeareff_particles_contained/O");
    fXSecTreeCosmic->Branch("smeareff_lep_contained", &smeareff_lep_contained, "smeareff_lep_contained/O");
    fXSecTreeCosmic->Branch("smeareff_cc", &smeareff_cc, "smeareff_cc/I");
    fXSecTreeCosmic->Branch("smeareff_nu_pdg", &smeareff_nu_pdg, "smeareff_nu_pdg/I");
    fXSecTreeCosmic->Branch("smeareff_int_type", &smeareff_int_type, "smeareff_int_type/I");
    fXSecTreeCosmic->Branch("smeareff_n_pipm", &smeareff_n_pipm, "smeareff_n_pipm/i");
    fXSecTreeCosmic->Branch("smeareff_n_pi0", &smeareff_n_pi0, "smeareff_n_pi0/i");
    fXSecTreeCosmic->Branch("smeareff_n_pr", &smeareff_n_pr, "smeareff_n_pr/i");
    fXSecTreeCosmic->Branch("smeareff_nu_energy", &smeareff_nu_energy, "smeareff_nu_energy/D");
    fXSecTreeCosmic->Branch("smeareff_lep_mom", &smeareff_lep_mom, "smeareff_lep_mom/D");
    fXSecTreeCosmic->Branch("smeareff_lep_theta", &smeareff_lep_theta, "smeareff_lep_theta/D");
    fXSecTreeCosmic->Branch("smeareff_pr1_mom", &smeareff_pr1_mom, "smeareff_pr1_mom/D");
    fXSecTreeCosmic->Branch("smeareff_pr1_theta", &smeareff_pr1_theta, "smeareff_pr1_theta/D");
    fXSecTreeCosmic->Branch("smeareff_lep_pr1_angle", &smeareff_lep_pr1_angle, "smeareff_lep_pr1_angle/D");
    fXSecTreeCosmic->Branch("smeareff_pipm1_mom", &smeareff_pipm1_mom, "smeareff_pipm1_mom/D");
    fXSecTreeCosmic->Branch("smeareff_pipm1_theta", &smeareff_pipm1_theta, "smeareff_pipm1_theta/D");
    fXSecTreeCosmic->Branch("smeareff_lep_pipm1_angle", &smeareff_lep_pipm1_angle, "smeareff_lep_pipm1_angle/D");
    fXSecTreeCosmic->Branch("smeareff_delta_pt", &smeareff_delta_pt, "smeareff_delta_pt/D");
    fXSecTreeCosmic->Branch("smeareff_delta_alphat", &smeareff_delta_alphat, "smeareff_delta_alphat/D");
    fXSecTreeCosmic->Branch("smeareff_delta_phit", &smeareff_delta_phit, "smeareff_delta_phit/D");

    // Reco selection and kinematic variables
    fXSecTreeCosmic->Branch("reco_particles_contained", &reco_particles_contained, "reco_particles_contained/O");
    fXSecTreeCosmic->Branch("reco_lep_contained", &reco_lep_contained, "reco_lep_contained/O");
    fXSecTreeCosmic->Branch("reco_cc", &reco_cc, "reco_cc/I");
    fXSecTreeCosmic->Branch("reco_nu_pdg", &reco_nu_pdg, "reco_nu_pdg/I");
    fXSecTreeCosmic->Branch("reco_int_type", &reco_int_type, "reco_int_type/I");
    fXSecTreeCosmic->Branch("reco_n_pipm", &reco_n_pipm, "reco_n_pipm/i");
    fXSecTreeCosmic->Branch("reco_n_pi0", &reco_n_pi0, "reco_n_pi0/i");
    fXSecTreeCosmic->Branch("reco_n_pr", &reco_n_pr, "reco_n_pr/i");
    fXSecTreeCosmic->Branch("reco_nu_energy", &reco_nu_energy, "reco_nu_energy/D");
    fXSecTreeCosmic->Branch("reco_lep_mom", &reco_lep_mom, "reco_lep_mom/D");
    fXSecTreeCosmic->Branch("reco_lep_theta", &reco_lep_theta, "reco_lep_theta/D");
    fXSecTreeCosmic->Branch("reco_pr1_mom", &reco_pr1_mom, "reco_pr1_mom/D");
    fXSecTreeCosmic->Branch("reco_pr1_theta", &reco_pr1_theta, "reco_pr1_theta/D");
    fXSecTreeCosmic->Branch("reco_lep_pr1_angle", &reco_lep_pr1_angle, "reco_lep_pr1_angle/D");
    fXSecTreeCosmic->Branch("reco_pipm1_mom", &reco_pipm1_mom, "reco_pipm1_mom/D");
    fXSecTreeCosmic->Branch("reco_pipm1_theta", &reco_pipm1_theta, "reco_pipm1_theta/D");
    fXSecTreeCosmic->Branch("reco_lep_pipm1_angle", &reco_lep_pipm1_angle, "reco_lep_pipm1_angle/D");
    fXSecTreeCosmic->Branch("reco_delta_pt", &reco_delta_pt, "reco_delta_pt/D");
    fXSecTreeCosmic->Branch("reco_delta_alphat", &reco_delta_alphat, "reco_delta_alphat/D");
    fXSecTreeCosmic->Branch("reco_delta_phit", &reco_delta_phit, "reco_delta_phit/D");

    fMetaDataTree = tfs->make<TTree>("metadata", "xsec tree");
    fMetaDataTree->Branch("pot", &pot, "pot/D");

    // Initial output
    std::cout<<"----------------- XSec Tree Module -------------------"<<std::endl;

    fRandom = new TRandom2();

  } // XSecTreeCosmic::beginJob()


  // Called for every sub run
  void XSecTreeCosmic::beginSubRun(const art::SubRun& subrun){

//    art::Handle< sumdata::POTSummary > potHandle;
 //   subrun.getByLabel(fGenModuleLabel, potHandle);
 //   const sumdata::POTSummary& potSum = (*potHandle);
 //   pot = potSum.totpot;
     pot=0;

    fMetaDataTree->Fill();
    return;
  } // XSecTreeCosmic::beginSubRun()


  void XSecTreeCosmic::analyze(const art::Event& event)
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
    // Retrieve all the truth info in the events
    art::ServiceHandle<cheat::ParticleInventoryService> pi_serv;
    art::Handle<std::vector<simb::MCTruth>> genHandle;
    std::vector<art::Ptr<simb::MCTruth>> mctruthList;
    if(event.getByLabel(fGenModuleLabel, genHandle)) art::fill_ptr_vector(mctruthList, genHandle);

    //----------------------------------------------------------------------------------------------------------
    //                                           FILLING THE TREE
    //----------------------------------------------------------------------------------------------------------
    // Loop over all the neutrino interactions
    for (size_t i = 0; i < mctruthList.size(); i++){
      if(fVerbose) std::cout<<"\n\nNeutrino: "<<i<<"\n";
      // Reset all the tree variables
      ResetVars();

      // Get the pointer to the MCTruth object
      art::Ptr<simb::MCTruth> mctruth = mctruthList.at(i);

      // Check the interaction is within the TPC
     // geo::Point_t vertex {mctruth->GetNeutrino().Nu().Vx(), 
     //                      mctruth->GetNeutrino().Nu().Vy(), 
     //                      mctruth->GetNeutrino().Nu().Vz()};
         geo::Point_t vertex {120,0,200};
     //
      if(fVerbose) std::cout<<"->Vertex: ("<<vertex.X()<<", "<<vertex.Y()<<", "<<vertex.Z()<<")\n";
      if(!fTPCGeo.InFiducial(vertex, 0.)) continue;

      vtx_x = vertex.X();
      vtx_y = vertex.Y();
      vtx_z = vertex.Z();

      //---------------------- FILLING ALL THE TRUE INTERACTION PARAMETERS ------------------------------------

      // Fill all the neutrino parameters
     // if(mctruth->GetNeutrino().CCNC() == simb::kCC) true_cc = 1;
     // else
      true_cc = 0;
      true_nu_pdg = 0 ; // mctruth->GetNeutrino().Nu().PdgCode();
      true_int_type = 0; // mctruth->GetNeutrino().Mode();
      true_nu_energy = 0; // mctruth->GetNeutrino().Nu().E();

      // Get all the particles propagated by geant 4
      std::vector<const simb::MCParticle*> parts = pi_serv->MCTruthToParticles_Ps(mctruth);
      // Get only the interesting ones
      std::vector<const simb::MCParticle*> particles = InterestingParticles(parts);

      if(fVerbose) std::cout<<particles.size()<<" interesting particles\n";

      // Lepton stuff
      for(size_t j = 0; j < particles.size(); j++){
        int pdg = std::abs(particles.at(j)->PdgCode());
        // Only consider the primary muon
        if(!(pdg == 13 || pdg == 11)) continue;
        lep_j = j;
        true_lep_mom = particles.at(j)->P();
        TVector3 start = particles.at(j)->Position().Vect();
        TVector3 end = particles.at(j)->EndPosition().Vect();
        true_lep_theta = (end - start).Theta();
        true_lep_contained = fTPCGeo.IsContained(*particles.at(j));
      }

      // Secondary particle stuff
      for(size_t j = 0; j < particles.size(); j++){
        if((int)j == lep_j) continue;

        // Only consider pi0, charged pi and protons
        int pdg = std::abs(particles.at(j)->PdgCode());

        TVector3 start = particles.at(j)->Position().Vect();
        TVector3 end = particles.at(j)->EndPosition().Vect();

//Initialise variables to 0, then will only fill it with a non 0 value when the correct conditions are met
//  fpairInvM = 0; fpairE = 0; fpairAngle = 0; fsepAngle = 0 ; fpairAsym = 0;

        // Count particle numbers
        if(pdg == 111){ 
          true_n_pi0++;
          if(!fTPCGeo.IsContained(*particles.at(j))) true_particles_contained = false;
	  int trackidpi0 = particles.at(j)->TrackId();

	  std::cout<<"///////////////// Here is a lovely lil pi0 with Event ID "<<event.id().event()<<std::endl;

	  std::cout << "*************** This lovely pi0 has " << particles.at(j)->NumberDaughters() << " daughters." <<std::endl;

	  pi0_KE.push_back(particles.at(j)->E()-0.135);  //also need to store pi0 energy

	 	 for(size_t k = 0; k < particles.size(); k++){

	  	TVector3 start = particles.at(k)->Position().Vect();
		TVector3 end = particles.at(k)->EndPosition().Vect();

		std::cout << "&&&&&&&&&&&&&&&&&&& The track ID of the pi0 is: " << trackidpi0 << std::endl;

		std::cout << "^^^^^^^^^^^^^^^^^^^ The PDG Code of the particles in the event is: " << particles.at(k)->PdgCode() << ", with Track ID = " << particles.at(k)->TrackId() << std::endl;

//          for (auto particles.at(j) : particles.at(j)){ 
		if (particles.at(k)->Mother()==trackidpi0 && particles.at(k)->PdgCode()==22){ //look for photons
		
		true_n_phot++;

 		std::cout<<"Run = "<<event.run()<<", SubRun = "<<event.subRun()<<", Event = "<<event.id().event()<<std::endl;

        	int trackidphot = particles.at(k)->TrackId();
 		std::cout<<"Track ID of photon is:" << trackidphot << std::endl;

		Photon_PDG.push_back(particles.at(k)->PdgCode());
		Photon_KE.push_back(particles.at(k)->E());

			for(size_t l = 0; l < particles.size(); l++){

       			TVector3 start = particles.at(l)->Position().Vect();
        		TVector3 end = particles.at(l)->EndPosition().Vect();

//			for (auto particles.at(k) : particles.at(k)){
//			   if ((particles.at(l)->Mother()==trackidphot) && (particles.at(l)->PdgCode()==11 || particles.at(l)->PdgCode()==-11)){

			if ((particles.at(l)->Mother()==trackidphot) && (particles.at(l)->PdgCode()==11)){ //
	
			true_n_elec++;

			Electron_process.push_back(particles.at(l)->Process());
			Electron_momentum.push_back(particles.at(l)->P());
			Electron_theta.push_back(particles.at(l)->Momentum().Theta());
			Electron_phi.push_back(particles.at(l)->Momentum().Phi());
			Electron_time.push_back(particles.at(l)->T());
			Electron_Mother.push_back(particles.at(l)->Mother());
			Electron_PDG.push_back(particles.at(l)->PdgCode());
			Electron_px.push_back(particles.at(l)->Px());
			Electron_py.push_back(particles.at(l)->Py());
			Electron_pz.push_back(particles.at(l)->Pz());
			Electron_Vx.push_back(particles.at(l)->Vx()); 
			Electron_Vy.push_back(particles.at(l)->Vy()); 
			Electron_Vz.push_back(particles.at(l)->Vz()); 
			Electron_KE.push_back(particles.at(l)->E()-0.000511);
			int Electron_trackID = particles.at(l)->TrackId();


				for(size_t m = 0; m < particles.size(); m++){ 

					if (((particles.at(m)->Mother()==trackidphot) && (particles.at(m)->PdgCode()==-11) && (particles.at(m)->TrackId()!=Electron_trackID))){

					double dotProd = particles.at(l)->Px()*particles.at(m)->Px() + particles.at(l)->Py()*particles.at(m)->Py() + particles.at(l)->Pz()*particles.at(m)->Pz();

					std::vector<double> Ptot = {(particles.at(l)->Px() + particles.at(m)->Px()), (particles.at(l)->Py() + particles.at(m)->Py()), (particles.at(l)->Pz() + particles.at(m)->Pz())};

					fsepAngle.push_back(acos(dotProd/(particles.at(l)->P()*particles.at(m)->P())));

					fpairE.push_back((particles.at(l)->E()-0.000511) + (particles.at(m)->E()-0.000511));

        				double pairE_temp = ((particles.at(l)->E()-0.000511) + (particles.at(m)->E()-0.000511));
        				double diff = ((particles.at(l)->E()-0.000511) - (particles.at(m)->E()-0.000511));

					fpairAngle.push_back(atan(pow(pow(Ptot[0],2) + pow(Ptot[1],2),0.5)/Ptot[2]));

					fpairInvM.push_back(pow(pow(pairE_temp + 0.001022,2) - pow(Ptot[0],2)-pow(Ptot[1],2)-pow(Ptot[2],2),0.5));

        if ((particles.at(l)->E()-0.000511)>(particles.at(m)->E()-0.000511)) {
		fKE_low.push_back((particles.at(m)->E()-0.000511));
		fKE_high.push_back((particles.at(l)->E()-0.000511));
		} //else determines which electron is more energetic
        else {fKE_low.push_back((particles.at(l)->E()-0.000511));
		fKE_high.push_back((particles.at(m)->E()-0.000511));}

		fpairAsym.push_back(abs(diff)/pairE_temp);

							}  //end of loop for positrons

						}  //end of loop through partcles within event again*2

					}  //end of loop looking for electrons

				} // end of looping through particles within event again*1
//			}
        		} //end of loop looking for photons

/*
	if (Electron_process.size()==2) {

	// calculate invariant mass by accessing the variables using Electron_px[0] and Electron_py[1] 
	
        double dotProd = Electron_px[0]*Electron_px[1] + Electron_py[0]*Electron_py[1] + Electron_pz[0]*Electron_pz[1];

        std::vector<double> Ptot = {(Electron_px[0] + Electron_px[1]), (Electron_py[0] + Electron_py[1]), (Electron_pz[0] + Electron_pz[1])}; 

	fsepAngle = acos(dotProd/(Electron_momentum[0]*Electron_momentum[1]));
        fpairE = (Electron_KE[0]) + (Electron_KE[1]);

//        double pairE_temp = (Electron_KE[0]-0.000511) + (Electron_KE[1]-0.000511);
 
	fpairAngle = atan(pow(pow(Ptot[0],2) + pow(Ptot[1],2),0.5)/Ptot[2]);
	fpairInvM = pow(pow(fpairE + 0.001022,2) - pow(Ptot[0],2)-pow(Ptot[1],2)-pow(Ptot[2],2),0.5);


        if ((Electron_KE[0])>(Electron_KE[1])) {
		(fKE_low = Electron_KE[1]);
		(fKE_high = Electron_KE[0]);
		} //else determines which electron is more energetic
        else {(fKE_low = Electron_KE[0]); (fKE_high = Electron_KE[1]);}

		fpairAsym = abs(Electron_KE[1] - Electron_KE[0])/(Electron_KE[1] + Electron_KE[0]);

			} //end if Electron_process.size()==2
*/

		} // end of looking for particles with pdg==22 and mother id = trackidpi0
//	}
	} // end of loop through particles within event
//} //end of loop for pi0


        if(pdg == 211){ 
          true_n_pipm++;
          if(!fTPCGeo.IsContained(*particles.at(j))) true_particles_contained = false;

          if(particles.at(j)->P() < true_pipm1_mom) continue;
          true_pipm1_mom = particles.at(j)->P();
          true_pipm1_theta = (end - start).Theta();

          // Calculate things relative to the lepton if there is one
          if(lep_j == -1) continue;
          TVector3 lep_end = particles.at(lep_j)->EndPosition().Vect();
          true_lep_pipm1_angle = lep_end.Angle(end);
        }
        if(pdg == 2212){ 
          true_n_pr++;
          if(!fTPCGeo.IsContained(*particles.at(j))) true_particles_contained = false;

          if(particles.at(j)->P() < true_pr1_mom) continue;
          true_pr1_mom = particles.at(j)->P();
          true_pr1_theta = (end - start).Theta();

          // Calculate things relative to the lepton if there is one
          if(lep_j == -1) continue;
          TVector3 lep_end = particles.at(lep_j)->EndPosition().Vect();
          true_lep_pr1_angle = lep_end.Angle(end);

          // Transverse variables
          TVector3 delta_pt = DeltaPT(particles.at(lep_j)->Momentum().Vect(), particles.at(j)->Momentum().Vect());
          true_delta_pt = delta_pt.Mag();
          true_delta_alphat = delta_pt.Theta()*TMath::RadToDeg();
          true_delta_phit = delta_pt.Phi()*TMath::RadToDeg();
        }

      }

      //----------------------------- EFFICIENCY + SMEARING --------------------------------------
      // Apply reconstruction efficiencies to the true particles
      std::vector<const simb::MCParticle*> reco_particles = RecoParticles(parts);
      if(fVerbose) std::cout<<"Number of reconstructed particles = "<<reco_particles.size()<<"\n";
      
      // Smearing won't change these
      smeareff_cc                  = true_cc;
      smeareff_nu_pdg              = true_nu_pdg;
      smeareff_int_type            = true_int_type;

      // TODO Make a better neutrino energy calculator
      smeareff_nu_energy = VisibleEnergy(reco_particles);
      if(fVerbose) std::cout<<"Smeared energy = "<<smeareff_nu_energy<<"\n";

      // Reset the lepton index
      lep_j = -1;
      // Get the lepton kinematics if CC
      for(size_t j = 0; j < reco_particles.size(); j++){
        int pdg = std::abs(reco_particles.at(j)->PdgCode());

        // Get the lepton kinematics - electrons
        if(pdg == 11 || pdg == 13){
          std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(j));
          TVector3 start = cross_points.first;
          TVector3 end = cross_points.second;
          double contained_length = fTPCGeo.TpcLength(*reco_particles.at(j));

          lep_j = j;
          smeareff_lep_theta = (end - start).Theta();
          smeareff_lep_contained = fTPCGeo.IsContained(*reco_particles.at(j));
          if(pdg == 11) smeareff_lep_mom = SmearElectronMomentum(reco_particles.at(j)->P());
          else if(smeareff_lep_contained) smeareff_lep_mom = fRangeFitter.GetTrackMomentum(contained_length, 13);
          else smeareff_lep_mom = SmearMcsMomentum(reco_particles.at(j)->P());
        }

      }
      if(fVerbose) std::cout<<"Smeared the lepton kinematics\n";

      // Loop over all the reconstructed particles and smear kinematic variables
      for(size_t j = 0; j < reco_particles.size(); j++){
        if ((int)j == lep_j) continue;

        int pdg = std::abs(reco_particles.at(j)->PdgCode());
        std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(j));
        TVector3 start = cross_points.first;
        TVector3 end = cross_points.second;
        double contained_length = fTPCGeo.TpcLength(*reco_particles.at(j));

        // Count particle numbers and smear - pi0
        if(pdg == 111){
          smeareff_n_pi0++;
          if(!fTPCGeo.IsContained(*reco_particles.at(j))) smeareff_particles_contained = false;
        }

        // Count particle numbers and smear - pions
        if(pdg == 211){ 
          smeareff_n_pipm++;
          if(!fTPCGeo.IsContained(*reco_particles.at(j))) smeareff_particles_contained = false;
          double pipm1_mom = fRangeFitter.GetTrackMomentum(contained_length, 13);

          if(pipm1_mom < smeareff_pipm1_mom) continue;
          smeareff_pipm1_mom = pipm1_mom;
          smeareff_pipm1_theta = (end - start).Theta();

          // Calculate things relative to the lepton if there is one
          if(lep_j == -1) continue;
          TVector3 lep_end = fTPCGeo.CrossingPoints(*reco_particles.at(lep_j)).second;
          smeareff_lep_pipm1_angle = lep_end.Angle(end);
        }

        // Count particle numbers and smear - protons
        if(pdg == 2212){ 
          smeareff_n_pr++;
          if(!fTPCGeo.IsContained(*reco_particles.at(j))) smeareff_particles_contained = false;
          double pr1_mom = fRangeFitter.GetTrackMomentum(contained_length, 2212);

          if(pr1_mom < smeareff_pr1_mom) continue;
          smeareff_pr1_mom = pr1_mom;
          smeareff_pr1_theta = (end - start).Theta();

          // Calculate things relative to the lepton if there is one
          if(lep_j == -1) continue;
          TVector3 lep_end = fTPCGeo.CrossingPoints(*reco_particles.at(lep_j)).second;
          smeareff_lep_pr1_angle = lep_end.Angle(end);

          // Transverse variables
          TVector3 scaled_lep_mom = reco_particles.at(lep_j)->Momentum().Vect()*(smeareff_lep_mom/reco_particles.at(lep_j)->P());
          TVector3 scaled_pr_mom = reco_particles.at(j)->Momentum().Vect()*(pr1_mom/reco_particles.at(j)->P());
          TVector3 delta_pt = DeltaPT(scaled_lep_mom, scaled_pr_mom);
          smeareff_delta_pt = delta_pt.Mag();
          smeareff_delta_alphat = delta_pt.Theta()*TMath::RadToDeg();
          smeareff_delta_phit = delta_pt.Phi()*TMath::RadToDeg();
        }

      }

      //---------------------------- CC INCLUSIVE SELECTION --------------------------------------

      // Same as smeared neutrino energy
      reco_nu_energy = smeareff_nu_energy;

      bool cc_selected = IsCCInc(vertex, reco_particles); 

      if(cc_selected){ 
        if(fVerbose) std::cout<<"Selected as CC\n";
        reco_cc = 1;
        reco_nu_pdg = 14;
      }
      else{ 
        reco_cc = -1;
      }

      //------------------------------------ RECO FSI ------------------------------------------

      for(size_t j = 0; j < reco_particles.size(); j++){
        // Don't look at the particle selected as the muon
        if(longest_j == (int)j) continue;

        int pdg = std::abs(reco_particles.at(j)->PdgCode());

        double contained_length = fTPCGeo.TpcLength(*reco_particles.at(j));
        std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(j));
        TVector3 start = cross_points.first;
        TVector3 end = cross_points.second;

        // Apply PID estimation and count reco particles
        if(pdg == 111){ 
          reco_n_pi0++;
          if(!fTPCGeo.IsContained(*reco_particles.at(j))) reco_particles_contained = false;
        }

        // Treat muons and pions as the same
        if(pdg == 13 || pdg == 211){
          // Check that particle is contained
          if(!fTPCGeo.IsContained(*reco_particles.at(j))) reco_particles_contained = false;

          double rand_pid = fRandom->Rndm();
          // ID as pion, calculate leading pion variables
          if(rand_pid < fPionPidEff){ 
            reco_n_pipm++;

            // Leading pion momentum and angle
            double pipm1_mom = fRangeFitter.GetTrackMomentum(contained_length, 13);
            if(pipm1_mom < reco_pipm1_mom) continue;
            // FIXME no idea how to estimate pion momentum
            reco_pipm1_mom = pipm1_mom;
            reco_pipm1_theta = (end - start).Theta();

            // Angle between leading pion and lepton candidate
            if(longest_j == -1) continue;
            TVector3 lep_end = fTPCGeo.CrossingPoints(*reco_particles.at(longest_j)).second;
            reco_lep_pipm1_angle = lep_end.Angle(end);
          }

          // ID as proton, calculate leading proton variables
          else{
            reco_n_pr++;

            // Leading proton momentum and angle
            double pr1_mom = fRangeFitter.GetTrackMomentum(contained_length, 2212);
            if(pr1_mom < reco_pr1_mom) continue;
            reco_pr1_mom = pr1_mom;
            reco_pr1_theta = (end - start).Theta();

            // Angle between leading proton and lepton candidate
            if(longest_j == -1) continue;
            TVector3 lep_end = fTPCGeo.CrossingPoints(*reco_particles.at(longest_j)).second;
            reco_lep_pr1_angle = lep_end.Angle(end);

            // Transverse variables
            TVector3 scaled_lep_mom = reco_particles.at(longest_j)->Momentum().Vect()*(reco_lep_mom/reco_particles.at(longest_j)->P());
            TVector3 scaled_pr_mom = reco_particles.at(j)->Momentum().Vect()*(pr1_mom/reco_particles.at(j)->P());
            TVector3 delta_pt = DeltaPT(scaled_lep_mom, scaled_pr_mom);
            reco_delta_pt = delta_pt.Mag();
            reco_delta_alphat = delta_pt.Theta()*TMath::RadToDeg();
            reco_delta_phit = delta_pt.Phi()*TMath::RadToDeg();
          }
        }

        if(pdg == 2212){
          // Check that particle is contained
          if(!fTPCGeo.IsContained(*reco_particles.at(j))) reco_particles_contained = false;

          double rand_pid = fRandom->Rndm();
          // ID as proton, calculate leading proton variables
          if(rand_pid < fProtonPidEff){ 
            reco_n_pr++;

            // Leading proton momentum and angle
            double pr1_mom = fRangeFitter.GetTrackMomentum(contained_length, 2212);
            if(pr1_mom < reco_pr1_mom) continue;
            reco_pr1_mom = pr1_mom;
            reco_pr1_theta = (end - start).Theta();

            // Angle between leading proton and lepton candidate
            if(longest_j == -1) continue;
            TVector3 lep_end = fTPCGeo.CrossingPoints(*reco_particles.at(longest_j)).second;
            reco_lep_pr1_angle = lep_end.Angle(end);

            // Transverse variables
            TVector3 scaled_lep_mom = reco_particles.at(longest_j)->Momentum().Vect()*(reco_lep_mom/reco_particles.at(longest_j)->P());
            TVector3 scaled_pr_mom = reco_particles.at(j)->Momentum().Vect()*(pr1_mom/reco_particles.at(j)->P());
            TVector3 delta_pt = DeltaPT(scaled_lep_mom, scaled_pr_mom);
            reco_delta_pt = delta_pt.Mag();
            reco_delta_alphat = delta_pt.Theta()*TMath::RadToDeg();
            reco_delta_phit = delta_pt.Phi()*TMath::RadToDeg();
          }

          // ID as pion, calculate leading pion variables
          else{ 
            reco_n_pipm++;

            // Leading pion momentum and angle
            double pipm1_mom = fRangeFitter.GetTrackMomentum(contained_length, 13);
            if(pipm1_mom < reco_pipm1_mom) continue;
            reco_pipm1_mom = pipm1_mom;
            reco_pipm1_theta = (end - start).Theta();

            // Angle between leading pion and lepton candidate
            if(longest_j == -1) continue;
            TVector3 lep_end = fTPCGeo.CrossingPoints(*reco_particles.at(longest_j)).second;
            reco_lep_pipm1_angle = lep_end.Angle(end);
          }
        }
          
      }

      
      if(fVerbose) std::cout<<"->true_cc:                  "<<true_cc<<"\n"
                            <<"->true_nu_pdg:              "<<true_nu_pdg<<"\n"
                            <<"->true_nu_energy:           "<<true_nu_energy<<"\n"
                            <<"->true_int_type:            "<<true_int_type<<"\n"
                            <<"->true_lep_contained:       "<<true_lep_contained<<"\n"
                            <<"-->true_lep_mom:            "<<true_lep_mom<<"\n"
                            <<"-->true_lep_theta:          "<<true_lep_theta<<"\n"
                            <<"->true_particles_contained: "<<true_particles_contained<<"\n"
                            <<"->true_n_pi0:               "<<true_n_pi0<<"\n"
			    <<"->true_n_phot:    	   "<<true_n_phot<<"\n"
                            <<"->true_n_pipm:              "<<true_n_pipm<<"\n"
                            <<"-->true_pipm1_mom:          "<<true_pipm1_mom<<"\n"
                            <<"-->true_pipm1_theta:        "<<true_pipm1_theta<<"\n"
                            <<"-->true_lep_pipm1_angle:    "<<true_lep_pipm1_angle<<"\n"
                            <<"->true_n_pr:                "<<true_n_pr<<"\n"
                            <<"-->true_pr1_mom:            "<<true_pr1_mom<<"\n"
                            <<"-->true_pr1_theta:          "<<true_pr1_theta<<"\n"
                            <<"-->true_lep_pr1_angle:      "<<true_lep_pr1_angle<<"\n"
                            <<"-->true_delta_pt:           "<<true_delta_pt<<"\n"
                            <<"-->true_delta_alphat:       "<<true_delta_alphat<<"\n"
                            <<"-->true_delta_phit:         "<<true_delta_phit<<"\n"
                            <<"\n"
                            <<"->reco_cc:                  "<<reco_cc<<"\n"
                            <<"->reco_nu_pdg:              "<<reco_nu_pdg<<"\n"
                            <<"->reco_nu_energy:           "<<reco_nu_energy<<"\n"
                            <<"->reco_int_type:            "<<reco_int_type<<"\n"
                            <<"->reco_lep_contained:       "<<reco_lep_contained<<"\n"
                            <<"-->reco_lep_mom:            "<<reco_lep_mom<<"\n"
                            <<"-->reco_lep_theta:          "<<reco_lep_theta<<"\n"
                            <<"->reco_particles_contained: "<<reco_particles_contained<<"\n"
                            <<"->reco_n_pi0:               "<<reco_n_pi0<<"\n"
                            <<"->reco_n_pipm:              "<<reco_n_pipm<<"\n"
                            <<"-->reco_pipm1_mom:          "<<reco_pipm1_mom<<"\n"
                            <<"-->reco_pipm1_theta:        "<<reco_pipm1_theta<<"\n"
                            <<"-->reco_lep_pipm1_angle:    "<<reco_lep_pipm1_angle<<"\n"
                            <<"->reco_n_pr:                "<<reco_n_pr<<"\n"
                            <<"-->reco_pr1_mom:            "<<reco_pr1_mom<<"\n"
                            <<"-->reco_pr1_theta:          "<<reco_pr1_theta<<"\n"
                            <<"-->reco_lep_pr1_angle:      "<<reco_lep_pr1_angle<<"\n"
                            <<"-->reco_delta_pt:           "<<reco_delta_pt<<"\n"
                            <<"-->reco_delta_alphat:       "<<reco_delta_alphat<<"\n"
                            <<"-->reco_delta_phit:         "<<reco_delta_phit<<"\n";


      fXSecTreeCosmic->Fill();

 std::cout << "Variables are being filled here." << std::endl; 

    // clear vectors
    Electron_process.clear(); Electron_momentum.clear();
    Electron_px.clear(); Electron_py.clear(); Electron_pz.clear();
    Electron_theta.clear(); Electron_phi.clear();
    Electron_time.clear(); Electron_Mother.clear();Electron_PDG.clear();
    Electron_Vx.clear(); Electron_Vy.clear(); Electron_Vz.clear();
    Electron_KE.clear(); Photon_PDG.clear(); Photon_KE.clear(); pi0_KE.clear();

	fpairInvM.clear(); fpairE.clear(); fpairAngle.clear(); fsepAngle.clear(); fpairAsym.clear(); fKE_low.clear(); fKE_high.clear();
    }

  } // XSecTreeCosmic::analyze()


  void XSecTreeCosmic::endJob(){

  } // XSecTreeCosmic::endJob()


  // Reset variables and counters
  void XSecTreeCosmic::ResetVars(){

    lep_j = -1;
    longest_j = -1;

    vtx_x = -99999;
    vtx_y = -99999;
    vtx_z = -99999;
    
    true_particles_contained = true;
    true_lep_contained       = false;
    true_cc                  = -1;
    true_nu_pdg              = -99999;
    true_int_type            = -99999;
    true_n_pipm              = 0;
    true_n_pi0               = 0;
    true_n_phot		     = 0;
    true_n_pr                = 0;
    true_nu_energy           = -99999;
    true_lep_mom             = -99999;
    true_lep_theta           = -99999;
    true_pr1_mom             = -99999;
    true_pr1_theta           = -99999;
    true_lep_pr1_angle       = -99999;
    true_pipm1_mom           = -99999;
    true_pipm1_theta         = -99999;
    true_lep_pipm1_angle     = -99999;
    true_delta_pt            = -99999;
    true_delta_alphat        = -99999;
    true_delta_phit          = -99999;

    smeareff_particles_contained = true;
    smeareff_lep_contained       = false;
    smeareff_cc                  = -1;
    smeareff_nu_pdg              = -99999;
    smeareff_int_type            = -99999;
    smeareff_n_pipm              = 0;
    smeareff_n_pi0               = 0;
    smeareff_n_pr                = 0;
    smeareff_nu_energy           = -99999;
    smeareff_lep_mom             = -99999;
    smeareff_lep_theta           = -99999;
    smeareff_pr1_mom             = -99999;
    smeareff_pr1_theta           = -99999;
    smeareff_lep_pr1_angle       = -99999;
    smeareff_pipm1_mom           = -99999;
    smeareff_pipm1_theta         = -99999;
    smeareff_lep_pipm1_angle     = -99999;
    smeareff_delta_pt            = -99999;
    smeareff_delta_alphat        = -99999;
    smeareff_delta_phit          = -99999;

    reco_particles_contained = true;
    reco_lep_contained       = false;
    reco_cc                  = -1;
    reco_nu_pdg              = -99999;
    reco_int_type            = -99999;
    reco_n_pipm              = 0;
    reco_n_pi0               = 0;
    reco_n_pr                = 0;
    reco_nu_energy           = -99999;
    reco_lep_mom             = -99999;
    reco_lep_theta           = -99999;
    reco_pr1_mom             = -99999;
    reco_pr1_theta           = -99999;
    reco_lep_pr1_angle       = -99999;
    reco_pipm1_mom           = -99999;
    reco_pipm1_theta         = -99999;
    reco_lep_pipm1_angle     = -99999;
    reco_delta_pt            = -99999;
    reco_delta_alphat        = -99999;
    reco_delta_phit          = -99999;

  } // XSecTreeCosmic::ResetVars

  
  // Give us a list of the stable, primary particles that we're interested in
  std::vector<const simb::MCParticle*> XSecTreeCosmic::InterestingParticles(std::vector<const simb::MCParticle*> particles){

    std::vector<const simb::MCParticle*> interesting;

    // Loop over all of the particles
    for(size_t j = 0; j < particles.size(); j++){
      // Only consider stable final states particles
//      if(particles.at(j)->StatusCode() != 1) continue;
 
     // Only want primary particles
//      if(particles.at(j)->Mother() != 0) continue;
//commented out as don't just want primary products -> also want decay products

      // Only consider electrons, muons, pi0, charged pi and protons TODO for now...
      int pdg = std::abs(particles.at(j)->PdgCode());
      if(!(pdg == 11 || pdg == -11 || pdg == 22 || pdg == 13 || pdg == 111 || pdg == 211 || pdg == 2212)) continue;
      interesting.push_back(particles.at(j));
    }

    return interesting;

  } // XSecTreeCosmic::InterestingParticles()


  // Apply reconstruction efficiencies to true particles
  std::vector<const simb::MCParticle*> XSecTreeCosmic::RecoParticles(std::vector<const simb::MCParticle*> particles){

    std::vector<const simb::MCParticle*> reco_particles;

    for(size_t j = 0; j < particles.size(); j++){
      // Only consider stable final states particles
      if(particles.at(j)->StatusCode() != 1) continue;
      // Only want primary particles
      if(particles.at(j)->Mother() != 0) continue;

      int pdg = std::abs(particles.at(j)->PdgCode());
      if(!(pdg == 11 || pdg == 13 || pdg == 111 || pdg == 211 || pdg == 2212)) continue;

      // Apply efficiency cut (energy thresholds + flat efficiency)
      double rand_eff = fRandom->Rndm();
      if(pdg == 11 && (particles.at(j)->P() < fElectronThreshold || rand_eff > fElectronEff)) continue;
      if(pdg == 13 && (particles.at(j)->P() < fMuonThreshold || rand_eff > fMuonEff)) continue;
      // Consider the secondary photons when reconstructing pi0
      if(pdg == 111){
        int n_photons = 0;
        // Get the pi0 daughters
        std::vector<int> d_trk_ids;
        for(int k = 0; k < particles.at(j)->NumberDaughters(); k++){
          d_trk_ids.push_back(particles.at(j)->Daughter(k));
        }
        if(d_trk_ids.size() < 2) continue;
        // Match by track ID to g4 particles and apply threshold and effciencies
        for(size_t i = 0; i < particles.size(); i++){
          for(size_t k = 0; k < d_trk_ids.size(); k++){
            if(particles.at(i)->TrackId() != d_trk_ids.at(k)) continue;
            double rand_ph_eff = fRandom->Rndm();
            if(particles.at(i)->PdgCode() == 22 && 
             particles.at(i)->P() > fPhotonThreshold && 
             rand_ph_eff < fPhotonEff &&
             fTPCGeo.IsContained(*particles.at(i))) n_photons++;
          }
        }
        // Need to reconstruct 2 photons to reconstruct pi0
        if(n_photons < 2) continue;
      }
      if(pdg == 211 && (particles.at(j)->P() < fPionThreshold || rand_eff > fPionEff)) continue;
      if(pdg == 2212 && (particles.at(j)->P() < fProtonThreshold || rand_eff > fProtonEff)) continue;

      reco_particles.push_back(particles.at(j));
    }

    return reco_particles;

  } // XSecTreeCosmic::RecoParticles()

  // Smear electron momentum based on TRACS performance
  double XSecTreeCosmic::SmearElectronMomentum(double momentum){
    //For contained muons use range based bias and resolution
    //Values from Fig 5 of https://arxiv.org/pdf/1703.06187.pdf
    double bias = -0.15;
    double resolution = 0.33;
    double momentum_smear = fRandom->Gaus(momentum, resolution*momentum)+bias*momentum;
    if(momentum_smear<0){momentum_smear = 0;}

    return momentum_smear;

  } // XSecTreeCosmic::SmearElectronMomentum()

  // Smear momentum for exiting particles using MCS based method
  double XSecTreeCosmic::SmearMcsMomentum(double momentum){
    //For exiting muons use multiple coulomb scattering bias and resolution
    //Values from Fig 12 of https://arxiv.org/pdf/1703.06187.pdf
    double bias[] = {0.0273,0.0409,0.0352,0.0250,0.0227,0.0068,0.0364,0.0273,0.0227};
    double resolution[] = {0.127,0.145,0.143,0.141,0.164,0.177,0.250,0.266,0.341};
    int pos = 8; 
    for(int i=0; i<9; i++){
      if(momentum<(0.34+0.41*(i+1))) {pos = i; break;}
    }
    double momentum_smear = fRandom->Gaus(momentum, resolution[pos]*momentum) + bias[pos]*momentum;
    if(momentum_smear<0) {momentum_smear = 0;}

    return momentum_smear;

  } // XSecTreeCosmic::SmearMcsMomentum()


  // Smear momentum for contained particles using range based method
  double XSecTreeCosmic::SmearRangeMomentum(double momentum){
    //For contained muons use range based bias and resolution
    //Values from Fig 5 of https://arxiv.org/pdf/1703.06187.pdf
    double bias[] = {-0.0035,-0.0059,-0.0047,-0.0059,-0.0035,-0.0029,-0.0076,-0.0059,0.0006};
    double resolution[] = {0.017,0.021,0.023,0.026,0.025,0.030,0.030,0.040,0.032};
    int pos = 8; 
    for(int i=0; i<9; i++){
      if(momentum<(0.33+0.186*(i+1))) {pos = i; break;}
    }
    double momentum_smear = fRandom->Gaus(momentum, resolution[pos]*momentum)+bias[pos]*momentum;
    if(momentum_smear<0){momentum_smear = 0;}

    return momentum_smear;

  } // XSecTreeCosmic::SmearRangeMomentum()


  // Calculate the visible energy as neutrino energy estimator
  // TODO poorly copied from Gray's stuff, will likely change anyway
  double XSecTreeCosmic::VisibleEnergy(std::vector<const simb::MCParticle*> particles){
    //
    double visible_E = 0;

    for(size_t j = 0; j < particles.size(); j++){
      // Only consider stable final states particles
      if(particles.at(j)->StatusCode() != 1) continue;
      // Only want primary particles
      if(particles.at(j)->Mother() != 0) continue;

      int pdg = std::abs(particles.at(j)->PdgCode());
      if(pdg == 111) continue;

      if(pdg == 13){
        double smearing_percentage;
        if(fTPCGeo.IsContained(*particles.at(j))) smearing_percentage = 0.02;
        else smearing_percentage = -0.102 * TMath::Log(0.000612*fTPCGeo.TpcLength(*particles.at(j)));
        double lep_E = particles.at(j)->E();
        visible_E += std::max(fRandom->Gaus(lep_E, smearing_percentage*lep_E), 0.);
        continue;
      }

      double mass = particles.at(j)->Mass();
      double this_visible_energy = (particles.at(j)->E() - mass); // GeV
      this_visible_energy = fRandom->Gaus(this_visible_energy, 0.05*this_visible_energy);
      this_visible_energy = std::max(this_visible_energy, 0.);
      if(this_visible_energy > 0.021) visible_E += this_visible_energy;
    }

    return visible_E;

  } // XSecTreeCosmic::VisibleEnergy()


  // Calculate transverse variables (https://link.aps.org/accepted/10.1103/PhysRevC.94.015503)
  // TODO should this be just proton momentum or all other tracks?
  TVector3 XSecTreeCosmic::DeltaPT(TVector3 mu_mom, TVector3 pr_mom){
    // Assume neutrino is directly along Z, magnitude unimportant
    TVector3 nu_mom(0, 0, 1);

    // Get the transverse momentum for the muon and proton
    TVector3 mu_rot(mu_mom);
    mu_rot.Rotate(TMath::Pi(), nu_mom);
    TVector3 mu_pt = (mu_mom - mu_rot)*0.5;

    TVector3 pr_rot(mu_mom);
    pr_rot.Rotate(TMath::Pi(), nu_mom);
    TVector3 pr_pt = (pr_mom - pr_rot)*0.5;

    // Calculate the variables
    TVector3 tot_pt = mu_pt + pr_pt;
    double phi = TMath::ACos(mu_pt.Dot(pr_pt)*(-1)/mu_pt.Mag()/pr_pt.Mag());
    double theta = TMath::ACos(tot_pt.Dot(mu_pt)*(-1)/tot_pt.Mag()/mu_pt.Mag());

    TVector3 delta_pt;
    delta_pt.SetMagThetaPhi(tot_pt.Mag(), theta, phi);

    return delta_pt;

  } // XSecTreeCosmic::DeltaPT()


  // Inclusive charged current selection
  bool XSecTreeCosmic::IsCCInc(geo::Point_t vertex, std::vector<const simb::MCParticle*> reco_particles){
    bool cc_selected = true;
    // Check vertex is inside the fiducial volume
    if(!fTPCGeo.InFiducial(vertex, fWallCut, fWallCut, fWallCut, fWallCut, fWallCut, fBackCut)){ 
      if(fVerbose) std::cout<<"Not in fiducial\n";
      cc_selected = false; 
    }

    // Loop over the mu/pi/pr secondary particles and find the longest
    double max_contained_length = 0;
    int max_pdg = -99999;
    for(size_t j = 0; j < reco_particles.size(); j++){
      // Only consider track-like particles
      int pdg = std::abs(reco_particles.at(j)->PdgCode());
      if(!(pdg == 13 || pdg == 211 || pdg == 2212)) continue;

      double contained_length = fTPCGeo.TpcLength(*reco_particles.at(j));
      if(contained_length < max_contained_length) continue;
      max_contained_length = contained_length;
      max_pdg = pdg;
      longest_j = j;

      // Check if particle is contained
      reco_lep_contained = fTPCGeo.IsContained(*reco_particles.at(j));

      std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(j));
      TVector3 start = cross_points.first;
      TVector3 end = cross_points.second;
      reco_lep_theta = (end - start).Theta();

      // Smear momentum based on whether particle is contained or not
      if(reco_lep_contained) reco_lep_mom = fRangeFitter.GetTrackMomentum(contained_length, 13);
      else reco_lep_mom = SmearMcsMomentum(reco_particles.at(j)->P());
    }

    if(fVerbose) std::cout<<"max contained length = "<<max_contained_length<<" pdg = "<<max_pdg<<"\n";

    // Check length of particle
    if(reco_lep_contained && max_contained_length < fMinContainedLength){ 
      cc_selected = false;
      if(fVerbose) std::cout<<"contained length too short\n";
    }
    if(!reco_lep_contained && max_contained_length < fMinExitingLength){ 
      cc_selected = false;
      if(fVerbose) std::cout<<"exiting length too short\n";
    }

    return cc_selected;
  }

//  out->cd();
//  fXSecTreeCosmic->Write();
//  out->Close();

  DEFINE_ART_MODULE(XSecTreeCosmic)
} // namespace sbnd
