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
#include "TH2.h"
#include "TH1.h"
#include "TFile.h"
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
    void ResetDetSyst();

    // Give us a list of the stable, primary particles that we're interested in
    std::vector<const simb::MCParticle*> InterestingParticles(std::vector<const simb::MCParticle*> particles);

    // Apply reconstruction efficiencies to true particles
    std::vector<const simb::MCParticle*> RecoParticles(std::vector<const simb::MCParticle*> particles, double muerr=0, double pierr=0, double perr=0);

    // Smear momentum for electrons from TRACS performance
    double SmearElectronMomentum(double momentum);

    // Smear momentum for exiting particles using MCS based method
    double SmearMcsMomentum(double momentum);

    // Smear MCS momentum using SBND simulation
    double SmearMcsMomentumSbnd(double momentum, double length);
    double SmearMcsMomentumHist(double momentum, double length, double err=0);

    // Use histograms for efficiencies
    bool MuonHistEff(double length, double theta, double err=0);
    bool PionHistEff(double length, double theta, double err=0);
    bool ProtonHistEff(double length, double theta, double err=0);

    // Smear momentum for contained particles using range based method
    double SmearRangeMomentum(double momentum);

    // Calculate the visible energy as neutrino energy estimator
    double VisibleEnergy(std::vector<const simb::MCParticle*> particles, int lep_j);

    // Calculate transverse variables (https://link.aps.org/accepted/10.1103/PhysRevC.94.015503)
    TVector3 DeltaPT(TVector3 mu_mom, TVector3 pr_mom);

    // Inclusive charged current selection
    std::pair<bool,int> IsCCInc(std::vector<const simb::MCParticle*> reco_particles, double ppid=0, double mupid=0);

    // Use histograms for PID
    bool MuonHistPid(double momentum, int pdg, double err=0);
    bool ProtonHistPid(double momentum, double err=0);

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
    std::vector<std::string> fGenieWeightCalcs;
    std::vector<std::string> fFluxWeightCalcs;
    

    TPCGeoAlg fTPCGeo;
    trkf::TrackMomentumCalculator fRangeFitter;

    evwgh::WeightManager _wgt_manager;

    TRandom2 *fRandom;

    // "Random" uncertainties for detector systematics, need to be consistent over universes
    double fMueff[50] = { 0.0024,-0.0285,-0.0168,0.0108,-0.0432,0.0285,0.0012,-0.0440,0.0052,-0.0121,-0.0498,0.0126,0.0092,0.0487,-0.0078,0.0377,-0.0178,0.0070,0.0488,-0.0057,0.0606,-0.0190,0.0262,-0.0063,-0.0550,0.0177,-0.0325,-0.0200,-0.0567,0.0673,-0.0378,0.0197,-0.0147,-0.0037,-0.0420,-0.0210,0.0733,0.0442,-0.0226,0.1051,-0.0227,0.0398,-0.0568,-0.0285,0.0003,-0.0145,-0.0746,0.0168,-0.0195,-0.0967 };
    double fPieff[50] = { 0.1248,-0.0160,0.0138,-0.0712,0.0118,0.0594,-0.0681,0.0236,0.0378,-0.0487,-0.0094,-0.1085,0.0927,0.0832,-0.0209,-0.0224,-0.0478,-0.0360,-0.0612,0.0687,0.0294,-0.0034,0.0271,-0.0613,0.0943,-0.0160,-0.0351,0.0103,0.0456,0.0487,0.0293,-0.0165,0.0675,-0.0926,-0.0080,-0.0135,-0.0009,-0.0434,-0.0272,0.0455,0.0918,-0.0451,0.0581,0.0881,-0.0736,0.0294,-0.1525,0.0599,0.0052,0.0587 };
    double fPeff[50] = { -0.0268,-0.0096,0.0281,-0.0131,-0.0513,-0.0149,0.0085,-0.0743,0.0829,-0.0621,0.0078,0.0272,0.0160,-0.0047,0.1336,0.0099,0.0517,-0.0004,0.0642,-0.0710,-0.0234,-0.0058,0.0126,-0.0519,-0.0320,-0.0260,-0.0072,-0.0729,0.0467,-0.0855,-0.0268,-0.0282,0.0922,0.0082,-0.0330,-0.0832,-0.0181,-0.0393,0.0205,0.0511,-0.0232,-0.0110,0.0173,-0.0278,-0.0079,-0.0509,0.0101,-0.0714,-0.0247,-0.0786 };
    double fPpid[50] = { 0.0468,0.0209,0.0660,-0.0251,-0.0154,-0.0965,0.1025,0.0610,0.0150,-0.0071,0.0606,-0.0001,0.0310,0.0640,-0.0186,0.0297,-0.0415,0.0596,0.0723,-0.0288,-0.0141,0.0857,-0.0558,0.0459,0.0659,-0.0107,0.0644,0.0261,-0.0003,-0.0516,-0.0609,-0.0603,0.0119,0.0697,-0.0024,0.0095,-0.0004,0.0125,0.0407,0.0186,-0.0264,0.0136,0.0242,-0.0812,0.0535,-0.0374,-0.0652,-0.0608,0.0148,-0.1042 };
    double fMupid[50] = { 0.0691,0.0280,-0.0278,-0.0014,-0.0674,-0.0137,-0.0017,-0.0071,0.0168,0.0075,-0.1026,0.0064,0.0437,-0.0646,0.0406,-0.0117,-0.0190,-0.0211,0.0045,-0.0100,-0.0346,0.0465,0.0473,0.0190,0.0187,0.0634,-0.0490,-0.0249,0.0203,0.0430,-0.0179,-0.0260,-0.0461,-0.0155,-0.0274,0.0822,0.0375,-0.0127,-0.0385,0.0162,0.0296,0.0049,0.0046,0.0182,-0.0341,-0.0099,-0.0786,-0.0062,0.0834,-0.0196 };
    double fMomres[50] = { 0.0261,0.0202,0.0001,0.1030,0.0353,0.0307,0.0639,-0.0309,-0.0447,0.0363,-0.0377,0.0514,-0.0162,-0.0482,0.0588,0.0450,-0.0785,-0.1456,-0.0394,-0.0409,-0.0283,-0.0274,-0.0291,0.0145,0.0510,0.0332,0.0257,-0.0703,-0.0129,0.0088,0.0614,-0.0226,0.0233,-0.0132,0.0229,0.0233,-0.0372,0.0469,-0.0063,-0.0161,0.0239,-0.0442,-0.0013,-0.0287,0.0436,-0.0057,0.0532,0.0037,0.0578,-0.0484 }; 

    // Global variables
    int lep_j;
    int longest_j;

    // Tree
    TTree *fXSecTree;
    TTree *fMetaDataTree;
    TTree *fWeightTree;
    TTree *fDetSystTree;

    // Reconstruction histograms
    TH2D* hMuRecoEff;
    TH2D* hPiRecoEff;
    TH2D* hPRecoEff;
    TH1D* hProtonId;
    TH1D* hMuMuId;
    TH1D* hPiMuId;
    TH1D* hPMuId;
    TH1D* hMcsMomBias;
    TH1D* hMcsMomRes;

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
    std::map<std::string, double> lep_phi;
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
    double genie_weights[100];
    double flux_weights[100];

    // Detector systematic tree parameters
    double ds_vtx_x;
    double ds_vtx_y;
    double ds_vtx_z;
    bool ds_particles_contained[50];
    bool ds_lep_contained[50];
    int ds_cc[50];
    int ds_nu_pdg[50];
    double ds_lep_mom[50];
    double ds_lep_theta[50];
    double ds_lep_phi[50];
    double ds_vise[50];
    unsigned int ds_ntracks[50];

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

    _wgt_manager.Configure(p, *this);

    
    fGenieWeightCalcs = {
    "genie_all_Genie"};

    fFluxWeightCalcs = {
    /*"bnbcorrection_FluxHist",
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
    "piontotxsec_FluxUnisim",*/
    "piplus_PrimaryHadronSWCentralSplineVariation"
    };

    // Get histograms
    cet::search_path sp("FW_SEARCH_PATH");
    std::string effname;
    sp.find_file("PhysicsBook/EfficiencyHists.root", effname);
    TFile *efffile = new TFile(effname.c_str(), "READ");
    hMuRecoEff = (TH2D*)efffile->Get("hLengthThetaReco_#mu");
    hPiRecoEff = (TH2D*)efffile->Get("hLengthThetaReco_#pi");
    hPRecoEff = (TH2D*)efffile->Get("hLengthThetaReco_p");
    efffile->Close();
    std::string selname;
    sp.find_file("PhysicsBook/SelectionHists.root", selname);
    TFile *selfile = new TFile(selname.c_str(), "READ");
    hProtonId = (TH1D*)selfile->Get("hMinPChi2p");
    hMuMuId = (TH1D*)selfile->Get("hMomNoPLenSel#mu");
    hPiMuId = (TH1D*)selfile->Get("hMomNoPLenSel#pi");
    hPMuId = (TH1D*)selfile->Get("hMomNoPLenSelp");
    selfile->Close();
    std::string smearname;
    sp.find_file("PhysicsBook/SmearingHists.root", smearname);
    TFile *smearfile = new TFile(smearname.c_str(), "READ");
    hMcsMomBias = (TH1D*)smearfile->Get("hBiasmu_mcs_mom_length");
    hMcsMomRes = (TH1D*)smearfile->Get("hResmu_mcs_mom_length");
    smearfile->Close();

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
      fXSecTree->Branch((fRecoFormats[i]+"_lep_phi").c_str(), &lep_phi[fRecoFormats[i]]);
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

    fWeightTree = tfs->make<TTree>("weight", "xsec tree");
    fWeightTree->Branch("genie_weights", &genie_weights, "genie_weights[100]/D");
    fWeightTree->Branch("flux_weights", &flux_weights, "flux_weights[100]/D");

    fDetSystTree = tfs->make<TTree>("detsyst", "xsec tree");
    fDetSystTree->Branch("ds_vtx_x", &ds_vtx_x);
    fDetSystTree->Branch("ds_vtx_y", &ds_vtx_y);
    fDetSystTree->Branch("ds_vtx_z", &ds_vtx_z);
    fDetSystTree->Branch("ds_particles_contained", &ds_particles_contained, "ds_particles_contained[50]/O");
    fDetSystTree->Branch("ds_lep_contained", &ds_lep_contained, "ds_lep_contained[50]/O");
    fDetSystTree->Branch("ds_cc", &ds_cc, "ds_cc[50]/I");
    fDetSystTree->Branch("ds_nu_pdg", &ds_nu_pdg, "ds_nu_pdg[50]/I");
    fDetSystTree->Branch("ds_lep_mom", &ds_lep_mom, "ds_lep_mom[50]/D");
    fDetSystTree->Branch("ds_lep_theta", &ds_lep_theta, "ds_lep_theta[50]/D");
    fDetSystTree->Branch("ds_lep_phi", &ds_lep_phi, "ds_lep_phi[50]/D");
    fDetSystTree->Branch("ds_vise", &ds_vise, "ds_vise[50]/D");
    fDetSystTree->Branch("ds_ntracks", &ds_ntracks, "ds_ntracks[50]/i");
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
    /*if(fluxWeightList.size() == mctruthList.size() && genieWeightList.size() == mctruthList.size()){
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
          if(kv.second.size()<100) continue;
          for(size_t i = 0; i < 100; i++){
            flux_weights[i] *= kv.second[i];
          }
        }
        for(auto const &kv : genieWeightList[inu]->fWeight){
          std::cout<<"Name = "<<kv.first<<" vector size = "<<kv.second.size()<<"\n";
          if(kv.second.size()<100) continue;
          for(size_t i = 0; i < 100; i++){
            genie_weights[i] *= kv.second[i];
          }
        }
        fWeightTree->Fill();
      }
    }
    else{*/
      std::cout<<"Weights not found in file, calculating...\n";
      
      // Implementation of required member function here.
      for (unsigned int inu = 0; inu < mctruthList.size(); ++inu) {
        ResetWeights();

        // This tree needs to be the same size at the interaction tree!
        // Get the pointer to the MCTruth object
        art::Ptr<simb::MCTruth> mctruth = mctruthList.at(inu);
        // Check the interaction is within the TPC
        geo::Point_t vertex {mctruth->GetNeutrino().Nu().Vx(), 
                             mctruth->GetNeutrino().Nu().Vy(), 
                             mctruth->GetNeutrino().Nu().Vz()};
        if(!fTPCGeo.InFiducial(vertex, 0.)) continue;

        // Copy event to non const type FIXME very naughty
        art::Event& e = const_cast<art::Event&>(event);
        evwgh::MCEventWeight mcwgh = _wgt_manager.Run(e, inu);
        std::cout<<"Number of weights = "<<mcwgh.fWeight.size()<<"\n";
        for(auto const &kv : mcwgh.fWeight){
          //std::cout<<"Name = "<<kv.first<<" vector size = "<<kv.second.size()<<"\n";
          if(std::find(fGenieWeightCalcs.begin(), fGenieWeightCalcs.end(), kv.first) != fGenieWeightCalcs.end()){
            for(size_t i = 0; i < kv.second.size(); i++){
              if(i >= 100) continue;
              genie_weights[i] *= kv.second[i];
            }
          }
          if(std::find(fFluxWeightCalcs.begin(), fFluxWeightCalcs.end(), kv.first) != fFluxWeightCalcs.end()){
            std::cout<<"Name = "<<kv.first<<" vector size = "<<kv.second.size()<<"\n";
            for(size_t i = 0; i < kv.second.size(); i++){
              if(i >= 100) continue;
              flux_weights[i] *= kv.second[i];
            }
          }
        }
        fWeightTree->Fill();
      }
    //}

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
        lep_phi["true"]       = (end - start).Phi();
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
      nu_energy["smeareff"] = VisibleEnergy(reco_particles, lep_j);
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
          lep_phi["eff"]                    = (end - start).Phi();
          lep_contained["eff"]              = fTPCGeo.IsContained(*reco_particles.at(j));
          lep_theta["smeareff"]             = lep_theta["eff"];
          lep_phi["smeareff"]               = lep_phi["eff"];
          lep_contained["smeareff"]         = lep_contained["eff"];


          if(pdg == 11){
            // When not smearing, just take true momentum
            lep_mom["eff"]      = reco_particles.at(j)->P();
            lep_mom["smeareff"] = SmearElectronMomentum(reco_particles.at(j)->P());
          }
          else if(lep_contained["smeareff"])
            //lep_mom["smeareff"] = fRangeFitter.GetTrackMomentum(contained_length, 13);
            lep_mom["smeareff"] = SmearRangeMomentum(reco_particles.at(j)->P());
          else{ 
            if(!fUseSbndSmearing) lep_mom["smeareff"] = SmearMcsMomentum(reco_particles.at(j)->P());
            //else lep_mom["smeareff"] = SmearMcsMomentumSbnd(reco_particles.at(j)->P(), contained_length);
            else lep_mom["smeareff"] = SmearMcsMomentumHist(reco_particles.at(j)->P(), contained_length);
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


      // Random probability of being rejected by cosmic ID cuts
      bool cosmic_id = false;
      if(std::abs(nu_pdg["true"]) == 14 && cc["true"] == 1){
        if(fRandom->Rndm() < 0.1) cosmic_id = true;
      }
      else if (fRandom->Rndm() < 0.15) cosmic_id = true;

      std::pair<bool, int> cc_selected = IsCCInc(reco_particles); 
      
      if(cc_selected.first && !cosmic_id){ 
        if(fVerbose) std::cout<<"Selected as CC\n";
        cc["reco"]     = 1;
        nu_pdg["reco"] = 14;
      }
      else{
        cc["reco"] = -1;
      }

      //------------------------------------ LEPTON KINEMATICS ------------------------------------------
      longest_j = cc_selected.second;

      // Same as smeared neutrino energy
      nu_energy["reco"] = VisibleEnergy(reco_particles, longest_j);

      if(longest_j != -1){
        // Check if particle is contained
        double contained_length = fTPCGeo.TpcLength(*reco_particles.at(longest_j));
        std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(longest_j));
        TVector3 start                             = cross_points.first;
        TVector3 end                               = cross_points.second;
        lep_contained["reco"] = fTPCGeo.IsContained(*reco_particles.at(longest_j));
        lep_theta["reco"] = (end - start).Theta();
        lep_phi["reco"] = (end - start).Phi();

        // Smear momentum based on whether particle is contained or not
        if(lep_contained["reco"]) 
          //lep_mom["reco"] = fRangeFitter.GetTrackMomentum(contained_length, 13);
          lep_mom["reco"] = SmearRangeMomentum(reco_particles.at(longest_j)->P());
        else{ 
          if(!fUseSbndSmearing) lep_mom["reco"] = SmearMcsMomentum(reco_particles.at(longest_j)->P());
          else lep_mom["reco"] = SmearMcsMomentumHist(reco_particles.at(longest_j)->P(), contained_length);
        }
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

      // ----------------------------------------- DETECTOR SYSTEMATICS ----------------------------------------
      // Reset the detector systematic tree
      ResetDetSyst();
      ds_vtx_x = vtx_x;
      ds_vtx_y = vtx_y;
      ds_vtx_z = vtx_z;
      // Loop over the number of different universes
      for(size_t u = 0; u < 50; u++){
        // Get the changes in efficiency for this universe
        // Reconstruct particles with changes to efficiencies
        std::vector<const simb::MCParticle*> ds_reco_particles = RecoParticles(particles, fMueff[u], fPieff[u], fPeff[u]);
        // Perform the CC inclusive selection with changes to efficiencies
        std::pair<bool, int> ds_cc_selected = IsCCInc(ds_reco_particles, fPpid[u], fMupid[u]); 
        
        if(ds_cc_selected.first && !cosmic_id){ 
          ds_cc[u]     = 1;
          ds_nu_pdg[u] = 14;
        }
        else{ 
          ds_cc[u] = -1;
        }

        int ds_j = ds_cc_selected.second;
        ds_vise[u] = VisibleEnergy(ds_reco_particles, ds_j);

        if(ds_j != -1){
          // Check if particle is contained
          double ds_contained_length = fTPCGeo.TpcLength(*ds_reco_particles.at(ds_j));
          std::pair<TVector3, TVector3> ds_cross_points = fTPCGeo.CrossingPoints(*ds_reco_particles.at(ds_j));
          TVector3 ds_start = ds_cross_points.first;
          TVector3 ds_end = ds_cross_points.second;
          ds_lep_contained[u] = fTPCGeo.IsContained(*ds_reco_particles.at(ds_j));
          ds_lep_theta[u] = (ds_end - ds_start).Theta();
          ds_lep_phi[u] = (ds_end - ds_start).Phi();

          // Smear variables with changes to efficiencies
          // Fill the reconstructed variables for this universe
          if(ds_lep_contained[u]) 
            //ds_lep_mom[u] = fRangeFitter.GetTrackMomentum(ds_contained_length, 13);
            ds_lep_mom[u] = SmearRangeMomentum(ds_reco_particles.at(ds_j)->P());
          else{ 
            ds_lep_mom[u] = SmearMcsMomentumHist(ds_reco_particles.at(ds_j)->P(), ds_contained_length, fMomres[u]);
          }
        }

        for(size_t dj = 0; dj < ds_reco_particles.size(); dj++){
          // Don't look at the particle selected as the muon
          if(ds_j == (int)dj) continue;
          int pdg = std::abs(ds_reco_particles.at(dj)->PdgCode());
          if(pdg == 111 || pdg == 13 || pdg == 211 || pdg == 2212){ 
            if(!fTPCGeo.IsContained(*ds_reco_particles.at(dj))) ds_particles_contained[u] = false;
          }
          if(pdg == 111 || pdg == 13 || pdg == 211 || pdg == 2212){ 
            ds_ntracks[u] += 1;
          }
        }
      }
      // Fill the tree
      fDetSystTree->Fill();
     
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
      lep_phi[fRecoFormats[i]]             = -99999;
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
    for(size_t i = 0; i < 100; i++){
      genie_weights[i] = 1.;
      flux_weights[i] = 1.;
    }
  }

  void XSecTree::ResetDetSyst(){
    ds_vtx_x = -99999.;
    ds_vtx_y = -99999.;
    ds_vtx_z = -99999.;
    for(size_t i = 0; i < 50; i++){
      ds_particles_contained[i] = true;
      ds_lep_contained[i]       = false;
      ds_cc[i]                  = -1;
      ds_nu_pdg[i]              = 0;
      ds_lep_mom[i]             = -99999;
      ds_lep_theta[i]           = -99999;
      ds_lep_phi[i]             = -99999;
      ds_vise[i]                = -99999;
      ds_ntracks[i]             = 0;
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
  std::vector<const simb::MCParticle*> XSecTree::RecoParticles(std::vector<const simb::MCParticle*> particles, double muerr, double pierr, double perr){

    std::vector<const simb::MCParticle*> reco_particles;

    for(size_t j = 0; j < particles.size(); j++){
      // Only consider stable final states particles
      if(particles.at(j)->StatusCode() != 1) continue;
      // Only want primary particles
      if(particles.at(j)->Mother() != 0) continue;

      int pdg = std::abs(particles.at(j)->PdgCode());
      if(!(pdg == 11 || pdg == 13 || pdg == 111 || pdg == 211 || pdg == 2212)) continue;

      double length = fTPCGeo.TpcLength(*particles.at(j));
      std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*particles.at(j));
      TVector3 start                             = cross_points.first;
      TVector3 end                               = cross_points.second;
      double theta = (end-start).Theta();

      // Apply efficiency cut (energy thresholds + flat efficiency)
      double rand_eff = fRandom->Rndm();
      if(pdg == 11 && (particles.at(j)->P() < fElectronThreshold || rand_eff > fElectronEff)) continue;
      if(pdg == 13){ 
        if(!fUseHistEfficiency){
          if(particles.at(j)->P() < fMuonThreshold || rand_eff > fMuonEff) continue;
        }
        else if(!MuonHistEff(length, theta, muerr)) continue;
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
        else if(!PionHistEff(length, theta, pierr)) continue;
      }
      if(pdg == 2212){
        if(!fUseHistEfficiency){
          if(particles.at(j)->P() < fProtonThreshold || rand_eff > fProtonEff) continue;
        }
        else if(!ProtonHistEff(length, theta, perr)) continue;
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

  // Smear momentum for exiting particles using MCS based method
  double XSecTree::SmearMcsMomentumHist(double momentum, double length, double err){
    //For exiting muons use multiple coulomb scattering bias and resolution
    int bbin = hMcsMomBias->GetXaxis()->FindBin(length);
    if(bbin == hMcsMomBias->GetNbinsX() + 1) bbin = hMcsMomBias->GetNbinsX();
    double bias = hMcsMomBias->GetBinContent(bbin);
    int rbin = hMcsMomRes->GetXaxis()->FindBin(length);
    if(rbin == hMcsMomRes->GetNbinsX() + 1) rbin = hMcsMomRes->GetNbinsX();
    double resolution = hMcsMomRes->GetBinContent(rbin)*(1.+err);

    double momentum_smear = fRandom->Gaus(momentum, resolution*momentum) + bias*momentum;
    if(momentum_smear<0) {momentum_smear = 0;}

    return momentum_smear;

  } // XSecTree::SmearMcsMomentumSbnd()

  bool XSecTree::MuonHistEff(double length, double theta, double err){
    bool reconstructed = false;
    int lbin = hMuRecoEff->GetXaxis()->FindBin(length);
    if(lbin == hMuRecoEff->GetNbinsX() + 1) lbin = hMuRecoEff->GetNbinsX();
    int tbin = hMuRecoEff->GetYaxis()->FindBin(length);
    if(tbin == hMuRecoEff->GetNbinsY() + 1) tbin = hMuRecoEff->GetNbinsY();

    double efficiency = hMuRecoEff->GetBinContent(lbin, tbin) + err;
    if((hMuRecoEff->GetBinContent(lbin, tbin) <= 0 && length > 50.) || efficiency > 1.) efficiency = 1.;
    double rand_eff = fRandom->Rndm();
    if(rand_eff < efficiency) reconstructed = true;

    return reconstructed;

  }

  bool XSecTree::PionHistEff(double length, double theta, double err){
    bool reconstructed = false;
    int lbin = hPiRecoEff->GetXaxis()->FindBin(length);
    if(lbin == hPiRecoEff->GetNbinsX() + 1) lbin = hPiRecoEff->GetNbinsX();
    int tbin = hPiRecoEff->GetYaxis()->FindBin(length);
    if(tbin == hPiRecoEff->GetNbinsY() + 1) tbin = hPiRecoEff->GetNbinsY();

    double efficiency = hPiRecoEff->GetBinContent(lbin, tbin) + err;
    if((hPiRecoEff->GetBinContent(lbin, tbin) <= 0 && length > 50.) || efficiency > 1.) efficiency = 1.;
    double rand_eff = fRandom->Rndm();
    if(rand_eff < efficiency) reconstructed = true;

    return reconstructed;

  } // XSecTree::PionHistEff()

  bool XSecTree::ProtonHistEff(double length, double theta, double err){
    bool reconstructed = false;
    int lbin = hPRecoEff->GetXaxis()->FindBin(length);
    if(lbin == hPRecoEff->GetNbinsX() + 1) lbin = hPRecoEff->GetNbinsX();
    int tbin = hPRecoEff->GetYaxis()->FindBin(length);
    if(tbin == hPRecoEff->GetNbinsY() + 1) tbin = hPRecoEff->GetNbinsY();

    double efficiency = hPRecoEff->GetBinContent(lbin, tbin) + err;
    if((hPRecoEff->GetBinContent(lbin, tbin) <= 0 && length > 50.) || efficiency > 1) efficiency = 1.;
    double rand_eff = fRandom->Rndm();
    if(rand_eff < efficiency) reconstructed = true;

    return reconstructed;

  } // XSecTree::ProtonHistEff()


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
  double XSecTree::VisibleEnergy(std::vector<const simb::MCParticle*> particles, int lep_j){
    //
    double visible_E = 0;

    for(size_t j = 0; j < particles.size(); j++){
      if((int)j == lep_j);
      // Only consider stable final states particles
      if(particles.at(j)->StatusCode() != 1) continue;
      // Only want primary particles
      if(particles.at(j)->Mother() != 0) continue;

      int pdg = std::abs(particles.at(j)->PdgCode());
      if(pdg == 111) continue;

      double edep = fTPCGeo.EDep(*particles.at(j));
      if (edep > 0.021) visible_E += edep;
/*
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
      */
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
  std::pair<bool, int> XSecTree::IsCCInc(std::vector<const simb::MCParticle*> reco_particles, double ppid, double mupid){
    bool cc_selected = true;

    // Loop over the mu/pi/pr secondary particles and find the longest
    double max_contained_length = 0;
    int n_escape = 0;
    double longest_escape = 0;
    int long_j = -1;
    for(size_t j = 0; j < reco_particles.size(); j++){
      // Only consider track-like particles
      int pdg = std::abs(reco_particles.at(j)->PdgCode());
      if(!(pdg == 13 || pdg == 211 || pdg == 2212)) continue;

      double contained_length = fTPCGeo.TpcLength(*reco_particles.at(j));
      std::pair<TVector3, TVector3> cross_points = fTPCGeo.CrossingPoints(*reco_particles.at(j));
      TVector3 start                             = cross_points.first;
      TVector3 end                               = cross_points.second;
      geo::Point_t pend = {end.X(), end.Y(), end.Z()};
      bool escapes = false;
      // Exiting particles
      if(!fTPCGeo.InFiducial(pend, 1.5)){
        n_escape++;
        escapes = true;
        if(contained_length > longest_escape){
          longest_escape = contained_length;
        }
      }
      // Contained particles
      if(!escapes && contained_length < 100.){
        // Proton ID
        if(pdg == 2212 && ProtonHistPid(reco_particles.at(j)->P(), ppid)) continue;
        // Minimum length cut
        if(contained_length < 25.) continue;
        // All other cuts combined and parametrized in momentum
        if(!(MuonHistPid(reco_particles.at(j)->P(), pdg, mupid))) continue;
      }
      if(contained_length < max_contained_length) continue;

      max_contained_length = contained_length;
      long_j            = j;
    }

    if(n_escape == 1){
      if(longest_escape == max_contained_length && longest_escape >= fMinExitingLength) cc_selected = true;
      else cc_selected = false;
    }
    else if(n_escape == 0){
      if(max_contained_length >= fMinContainedLength) cc_selected = true;
      else cc_selected = false;
    }
    else{
      cc_selected = false;
    }

    return std::make_pair(cc_selected, long_j);

  }

  bool XSecTree::MuonHistPid(double momentum, int pdg, double err){
    bool muonID = false;
    int bin = hMuMuId->GetXaxis()->FindBin(momentum);
    if(bin == hMuMuId->GetNbinsX()+1) bin = hMuMuId->GetNbinsX();

    double efficiency = 0;
    // TODO cheating...
    if(std::abs(pdg) == 13) efficiency = hMuMuId->GetBinContent(bin) + err;
    if(std::abs(pdg) == 211) efficiency = hPiMuId->GetBinContent(bin) + 3.*err;
    if(std::abs(pdg) == 2212) efficiency = hPMuId->GetBinContent(bin) + 2.*err;
    if(efficiency < 0) efficiency = 0;
    if(efficiency > 1) efficiency = 1;

    double rand_eff = fRandom->Rndm();
    if(rand_eff < efficiency) muonID = true;

    return muonID;
  }

  bool XSecTree::ProtonHistPid(double momentum, double err){
    bool protonID = false;
    int bin = hProtonId->GetXaxis()->FindBin(momentum);
    if(bin == hProtonId->GetNbinsX()+1) bin = hProtonId->GetNbinsX();

    double efficiency = hProtonId->GetBinContent(bin) + err;
    if(efficiency < 0) efficiency = 0.;
    if(efficiency > 1) efficiency = 1.;
    double rand_eff = fRandom->Rndm();
    if(rand_eff < efficiency) protonID = true;

    return protonID;
  }

  DEFINE_ART_MODULE(XSecTree)
} // namespace sbnd
