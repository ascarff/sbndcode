////////////////////////////////////////////////////////////////////////
// Class:       GenieTree
// Module Type: analyzer
// File:        GenieTree_module.cc
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
#include "larcoreobj/SummaryData/POTSummary.h"

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

  class GenieTree : public art::EDAnalyzer {
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

    }; // Config
 
    using Parameters = art::EDAnalyzer::Table<Config>;
 
    // Constructor: configures module
    explicit GenieTree(Parameters const& config);
 
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

  private:

    // fcl file parameters
    art::InputTag fGenModuleLabel;      ///< name of gen producer

    // Tree
    TTree *fGenieTree;

    // XSec tree true neutrino vertex
    double vtx_x;
    double vtx_y;
    double vtx_z;

    bool has_pdg_0;
    int event_number;

    // XSec true tree parameters
    bool true_cc;
    int true_nu_pdg;
    int true_int_type;
    unsigned int true_n_pipm;
    unsigned int true_n_pi0;
    unsigned int true_n_pr;
    double true_nu_energy;
    double true_lep_mom;
    double true_lep_theta;

    TTree *fMetaDataTree;
    double pot;
  }; // class GenieTree


  // Constructor
  GenieTree::GenieTree(Parameters const& config)
    : EDAnalyzer(config)
    , fGenModuleLabel       (config().GenModuleLabel())
  {

  }


  void GenieTree::beginJob()
  {
    // Access tfileservice to handle creating and writing histograms
    art::ServiceHandle<art::TFileService> tfs;
    // Define histograms
    fGenieTree = tfs->make<TTree>("interaction", "genie tree");

    // True neutrino vertex
    fGenieTree->Branch("vtx_x", &vtx_x);
    fGenieTree->Branch("vtx_y", &vtx_y);
    fGenieTree->Branch("vtx_z", &vtx_z);

    fGenieTree->Branch("has_pdg_0", &has_pdg_0);
    fGenieTree->Branch("event_number", &event_number);

    // True selection and kinematic variables
    fGenieTree->Branch("true_cc", &true_cc);
    fGenieTree->Branch("true_nu_pdg", &true_nu_pdg);
    fGenieTree->Branch("true_int_type", &true_int_type);
    fGenieTree->Branch("true_n_pipm", &true_n_pipm);
    fGenieTree->Branch("true_n_pi0", &true_n_pi0);
    fGenieTree->Branch("true_n_pr", &true_n_pr);
    fGenieTree->Branch("true_nu_energy", &true_nu_energy);
    fGenieTree->Branch("true_lep_mom", &true_lep_mom);
    fGenieTree->Branch("true_lep_theta", &true_lep_theta);

    fMetaDataTree = tfs->make<TTree>("metadata", "xsec tree");
    fMetaDataTree->Branch("pot", &pot);

  } // GenieTree::beginJob()

  // Called for every sub run
  void GenieTree::beginSubRun(const art::SubRun& subrun){

    art::Handle< sumdata::POTSummary > potHandle;
    subrun.getByLabel(fGenModuleLabel, potHandle);
    const sumdata::POTSummary& potSum = (*potHandle);
    pot = potSum.totpot;

    fMetaDataTree->Fill();
    return;
  } // XSecTree::beginSubRun()

  void GenieTree::analyze(const art::Event& event)
  {


    //----------------------------------------------------------------------------------------------------------
    //                                          GETTING PRODUCTS
    //----------------------------------------------------------------------------------------------------------
    // Retrieve all the truth info in the events
    art::Handle<std::vector<simb::MCTruth>> genHandle;
    std::vector<art::Ptr<simb::MCTruth>> mctruthList;
    if(event.getByLabel(fGenModuleLabel, genHandle)) art::fill_ptr_vector(mctruthList, genHandle);

    //----------------------------------------------------------------------------------------------------------
    //                                           FILLING THE TREE
    //----------------------------------------------------------------------------------------------------------
    // Loop over all the neutrino interactions
    for (size_t i = 0; i < mctruthList.size(); i++){
      // Reset all the tree variables
      ResetVars();

      event_number = event.id().event();

      // Get the pointer to the MCTruth object
      art::Ptr<simb::MCTruth> mctruth = mctruthList.at(i);

      // Check the interaction is within the TPC
      geo::Point_t vertex {mctruth->GetNeutrino().Nu().Vx(), 
                           mctruth->GetNeutrino().Nu().Vy(), 
                           mctruth->GetNeutrino().Nu().Vz()};

      vtx_x = vertex.X();
      vtx_y = vertex.Y();
      vtx_z = vertex.Z();

      if(std::abs(vtx_x) > 200 || std::abs(vtx_y) > 200 || vtx_z < 0 || vtx_z > 500) continue;

      //---------------------- FILLING ALL THE TRUE INTERACTION PARAMETERS ------------------------------------

      // Fill all the neutrino parameters
      if(mctruth->GetNeutrino().CCNC() == simb::kCC) true_cc = true;
      else true_cc = false;
      true_nu_pdg = mctruth->GetNeutrino().Nu().PdgCode();
      if(!true_cc || true_nu_pdg!=14) continue;
      true_int_type = mctruth->GetNeutrino().Mode();
      true_nu_energy = mctruth->GetNeutrino().Nu().E();

      true_lep_mom = mctruth->GetNeutrino().Lepton().P();
      true_lep_theta = mctruth->GetNeutrino().Lepton().Momentum().Vect().Theta();

      size_t nparts = mctruth->NParticles();
      // Lepton stuff
      for(size_t j = 0; j < nparts; j++){
        int pdg = std::abs(mctruth->GetParticle(j).PdgCode());
        if(pdg == 0) has_pdg_0 = true;
        if(mctruth->GetParticle(j).StatusCode()!=1) continue;
        if(pdg == 111) true_n_pi0++;
        if(pdg == 211) true_n_pipm++;
        if(pdg == 2212) true_n_pr++;
      }

      fGenieTree->Fill();
    }

  } // GenieTree::analyze()


  void GenieTree::endJob(){

  } // GenieTree::endJob()


  // Reset variables and counters
  void GenieTree::ResetVars(){

    vtx_x = -99999;
    vtx_y = -99999;
    vtx_z = -99999;

    has_pdg_0 = false;
    
    true_cc                  = false;
    true_nu_pdg              = -99999;
    true_int_type            = -99999;
    true_n_pipm              = 0;
    true_n_pi0               = 0;
    true_n_pr                = 0;
    true_nu_energy           = -99999;
    true_lep_mom             = -99999;
    true_lep_theta           = -99999;
  } // GenieTree::ResetVars

  DEFINE_ART_MODULE(GenieTree)
} // namespace sbnd
