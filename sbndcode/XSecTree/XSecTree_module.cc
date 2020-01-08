////////////////////////////////////////////////////////////////////////
// Class:       XSecTree
// Module Type: analyzer
// File:        XSecTree_module.cc
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
#include "larreco/RecoAlg/TrackMomentumCalculator.h"
#include "larsim/EventWeight/Base/Weight_t.h"
#include "larsim/EventWeight/Base/MCEventWeight.h"
#include "larsim/EventWeight/Base/WeightManager.h"

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

  class XSecTree : public art::EDAnalyzer {
  public:

    explicit XSecTree(fhicl::ParameterSet const & p);

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
    void ResetWeights();

    // Give us a list of the stable, primary particles that we're interested in
    std::vector<const simb::MCParticle*> InterestingParticles(std::vector<const simb::MCParticle*> particles);

    // Apply reconstruction efficiencies to true particles
    std::vector<const simb::MCParticle*> RecoParticles(std::vector<const simb::MCParticle*> particles);

    // Smear momentum for electrons from TRACS performance
    double SmearElectronMomentum(double momentum);

    // Smear momentum for exiting particles using MCS based method
    double SmearMcsMomentum(double momentum);

    // Smear MCS momentum using SBND simulation
    double SmearMcsMomentumSbnd(double momentum, double length);

    // Use histograms for efficiencies
    bool MuonHistEff(double momentum);
    bool PionHistEff(double momentum);
    bool ProtonHistEff(double momentum);

    // Smear momentum for contained particles using range based method
    double SmearRangeMomentum(double momentum);

    // Calculate the visible energy as neutrino energy estimator
    double VisibleEnergy(std::vector<const simb::MCParticle*> particles);

    // Calculate transverse variables (https://link.aps.org/accepted/10.1103/PhysRevC.94.015503)
    TVector3 DeltaPT(TVector3 mu_mom, TVector3 pr_mom);

    // Inclusive charged current selection
    bool IsCCInc(geo::Point_t vertex, std::vector<const simb::MCParticle*> reco_particles);

    // Use histograms for PID
    bool MuonHistPid(double momentum, int pdg);
    bool ProtonHistPid(double momentum);

  private:

    // fcl file parameters
    art::InputTag fGenModuleLabel;      ///< name of gen producer
    art::InputTag fG4ModuleLabel;       ///< name of g4 producer
    art::InputTag fFluxWeightLabel;
    art::InputTag fGenieWeightLabel;
    bool          fVerbose;             ///< print information about what's going on

    double fXminCut;
    double fXmaxCut;
    double fYminCut;
    double fYmaxCut;
    double fZminCut;
    double fZmaxCut;
    double fCpaCut;
    double fApaCut;

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

    bool fUseHistEfficiency;
    bool fUseSbndSmearing;
    bool fUseHistPid;

    // List of reco formats
    const std::vector<std::string> fRecoFormats;
    /*
    std::vector<std::string> fWeightCalcs {
    "genie_all_Genie",
    "genie_AGKYpT_Genie",
    "genie_AGKYxF_Genie",
    "genie_DISAth_Genie",
    "genie_DISBth_Genie",
    "genie_DISCv1u_Genie",
    "genie_DISCv2u_Genie",
    "genie_FermiGasModelKf_Genie",
    "genie_FermiGasModelSf_Genie",
    "genie_FormZone_Genie",
    "genie_IntraNukeNabs_Genie",
    "genie_IntraNukeNcex_Genie",
    "genie_IntraNukeNel_Genie",
    "genie_IntraNukeNinel_Genie",
    "genie_IntraNukeNmfp_Genie",
    "genie_IntraNukeNpi_Genie",
    "genie_IntraNukePIabs_Genie",
    "genie_IntraNukePIcex_Genie",
    "genie_IntraNukePIel_Genie",
    "genie_IntraNukePIinel_Genie",
    "genie_IntraNukePImfp_Genie",
    "genie_IntraNukePIpi_Genie",
    "genie_NC_Genie",
    "genie_NonResRvbarp1piAlt_Genie",
    "genie_NonResRvbarp1pi_Genie",
    "genie_NonResRvbarp2piAlt_Genie",
    "genie_NonResRvbarp2pi_Genie",
    "genie_NonResRvp1piAlt_Genie",
    "genie_NonResRvp1pi_Genie",
    "genie_NonResRvp2piAlt_Genie",
    "genie_NonResRvp2pi_Genie",
    "genie_ResDecayEta_Genie",
    "genie_ResDecayGamma_Genie",
    "genie_ResDecayTheta_Genie",
    "genie_ccresAxial_Genie",
    "genie_ccresVector_Genie",
    "genie_cohMA_Genie",
    "genie_cohR0_Genie",
    "genie_ncelAxial_Genie",
    "genie_ncelEta_Genie",
    "genie_ncresAxial_Genie",
    "genie_ncresVector_Genie",
    "genie_qema_Genie",
    "genie_qevec_Genie",
    "bnbcorrection_FluxHist",
    "expskin_FluxUnisim",
    "horncurrent_FluxUnisim",
    "kminus_PrimaryHadronNormalization",
    "kplus_PrimaryHadronFeynmanScaling",
    "kzero_PrimaryHadronSanfordWang",
    "nucleoninexsec_FluxUnisim",
    "nucleonqexsec_FluxUnisim",
    "nucleontotxsec_FluxUnisim",
    "piminus_PrimaryHadronSWCentralSplineVariation",
    "pioninexsec_FluxUnisim",
    "pionqexsec_FluxUnisim",
    "piontotxsec_FluxUnisim",
    "piplus_PrimaryHadronSWCentralSplineVariation"
    };
    */

    TPCGeoAlg fTPCGeo;
    trkf::TrackMomentumCalculator fRangeFitter;

    evwgh::WeightManager _wgt_manager;

    TRandom2 *fRandom;

    // Global variables
    int lep_j;
    int longest_j;

    // Tree
    TTree *fXSecTree;
    TTree *fMetaDataTree;
    TTree *fWeightTree;

    // XSec tree true neutrino vertex
    double vtx_x, vtx_y, vtx_z;

    bool has_pdg_0;

    // XSec true tree parameters
    std::map<std::string, bool> particles_contained;
    std::map<std::string, bool> lep_contained;
    std::map<std::string, bool> cc;
    std::map<std::string, int> nu_pdg;
    std::map<std::string, int> int_type;
    std::map<std::string, unsigned int> n_pipm;
    std::map<std::string, unsigned int> n_pi0;
    std::map<std::string, unsigned int> n_pr;
    std::map<std::string, double> nu_energy;
    std::map<std::string, double> lep_mom;
    std::map<std::string, double> lep_theta;
    std::map<std::string, double> pr1_mom;
    std::map<std::string, double> pr1_theta;
    std::map<std::string, double> lep_pr1_angle;
    std::map<std::string, double> pipm1_mom;
    std::map<std::string, double> pipm1_theta;
    std::map<std::string, double> lep_pipm1_angle;
    std::map<std::string, double> delta_pt;
    std::map<std::string, double> delta_alphat;
    std::map<std::string, double> delta_phit;

    // MetaData tree parameters
    double pot;

    // Weight tree parameters
    //std::map<std::string, std::vector<double>> weights;
    double weights[500];

  }; // class XSecTree

  XSecTree::XSecTree(fhicl::ParameterSet const & p)
    : EDAnalyzer{p}
    , fGenModuleLabel       {p.get<art::InputTag>("GenModuleLabel")}
    , fG4ModuleLabel        {p.get<art::InputTag>("G4ModuleLabel")}
    , fFluxWeightLabel      {p.get<art::InputTag>("FluxWeightLabel")}
    , fGenieWeightLabel     {p.get<art::InputTag>("GenieWeightLabel")}
    , fVerbose              {p.get<bool>("Verbose")}
    , fXminCut              {p.get<double>("XminCut")}
    , fXmaxCut              {p.get<double>("XmaxCut")}
    , fYminCut              {p.get<double>("YminCut")}
    , fYmaxCut              {p.get<double>("YmaxCut")}
    , fZminCut              {p.get<double>("ZminCut")}
    , fZmaxCut              {p.get<double>("ZmaxCut")}
    , fCpaCut               {p.get<double>("CpaCut")}
    , fApaCut               {p.get<double>("ApaCut")}
    , fMinContainedLength   {p.get<double>("MinContainedLength")}
    , fMinExitingLength     {p.get<double>("MinExitingLength")}
    , fMuonThreshold        {p.get<double>("MuonThreshold")}
    , fPi0Threshold         {p.get<double>("Pi0Threshold")}
    , fPhotonThreshold      {p.get<double>("PhotonThreshold")}
    , fPionThreshold        {p.get<double>("PionThreshold")}
    , fProtonThreshold      {p.get<double>("ProtonThreshold")}
    , fMuonEff              {p.get<double>("MuonEff")}
    , fPi0Eff               {p.get<double>("Pi0Eff")}
    , fPhotonEff            {p.get<double>("PhotonEff")}
    , fPionEff              {p.get<double>("PionEff")}
    , fProtonEff            {p.get<double>("ProtonEff")}
    , fPionPidEff           {p.get<double>("PionPidEff")}
    , fProtonPidEff         {p.get<double>("ProtonPidEff")}
    , fUseHistEfficiency    {p.get<bool>("UseHistEfficiency")}
    , fUseSbndSmearing      {p.get<bool>("UseSbndSmearing")}
    , fUseHistPid           {p.get<bool>("UseHistPid")}
    , fRecoFormats          ({"true","eff","smeareff","reco"})
  {

    //_wgt_manager.Configure(p, *this);

  }

  void XSecTree::beginJob()
  {
    // Access tfileservice to handle creating and writing histograms
    art::ServiceHandle<art::TFileService> tfs;
    
    // Define histograms
    fXSecTree = tfs->make<TTree>("interaction", "xsec tree");

    // Selection and kinematic variables
    fXSecTree->Branch("vtx_x", &vtx_x);
    fXSecTree->Branch("vtx_y", &vtx_y);
    fXSecTree->Branch("vtx_z", &vtx_z);
    for(unsigned int i = 0; i < fRecoFormats.size(); ++i){
      fXSecTree->Branch((fRecoFormats[i]+"_particles_contained").c_str(), &particles_contained[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_lep_contained").c_str(), &lep_contained[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_cc").c_str(), &cc[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_nu_pdg").c_str(), &nu_pdg[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_int_type").c_str(), &int_type[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_n_pipm").c_str(), &n_pipm[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_n_pi0").c_str(), &n_pi0[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_n_pr").c_str(), &n_pr[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_nu_energy").c_str(), &nu_energy[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_lep_mom").c_str(), &lep_mom[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_lep_theta").c_str(), &lep_theta[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_pr1_mom").c_str(), &pr1_mom[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_pr1_theta").c_str(), &pr1_theta[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_lep_pr1_angle").c_str(), &lep_pr1_angle[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_pipm1_mom").c_str(), &pipm1_mom[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_pipm1_theta").c_str(), &pipm1_theta[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_lep_pipm1_angle").c_str(), &lep_pipm1_angle[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_delta_pt").c_str(), &delta_pt[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_delta_alphat").c_str(), &delta_alphat[fRecoFormats[i]]);
      fXSecTree->Branch((fRecoFormats[i]+"_delta_phit").c_str(), &delta_phit[fRecoFormats[i]]);
    }

    fMetaDataTree = tfs->make<TTree>("metadata", "xsec tree");
    fMetaDataTree->Branch("pot", &pot);

    /*for(unsigned int i = 0; i < fWeightCalcs.size(); ++i){
      weights[fWeightCalcs[i]].push_back(0.);
    }*/
    fWeightTree = tfs->make<TTree>("weight", "xsec tree");
    fWeightTree->Branch("weights", &weights, "weights[500]/D");
    /*for(unsigned int i = 0; i < fWeightCalcs.size(); ++i){
      fWeightTree->Branch(fWeightCalcs[i].c_str(), &weights[fWeightCalcs[i]]);
    }*/

    // Initial output
    std::cout<<"----------------- XSec Tree Module -------------------"<<std::endl;

    fRandom = new TRandom2();

  } // XSecTree::beginJob()


  // Called for every sub run
  void XSecTree::beginSubRun(const art::SubRun& subrun){

    art::Handle< sumdata::POTSummary > potHandle;
    subrun.getByLabel(fGenModuleLabel, potHandle);
    const sumdata::POTSummary& potSum = (*potHandle);
    pot = potSum.totpot;

    fMetaDataTree->Fill();
    return;
  } // XSecTree::beginSubRun()


  void XSecTree::analyze(const art::Event& event)
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

    // Weights
    art::Handle<std::vector<evwgh::MCEventWeight>> fluxWeightHandle;
    std::vector<art::Ptr<evwgh::MCEventWeight>> fluxWeightList;
    if(event.getByLabel(fFluxWeightLabel, fluxWeightHandle)) art::fill_ptr_vector(fluxWeightList, fluxWeightHandle);

    art::Handle<std::vector<evwgh::MCEventWeight>> genieWeightHandle;
    std::vector<art::Ptr<evwgh::MCEventWeight>> genieWeightList;
    if(event.getByLabel(fGenieWeightLabel, genieWeightHandle)) art::fill_ptr_vector(genieWeightList, genieWeightHandle);

    //----------------------------------------------------------------------------------------------------------
    //                                           REWEIGHTING MCTRUTH
    //----------------------------------------------------------------------------------------------------------
    if(fluxWeightList.size() == mctruthList.size() && genieWeightList.size() == mctruthList.size()){
      for(unsigned int inu = 0; inu < mctruthList.size(); ++inu) {
        ResetWeights();

        // Get the pointer to the MCTruth object
        art::Ptr<simb::MCTruth> mctruth = mctruthList.at(inu);
        // Check the interaction is within the TPC
        geo::Point_t vertex {mctruth->GetNeutrino().Nu().Vx(), 
                             mctruth->GetNeutrino().Nu().Vy(), 
                             mctruth->GetNeutrino().Nu().Vz()};
        if(!fTPCGeo.InFiducial(vertex, 0.)) continue;

        for(auto const &kv : fluxWeightList[inu]->fWeight){
          std::cout<<"Name = "<<kv.first<<" vector size = "<<kv.second.size()<<"\n";
          /*if(weights.find(kv.first) != weights.end()){ 
            std::cout<<"->Name = "<<kv.first<<" vector size = "<<kv.second.size()<<"\n";
            weights[kv.first] = kv.second;
          }*/
          if(kv.second.size()!=500) continue;
          for(size_t i = 0; i < 500; i++){
            weights[i] *= kv.second[i];
          }
        }
        for(auto const &kv : genieWeightList[inu]->fWeight){
          std::cout<<"Name = "<<kv.first<<" vector size = "<<kv.second.size()<<"\n";
          //if(weights.find(kv.first) != weights.end()) weights[kv.first] = kv.second;
          if(kv.second.size()!=500) continue;
          for(size_t i = 0; i < 500; i++){
            weights[i] *= kv.second[i];
          }
        }
        fWeightTree->Fill();
      }
    }
    else{
      std::cout<<"Weights not found in file, calculating...\n";
      /*
      // Implementation of required member function here.
      for (unsigned int inu = 0; inu < mctruthList.size(); ++inu) {
        ResetWeights();
        // Copy event to non const type FIXME very naughty
        art::Event& e = const_cast<art::Event&>(event);
        evwgh::MCEventWeight mcwgh = _wgt_manager.Run(e, inu);
        std::cout<<"Number of weights = "<<mcwgh.fWeight.size()<<"\n";
        for(auto const &kv : mcwgh.fWeight){
          std::cout<<"Name = "<<kv.first<<" vector size = "<<kv.second.size()<<"\n";
          if(weights.find(kv.first) != weights.end()) weights[kv.first] = kv.second;
        }
        fWeightTree->Fill();
      }
      */
    }

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
      geo::Point_t vertex {mctruth->GetNeutrino().Nu().Vx(), 
                           mctruth->GetNeutrino().Nu().Vy(), 
                           mctruth->GetNeutrino().Nu().Vz()};
      if(fVerbose) std::cout<<"->Vertex: ("<<vertex.X()<<", "<<vertex.Y()<<", "<<vertex.Z()<<")\n";
      if(!fTPCGeo.InFiducial(vertex, 0.)) continue;

      vtx_x = vertex.X();
      vtx_y = vertex.Y();
      vtx_z = vertex.Z();

      //---------------------- FILLING ALL THE TRUE INTERACTION PARAMETERS ------------------------------------

      // Fill all the neutrino parameters
      if(mctruth->GetNeutrino().CCNC() == simb::kCC) cc["true"] = 1;
      else cc["true"]   = 0;
      nu_pdg["true"]    = mctruth->GetNeutrino().Nu().PdgCode();
      int_type["true"]  = mctruth->GetNeutrino().Mode();
      nu_energy["true"] = mctruth->GetNeutrino().Nu().E();

      // Get all the particles propagated by geant 4
      std::vector<const simb::MCParticle*> parts     = pi_serv->MCTruthToParticles_Ps(mctruth);
      // Get only the interesting ones
      std::vector<const simb::MCParticle*> particles = InterestingParticles(parts);

      if(fVerbose) std::cout<<particles.size() << " interesting particles\n";

      // Lepton stuff
      for(size_t j = 0; j < particles.size(); j++){
        int pdg = std::abs(particles.at(j)->PdgCode());
        if(pdg == 0) has_pdg_0 = true;
        // Only consider the primary muon
        if(!(pdg == 13 || pdg == 11)) continue;
        lep_j                 = j;
        TVector3 start        = particles.at(j)->Position().Vect();
        TVector3 end          = particles.at(j)->EndPosition().Vect();
        lep_mom["true"]       = particles.at(j)->P();
        lep_theta["true"]     = (end - start).Theta();
        lep_contained["true"] = fTPCGeo.IsContained(*particles.at(j));
      }

      // Secondary particle stuff
      for(size_t j = 0; j < particles.size(); j++){

        if((int)j == lep_j) continue;

        // Only consider pi0, charged pi and protons
        int pdg        = std::abs(particles.at(j)->PdgCode());
        TVector3 start = particles.at(j)->Position().Vect();
        TVector3 end   = particles.at(j)->EndPosition().Vect();

        // Count particle numbers
        if(pdg == 111){ 
          n_pi0["true"]++;
          if(!fTPCGeo.IsContained(*particles.at(j))) particles_contained["true"] = false;
        }
        if(pdg == 211){ 
          n_pipm["true"]++;
          if(!fTPCGeo.IsContained(*particles.at(j))) particles_contained["true"] = false;

          if(particles.at(j)->P() < pipm1_mom["true"]) continue;
          pipm1_mom["true"]   = particles.at(j)->P();
          pipm1_theta["true"] = (end - start).Theta();

          // Calculate things relative to the lepton if there is one
          if(lep_j == -1) continue;
          TVector3 lep_end        = particles.at(lep_j)->EndPosition().Vect();
          lep_pipm1_angle["true"] = lep_end.Angle(end);
        }
        if(pdg == 2212){ 
          n_pr["true"]++;
          if(!fTPCGeo.IsContained(*particles.at(j))) particles_contained["true"] = false;

          if(particles.at(j)->P() < pr1_mom["true"]) continue;
          pr1_mom["true"]   = particles.at(j)->P();
          pr1_theta["true"] = (end - start).Theta();

          // Calculate things relative to the lepton if there is one
          if(lep_j == -1) continue;
          TVector3 lep_end      = particles.at(lep_j)->EndPosition().Vect();
          lep_pr1_angle["true"] = lep_end.Angle(end);

          // Transverse variables
          TVector3 true_delta_pt = DeltaPT(particles.at(lep_j)->Momentum().Vect(), particles.at(j)->Momentum().Vect());
          delta_pt["true"]       = true_delta_pt.Mag();
          delta_alphat["true"]   = true_delta_pt.Theta()*TMath::RadToDeg();
          delta_phit["true"]     = true_delta_pt.Phi()*TMath::RadToDeg();
        }

      }

      //----------------------------- EFFICIENCY + SMEARING --------------------------------------
      // Apply reconstruction efficiencies to the true particles
      std::vector<const simb::MCParticle*> reco_particles = RecoParticles(parts);
      if(fVerbose) std::cout<<"Number of reconstructed particles = "<<reco_particles.size()<<"\n";
      
      // Smearing won't change these, access from truth
      if(mctruth->GetNeutrino().CCNC() == simb::kCC){
        cc["eff"]      = 1;
        cc["smeareff"] = 1;
      }
      else {
        cc["eff"]        = 0;
        cc["smeareff"]   = 0;
      }
      
      // Particle selection efficiency only
      nu_pdg["eff"]    = mctruth->GetNeutrino().Nu().PdgCode();
      int_type["eff"]  = mctruth->GetNeutrino().Mode();
      nu_energy["eff"] = mctruth->GetNeutrino().Nu().E();
      
      // Smearing + efficiency 
      nu_pdg["smeareff"]    = mctruth->GetNeutrino().Nu().PdgCode();
      int_type["smeareff"]  = mctruth->GetNeutrino().Mode();

      // TODO Make a better neutrino energy calculator
      nu_energy["smeareff"] = VisibleEnergy(reco_particles);
      if(fVerbose){
        std::cout<<"True energy    = "<< nu_energy["eff"] <<"\n";
        std::cout<<"Smeared energy = "<< nu_energy["smeareff"] <<"\n";
      }

      // Reset the lepton index
      lep_j = -1;
      // Get the lepton kinematics if CC
      for(size_t j = 0; j < reco_particles.size(); j++){
        int pdg = std::abs(reco_particles.at(j)->PdgCode());

        // Get the lepton kinematics - electrons
        if(pdg == 11 || pdg == 13){
          std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(j));

          TVector3 start                             = cross_points.first;
          TVector3 end                               = cross_points.second;
          double contained_length                    = fTPCGeo.TpcLength(*reco_particles.at(j));

          lep_j                             = j;
          lep_theta["eff"]                  = (end - start).Theta();
          lep_contained["eff"]              = fTPCGeo.IsContained(*reco_particles.at(j));
          lep_theta["smeareff"]             = (end - start).Theta();
          lep_contained["smeareff"]         = fTPCGeo.IsContained(*reco_particles.at(j));


          if(pdg == 11){
            // When not smearing, just take true momentum
            lep_mom["eff"]      = reco_particles.at(j)->P();
            lep_mom["smeareff"] = SmearElectronMomentum(reco_particles.at(j)->P());
          }
          else if(lep_contained["smeareff"])
            lep_mom["smeareff"] = fRangeFitter.GetTrackMomentum(contained_length, 13);
          else{ 
            if(!fUseSbndSmearing) lep_mom["smeareff"] = SmearMcsMomentum(reco_particles.at(j)->P());
            else lep_mom["smeareff"] = SmearMcsMomentumSbnd(reco_particles.at(j)->P(), contained_length);
          }
          
          // When not smearing, just take true momentum
          if(pdg == 13)
            lep_mom["eff"] = reco_particles.at(j)->P();
        }
      }
      if(fVerbose) std::cout<<"Smeared the lepton kinematics\n";

      // Loop over all the reconstructed particles and smear kinematic variables
      for(size_t j = 0; j < reco_particles.size(); j++){
        if ((int)j == lep_j) continue;

        int pdg                                    = std::abs(reco_particles.at(j)->PdgCode());
        std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(j));
        TVector3 start                             = cross_points.first;
        TVector3 end                               = cross_points.second;
        double contained_length                    = fTPCGeo.TpcLength(*reco_particles.at(j));

        // Count particle numbers and smear - pi0
        if(pdg == 111){
          n_pi0["eff"]++;
          n_pi0["smeareff"]++;
          if(!fTPCGeo.IsContained(*reco_particles.at(j))){
            particles_contained["eff"]      = false;
            particles_contained["smeareff"] = false;
          }
        }

        // Count particle numbers and smear - pions
        if(pdg == 211){ 
          n_pipm["eff"]++;
          n_pipm["smeareff"]++;

          if(!fTPCGeo.IsContained(*reco_particles.at(j))){
            particles_contained["eff"]      = false;
            particles_contained["smeareff"] = false;
          }

          double smear_pipm1_mom = fRangeFitter.GetTrackMomentum(contained_length, 13);
          double true_pipm1_mom  = reco_particles.at(j)->P();

          if(true_pipm1_mom >= pipm1_mom["eff"]){
            pipm1_mom["eff"]   = true_pipm1_mom;
            pipm1_theta["eff"] = (end - start).Theta();
            
            // Calculate things relative to the lepton if there is one
            if(lep_j == -1) continue;
            TVector3 lep_end       = fTPCGeo.CrossingPoints(*reco_particles.at(lep_j)).second;
            lep_pipm1_angle["eff"] = lep_end.Angle(end);
          }

          if(smear_pipm1_mom >= pipm1_mom["smeareff"]){
            pipm1_mom["smeareff"]   = smear_pipm1_mom;
            pipm1_theta["smeareff"] = (end - start).Theta();
            
            // Calculate things relative to the lepton if there is one
            if(lep_j == -1) continue;
            TVector3 lep_end            = fTPCGeo.CrossingPoints(*reco_particles.at(lep_j)).second;
            lep_pipm1_angle["smeareff"] = lep_end.Angle(end);
          }
        }

        // Count particle numbers and smear - protons
        if(pdg == 2212){ 
          n_pr["eff"]++;
          n_pr["smeareff"]++;

          if(!fTPCGeo.IsContained(*reco_particles.at(j))){
            particles_contained["eff"]      = false;
            particles_contained["smeareff"] = false;
          }

          double smear_pr1_mom = fRangeFitter.GetTrackMomentum(contained_length, 2212);
          double true_pr1_mom  = reco_particles.at(j)->P();

          if(true_pr1_mom >= pr1_mom["eff"]){
            pr1_mom["eff"]   = true_pr1_mom;
            pr1_theta["eff"] = (end - start).Theta();

            // Calculate things relative to the lepton if there is one
            if(lep_j == -1) continue;
            TVector3 lep_end     = fTPCGeo.CrossingPoints(*reco_particles.at(lep_j)).second;
            lep_pr1_angle["eff"] = lep_end.Angle(end);
            
            // Transverse variables
            TVector3 scaled_lep_mom = reco_particles.at(lep_j)->Momentum().Vect()*(lep_mom["eff"]/reco_particles.at(lep_j)->P());
            TVector3 scaled_pr_mom  = reco_particles.at(j)->Momentum().Vect()*(true_pr1_mom/reco_particles.at(j)->P());
            TVector3 reco_delta_pt  = DeltaPT(scaled_lep_mom, scaled_pr_mom);
            delta_pt["eff"]     = reco_delta_pt.Mag();
            delta_alphat["eff"] = reco_delta_pt.Theta()*TMath::RadToDeg();
            delta_phit["eff"]   = reco_delta_pt.Phi()*TMath::RadToDeg();
          }
          
          if(smear_pr1_mom >= pr1_mom["smeareff"]){
            pr1_mom["smeareff"]   = smear_pr1_mom;
            pr1_theta["smeareff"] = (end - start).Theta();
            
            // Calculate things relative to the lepton if there is one
            if(lep_j == -1) continue;
            TVector3 lep_end          = fTPCGeo.CrossingPoints(*reco_particles.at(lep_j)).second;
            lep_pr1_angle["smeareff"] = lep_end.Angle(end);
            
            // Transverse variables
            TVector3 scaled_lep_mom = reco_particles.at(lep_j)->Momentum().Vect()*(lep_mom["smeareff"]/reco_particles.at(lep_j)->P());
            TVector3 scaled_pr_mom  = reco_particles.at(j)->Momentum().Vect()*(smear_pr1_mom/reco_particles.at(j)->P());
            TVector3 reco_delta_pt  = DeltaPT(scaled_lep_mom, scaled_pr_mom);
            delta_pt["smeareff"]     = reco_delta_pt.Mag();
            delta_alphat["smeareff"] = reco_delta_pt.Theta()*TMath::RadToDeg();
            delta_phit["smeareff"]   = reco_delta_pt.Phi()*TMath::RadToDeg();
          }
        }
      }

      //---------------------------- CC INCLUSIVE SELECTION --------------------------------------

      // Same as smeared neutrino energy
      nu_energy["reco"] = VisibleEnergy(reco_particles);

      bool cc_selected = IsCCInc(vertex, reco_particles); 

      if(cc_selected){ 
        if(fVerbose) std::cout<<"Selected as CC\n";
        cc["reco"]     = 1;
        nu_pdg["reco"] = 14;
      }
      else{ 
        cc["reco"] = -1;
      }

      //------------------------------------ RECO FSI ------------------------------------------

      for(size_t j = 0; j < reco_particles.size(); j++){
        // Don't look at the particle selected as the muon
        if(longest_j == (int)j) continue;

        int pdg                                    = std::abs(reco_particles.at(j)->PdgCode());
        double contained_length                    = fTPCGeo.TpcLength(*reco_particles.at(j));
        std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(j));
        TVector3 start                             = cross_points.first;
        TVector3 end                               = cross_points.second;

        // Apply PID estimation and count reco particles
        if(pdg == 111){ 
          n_pi0["reco"]++;
          if(!fTPCGeo.IsContained(*reco_particles.at(j))) particles_contained["reco"] = false;
        }

        // Treat muons and pions as the same
        if(pdg == 13 || pdg == 211){
          // Check that particle is contained
          if(!fTPCGeo.IsContained(*reco_particles.at(j))) particles_contained["reco"] = false;

          double rand_pid = fRandom->Rndm();
          bool is_pion = rand_pid < fPionPidEff;
          if(fUseHistPid) is_pion = 1;
          // ID as pion, calculate leading pion variables
          if(is_pion){ 
            n_pipm["reco"]++;

            // Leading pion momentum and angle
            double reco_pipm1_mom = fRangeFitter.GetTrackMomentum(contained_length, 13);
            if(reco_pipm1_mom < pipm1_mom["reco"]) continue;
            // FIXME no idea how to estimate pion momentum
            pipm1_mom["reco"]   = reco_pipm1_mom;
            pipm1_theta["reco"] = (end - start).Theta();

            // Angle between leading pion and lepton candidate
            if(longest_j == -1) continue;
            TVector3 lep_end = fTPCGeo.CrossingPoints(*reco_particles.at(longest_j)).second;
            lep_pipm1_angle["reco"] = lep_end.Angle(end);
          }

          // ID as proton, calculate leading proton variables
          else{
            n_pr["reco"]++;

            // Leading proton momentum and angle
            double reco_pr1_mom = fRangeFitter.GetTrackMomentum(contained_length, 2212);
            if(reco_pr1_mom < pr1_mom["reco"]) continue;
            pr1_mom["reco"]   = reco_pr1_mom;
            pr1_theta["reco"] = (end - start).Theta();

            // Angle between leading proton and lepton candidate
            if(longest_j == -1) continue;
            TVector3 lep_end      = fTPCGeo.CrossingPoints(*reco_particles.at(longest_j)).second;
            lep_pr1_angle["reco"] = lep_end.Angle(end);

            // Transverse variables
            TVector3 scaled_lep_mom = reco_particles.at(longest_j)->Momentum().Vect()*(lep_mom["reco"]/reco_particles.at(longest_j)->P());
            TVector3 scaled_pr_mom  = reco_particles.at(j)->Momentum().Vect()*(reco_pr1_mom/reco_particles.at(j)->P());
            TVector3 reco_delta_pt  = DeltaPT(scaled_lep_mom, scaled_pr_mom);
            delta_pt["reco"]     = reco_delta_pt.Mag();
            delta_alphat["reco"] = reco_delta_pt.Theta()*TMath::RadToDeg();
            delta_phit["reco"]   = reco_delta_pt.Phi()*TMath::RadToDeg();
          }
        }

        if(pdg == 2212){
          // Check that particle is contained
          if(!fTPCGeo.IsContained(*reco_particles.at(j))) particles_contained["reco"] = false;

          double rand_pid = fRandom->Rndm();
          bool is_proton = rand_pid < fProtonPidEff;
          if(fUseHistPid) is_proton = ProtonHistPid(reco_particles.at(j)->P());
          // ID as proton, calculate leading proton variables
          if(is_proton){ 
            n_pr["reco"]++;

            // Leading proton momentum and angle
            double reco_pr1_mom = fRangeFitter.GetTrackMomentum(contained_length, 2212);
            if(reco_pr1_mom < pr1_mom["reco"]) continue;
            pr1_mom["reco"]   = reco_pr1_mom;
            pr1_theta["reco"] = (end - start).Theta();

            // Angle between leading proton and lepton candidate
            if(longest_j == -1) continue;
            TVector3 lep_end      = fTPCGeo.CrossingPoints(*reco_particles.at(longest_j)).second;
            lep_pr1_angle["reco"] = lep_end.Angle(end);

            // Transverse variables
            TVector3 scaled_lep_mom = reco_particles.at(longest_j)->Momentum().Vect()*(lep_mom["reco"]/reco_particles.at(longest_j)->P());
            TVector3 scaled_pr_mom  = reco_particles.at(j)->Momentum().Vect()*(reco_pr1_mom/reco_particles.at(j)->P());
            TVector3 reco_delta_pt  = DeltaPT(scaled_lep_mom, scaled_pr_mom);
            delta_pt["reco"]     = reco_delta_pt.Mag();
            delta_alphat["reco"] = reco_delta_pt.Theta()*TMath::RadToDeg();
            delta_phit["reco"]   = reco_delta_pt.Phi()*TMath::RadToDeg();
          }

          // ID as pion, calculate leading pion variables
          else{ 
            n_pipm["reco"]++;

            // Leading pion momentum and angle
            double reco_pipm1_mom = fRangeFitter.GetTrackMomentum(contained_length, 13);
            if(reco_pipm1_mom < pipm1_mom["reco"]) continue;
            pipm1_mom["reco"]   = reco_pipm1_mom;
            pipm1_theta["reco"] = (end - start).Theta();

            // Angle between leading pion and lepton candidate
            if(longest_j == -1) continue;
            TVector3 lep_end        = fTPCGeo.CrossingPoints(*reco_particles.at(longest_j)).second;
            lep_pipm1_angle["reco"] = lep_end.Angle(end);
          }
        }
      }
      
      if(fVerbose){

        // Loop over list of reco types and print everything
        for(unsigned int r = 0; r < fRecoFormats.size(); ++r)
          std::cout<<"-> "<<fRecoFormats[r]<<" cc:                   "<<cc[fRecoFormats[r]]<<"\n"
                   <<"-> "<<fRecoFormats[r]<<" nu_pdg:               "<<nu_pdg[fRecoFormats[r]]<<"\n"
                   <<"-> "<<fRecoFormats[r]<<" nu_energy:            "<<nu_energy[fRecoFormats[r]]<<"\n"
                   <<"-> "<<fRecoFormats[r]<<" int_type:             "<<int_type[fRecoFormats[r]]<<"\n"
                   <<"-> "<<fRecoFormats[r]<<" lep_contained:        "<<lep_contained[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" lep_mom:             "<<lep_mom[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" lep_theta:           "<<lep_theta[fRecoFormats[r]]<<"\n"
                   <<"->  "<<fRecoFormats[r]<<" particles_contained: "<<particles_contained[fRecoFormats[r]]<<"\n"
                   <<"-> "<<fRecoFormats[r]<<" n_pi0:                "<<n_pi0[fRecoFormats[r]]<<"\n"
                   <<"-> "<<fRecoFormats[r]<<" n_pipm:               "<<n_pipm[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" pipm1_mom:           "<<pipm1_mom[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" pipm1_theta:         "<<pipm1_theta[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" lep_pipm1_angle:     "<<lep_pipm1_angle[fRecoFormats[r]]<<"\n"
                   <<"-> "<<fRecoFormats[r]<<" n_pr:                 "<<n_pr[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" pr1_mom:             "<<pr1_mom[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" pr1_theta:           "<<pr1_theta[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" lep_pr1_angle:       "<<lep_pr1_angle[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" delta_pt:            "<<delta_pt[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" delta_alphat:        "<<delta_alphat[fRecoFormats[r]]<<"\n"
                   <<"--> "<<fRecoFormats[r]<<" delta_phit:          "<<delta_phit[fRecoFormats[r]]<<"\n"
                   <<"\n";
      }
      fXSecTree->Fill();
    }

  } // XSecTree::analyze()


  void XSecTree::endJob(){

  } // XSecTree::endJob()


  // Reset variables and counters
  void XSecTree::ResetVars(){

    lep_j = -1;
    longest_j = -1;

    vtx_x = -99999.;
    vtx_y = -99999.;
    vtx_z = -99999.;
    
    // Loop over reco formats and reset every variable in the tree
    for(unsigned int i = 0; i < fRecoFormats.size(); ++i){
      particles_contained[fRecoFormats[i]] = true;
      lep_contained[fRecoFormats[i]]       = false;
      cc[fRecoFormats[i]]                  = -1;
      nu_pdg[fRecoFormats[i]]              = -99999;
      int_type[fRecoFormats[i]]            = -99999;
      n_pipm[fRecoFormats[i]]              = 0;
      n_pi0[fRecoFormats[i]]               = 0;
      n_pr[fRecoFormats[i]]                = 0;
      nu_energy[fRecoFormats[i]]           = -99999;
      lep_mom[fRecoFormats[i]]             = -99999;
      lep_theta[fRecoFormats[i]]           = -99999;
      pr1_mom[fRecoFormats[i]]             = -99999;
      pr1_theta[fRecoFormats[i]]           = -99999;
      lep_pr1_angle[fRecoFormats[i]]       = -99999;
      pipm1_mom[fRecoFormats[i]]           = -99999;
      pipm1_theta[fRecoFormats[i]]         = -99999;
      lep_pipm1_angle[fRecoFormats[i]]     = -99999;
      delta_pt[fRecoFormats[i]]            = -99999;
      delta_alphat[fRecoFormats[i]]        = -99999;
      delta_phit[fRecoFormats[i]]          = -99999;
    }
  } // XSecTree::ResetVars

  void XSecTree::ResetWeights(){
    /*for(size_t i = 0; i < fWeightCalcs.size(); i++){
      weights[fWeightCalcs[i]].clear();
    }
    */
    for(size_t i = 0; i < 500; i++){
      weights[i] = 1.;
    }
  }
  
  // Give us a list of the stable, primary particles that we're interested in
  std::vector<const simb::MCParticle*> XSecTree::InterestingParticles(std::vector<const simb::MCParticle*> particles){

    std::vector<const simb::MCParticle*> interesting;

    // Loop over all of the particles
    for(size_t j = 0; j < particles.size(); j++){
      // Only consider stable final states particles
      if(particles.at(j)->StatusCode() != 1) continue;
      // Only want primary particles
      if(particles.at(j)->Mother() != 0) continue;

      // Only consider electrons, muons, pi0, charged pi and protons TODO for now...
      int pdg = std::abs(particles.at(j)->PdgCode());
      if(!(pdg == 11 || pdg == 13 || pdg == 111 || pdg == 211 || pdg == 2212)) continue;
      interesting.push_back(particles.at(j));
    }

    return interesting;

  } // XSecTree::InterestingParticles()


  // Apply reconstruction efficiencies to true particles
  std::vector<const simb::MCParticle*> XSecTree::RecoParticles(std::vector<const simb::MCParticle*> particles){

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
      if(pdg == 13){ 
        if(!fUseHistEfficiency){
          if(particles.at(j)->P() < fMuonThreshold || rand_eff > fMuonEff) continue;
        }
        else if(!MuonHistEff(particles.at(j)->P())) continue;
      }
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
      if(pdg == 211){
        if(!fUseHistEfficiency){
          if(particles.at(j)->P() < fPionThreshold || rand_eff > fPionEff) continue;
        }
        else if(!PionHistEff(particles.at(j)->P())) continue;
      }
      if(pdg == 2212){
        if(!fUseHistEfficiency){
          if(particles.at(j)->P() < fProtonThreshold || rand_eff > fProtonEff) continue;
        }
        else if(!ProtonHistEff(particles.at(j)->P())) continue;
      }

      reco_particles.push_back(particles.at(j));
    }

    return reco_particles;

  } // XSecTree::RecoParticles()

  // Smear electron momentum based on TRACS performance
  double XSecTree::SmearElectronMomentum(double momentum){
    //For contained muons use range based bias and resolution
    //Values from Fig 5 of https://arxiv.org/pdf/1703.06187.pdf
    double bias = -0.15;
    double resolution = 0.33;
    double momentum_smear = fRandom->Gaus(momentum, resolution*momentum)+bias*momentum;
    if(momentum_smear<0){momentum_smear = 0;}

    return momentum_smear;

  } // XSecTree::SmearElectronMomentum()

  // Smear momentum for exiting particles using MCS based method
  double XSecTree::SmearMcsMomentum(double momentum){
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

  } // XSecTree::SmearMcsMomentum()

  // Smear momentum for exiting particles using MCS based method
  double XSecTree::SmearMcsMomentumSbnd(double momentum, double length){
    //For exiting muons use multiple coulomb scattering bias and resolution
    double bias[] = {0.131486, 0.0170667, 0.0156267, 0.00236041, -0.00208193, -0.00393409, -0.0048733, -0.00505159, -0.00933451, -0.00548155};
    double resolution[] = { 0.328372, 0.212698, 0.164104, 0.129338, 0.116369, 0.107468, 0.0905116, 0.0873146, 0.0855313, 0.0919797};
    int pos = 9; 
    for(int i=0; i<10; i++){
      if(length<(40*(i+1))) {pos = i; break;}
    }
    double momentum_smear = fRandom->Gaus(momentum, resolution[pos]*momentum) + bias[pos]*momentum;
    if(momentum_smear<0) {momentum_smear = 0;}

    return momentum_smear;

  } // XSecTree::SmearMcsMomentumSbnd()


  bool XSecTree::MuonHistEff(double momentum){
    bool reconstructed = false;
    double efficiency[] = {0.0590501, 0.506081, 0.82274, 0.868142, 0.888397, 0.899807, 0.902781, 0.910641, 0.916731, 0.94};
    int pos = 9; 
    for(int i=0; i<10; i++){
      if(momentum<(0.1*(i+1))) {pos = i; break;}
    }
    double rand_eff = fRandom->Rndm();
    if(rand_eff < efficiency[pos]) reconstructed = true;

    return reconstructed;

  } // XSecTree::MuonHistEff() 

  bool XSecTree::PionHistEff(double momentum){
    bool reconstructed = false;
    double efficiency[] = {0.0364407, 0.137405, 0.374127, 0.49142, 0.568833, 0.588972, 0.616043, 0.626923, 0.632184, 0.65};
    int pos = 9; 
    for(int i=0; i<10; i++){
      if(momentum<(0.05*(i+1))) {pos = i; break;}
    }
    double rand_eff = fRandom->Rndm();
    if(rand_eff < efficiency[pos]) reconstructed = true;

    return reconstructed;

  } // XSecTree:PionHistEff()

  bool XSecTree::ProtonHistEff(double momentum){
    bool reconstructed = false;
    double efficiency[] = {0.000135655, 0.000968523, 0.00599737, 0.0423821, 0.106337, 0.198591, 0.345482, 0.504317, 0.632799, 0.75};
    int pos = 9; 
    for(int i=0; i<10; i++){
      if(momentum<(0.06*(i+1))) {pos = i; break;}
    }
    double rand_eff = fRandom->Rndm();
    if(rand_eff < efficiency[pos]) reconstructed = true;

    return reconstructed;

  } // XSecTree:ProtonHistEff()


  // Smear momentum for contained particles using range based method
  double XSecTree::SmearRangeMomentum(double momentum){
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

  } // XSecTree::SmearRangeMomentum()


  // Calculate the visible energy as neutrino energy estimator
  // TODO poorly copied from Gray's stuff, will likely change anyway
  double XSecTree::VisibleEnergy(std::vector<const simb::MCParticle*> particles){
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

  } // XSecTree::VisibleEnergy()


  // Calculate transverse variables (https://link.aps.org/accepted/10.1103/PhysRevC.94.015503)
  TVector3 XSecTree::DeltaPT(TVector3 mu_mom, TVector3 pr_mom){
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

  } // XSecTree::DeltaPT()


  // Inclusive charged current selection
  bool XSecTree::IsCCInc(geo::Point_t vertex, std::vector<const simb::MCParticle*> reco_particles){
    bool cc_selected = true;
    // Check vertex is inside the fiducial volume
    //if(!fTPCGeo.InFiducial(vertex, fWallCut, fWallCut, fWallCut, fWallCut, fWallCut, fBackCut)){ 
    if(!fTPCGeo.InFiducial(vertex, fXminCut, fYminCut, fZminCut, fXmaxCut, fYmaxCut, fZmaxCut, fCpaCut, fApaCut)){ 
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
      if(!fUseHistPid){
        if(contained_length < max_contained_length) continue;
      }
      else{
        if(!(MuonHistPid(reco_particles.at(j)->P(), pdg) && contained_length > max_contained_length)) continue;
      }
      max_contained_length = contained_length;
      max_pdg              = pdg;
      longest_j            = j;

      // Check if particle is contained
      lep_contained["reco"] = fTPCGeo.IsContained(*reco_particles.at(j));

      std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(j));
      TVector3 start                             = cross_points.first;
      TVector3 end                               = cross_points.second;
      lep_theta["reco"]                          = (end - start).Theta();

      // Smear momentum based on whether particle is contained or not
      if(lep_contained["reco"]) 
        lep_mom["reco"] = fRangeFitter.GetTrackMomentum(contained_length, 13);
      else{ 
        if(!fUseSbndSmearing) lep_mom["reco"] = SmearMcsMomentum(reco_particles.at(j)->P());
        else lep_mom["reco"] = SmearMcsMomentumSbnd(reco_particles.at(j)->P(), contained_length);
      }
    }

    if(fVerbose) std::cout<<"max contained length = "<<max_contained_length<<" pdg = "<<max_pdg<<"\n";

    // Check length of particle
    if(lep_contained["reco"] && max_contained_length < fMinContainedLength){ 
      cc_selected = false;
      if(fVerbose) std::cout<<"contained length too short\n";
    }
    if(!lep_contained["reco"] && max_contained_length < fMinExitingLength){ 
      cc_selected = false;
      if(fVerbose) std::cout<<"exiting length too short\n";
    }

    return cc_selected;

  }

  bool XSecTree::MuonHistPid(double momentum, int pdg){
    bool muonID = false;
    double efficiencyMu[] = {0, 0.0157303, 0.618693, 0.716556, 0.822193, 0.965766, 0.988912, 0.991038, 0.992492, 0.994742, 0.998403, 0.993258, 0.996441, 1, 1, 1, 1, 1, 1, 1};
    double efficiencyPi[] = {0, 0, 0.279539, 0.432954, 0.290393, 0.227886, 0.244541, 0.288079, 0.291304, 0.347518, 0.353535, 0.380282, 0.37931, 0.409091, 0.268293, 0.5, 0.5, 0.473684, 0.466667, 0.294118};
    double efficiencyP[] = {0,0, 0, 0, 0, 0, 0.000254453, 0.00367816, 0.014854, 0.0454402, 0.0460772, 0.0612366, 0.0734949, 0.0933041, 0.121581, 0.150794, 0.216912, 0.270115, 0.316239, 0.277778};
    int pos = 19; 
    for(int i=0; i<20; i++){
      if(momentum<(0.075*(i+1))) {pos = i; break;}
    }
    double rand_eff = fRandom->Rndm();
    if(std::abs(pdg) == 13 && rand_eff < efficiencyMu[pos]) muonID = true;
    if(std::abs(pdg) == 211 && rand_eff < efficiencyPi[pos]) muonID = true;
    if(std::abs(pdg) == 2212 && rand_eff < efficiencyP[pos]) muonID = true;

    return muonID;
  }

  bool XSecTree::ProtonHistPid(double momentum){
    bool protonID = false;
    double efficiency[] = {0, 0.111111, 0.23166, 0.533176, 0.693913, 0.675789, 0.649212, 0.596035, 0.502117, 0.397896, 0.278592, 0.229983, 0.130303, 0.136364, 0.088, 0.0833333, 0.0344828, 0.03125, 0.125, 0.0769231};
    int pos = 19; 
    for(int i=0; i<20; i++){
      if(momentum<(0.1*(i+1))) {pos = i; break;}
    }
    double rand_eff = fRandom->Rndm();
    if(rand_eff < efficiency[pos]) protonID = true;

    return protonID;
  }

  DEFINE_ART_MODULE(XSecTree)
} // namespace sbnd
