////////////////////////////////////////////////////////////////////////
// Class:       FluxReweight
// Module Type: analyzer
// File:        FluxReweight_module.cc
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
#include "nusimdata/SimulationBase/MCFlux.h"

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

// C++ includes
#include <map>
#include <vector>
#include <string>

namespace sbnd {

  class FluxReweight : public art::EDAnalyzer {
  public:

    explicit FluxReweight(fhicl::ParameterSet const & p);

    // Called once, at start of the job
    virtual void beginJob() override;
 
    // Called for every sub run
    virtual void beginSubRun(const art::SubRun& subrun) override;

    // Called once per event
    virtual void analyze(const art::Event& event) override;

    // Called once, at end of the job
    virtual void endJob() override;

    // Reset all counters/variables to their null values
    void ResetWeights();

  private:

    // fcl file parameters
    art::InputTag fGenModuleLabel;      ///< name of gen producer

    // List of reco formats
    std::vector<std::string> fFluxWeightCalcs;

    evwgh::WeightManager _wgt_manager;
    TPCGeoAlg fTPCGeo;

    // Tree
    TTree *fMetaDataTree;
    //TTree *fWeightTree;

    // MetaData tree parameters
    double pot;

    // Weight tree parameters
    //double vtx_x, vtx_y, vtx_z;
    //int pdg;
    //double energy;
    double weights[100];

    TH1D* hNuMu;
    TH1D* hAntiNuMu;
    TH1D* hNuE;
    TH1D* hAntiNuE;

    std::vector<TH1D*> hNuMu_rw;
    std::vector<TH1D*> hAntiNuMu_rw;
    std::vector<TH1D*> hNuE_rw;
    std::vector<TH1D*> hAntiNuE_rw;

  }; // class FluxReweight

  FluxReweight::FluxReweight(fhicl::ParameterSet const & p)
    : EDAnalyzer{p}
    , fGenModuleLabel       {p.get<art::InputTag>("GenModuleLabel")}
  {

    _wgt_manager.Configure(p, *this);

    fFluxWeightCalcs = {
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

  }

  void FluxReweight::beginJob()
  {
    // Access tfileservice to handle creating and writing histograms
    art::ServiceHandle<art::TFileService> tfs;

    fMetaDataTree = tfs->make<TTree>("metadata", "xsec tree");
    fMetaDataTree->Branch("pot", &pot);
/*
    fWeightTree = tfs->make<TTree>("weight", "xsec tree");
    fWeightTree->Branch("vtx_x", &vtx_x);
    fWeightTree->Branch("vtx_y", &vtx_y);
    fWeightTree->Branch("vtx_z", &vtx_z);
    fWeightTree->Branch("pdg", &pdg);
    fWeightTree->Branch("energy", &energy);
    fWeightTree->Branch("weights", &weights, "weights[100]/D");
*/
    // Initial output
    std::cout<<"----------------- XSec Tree Module -------------------"<<std::endl;

    hNuMu = tfs->make<TH1D>("hNuMu", "", 60, 0, 3);
    hAntiNuMu = tfs->make<TH1D>("hAntiNuMu", "", 60, 0, 3);
    hNuE = tfs->make<TH1D>("hNuE", "", 60, 0, 3);
    hAntiNuE = tfs->make<TH1D>("hAntiNuE", "", 60, 0, 3);

    for(int i = 0; i < 100; i++){
      TH1D* hNuMu_tmp = tfs->make<TH1D>(Form("hNuMu_rw%i", i), "", 60, 0, 3);
      TH1D* hAntiNuMu_tmp = tfs->make<TH1D>(Form("hAntiNuMu_rw%i", i), "", 60, 0, 3);
      TH1D* hNuE_tmp = tfs->make<TH1D>(Form("hNuE_rw%i", i), "", 60, 0, 3);
      TH1D* hAntiNuE_tmp = tfs->make<TH1D>(Form("hAntiNuE_rw%i", i), "", 60, 0, 3);
      hNuMu_rw.push_back(hNuMu_tmp);
      hAntiNuMu_rw.push_back(hAntiNuMu_tmp);
      hNuE_rw.push_back(hNuE_tmp);
      hAntiNuE_rw.push_back(hAntiNuE_tmp);
    }

  } // FluxReweight::beginJob()


  // Called for every sub run
  void FluxReweight::beginSubRun(const art::SubRun& subrun){

    art::Handle< sumdata::POTSummary > potHandle;
    subrun.getByLabel(fGenModuleLabel, potHandle);
    const sumdata::POTSummary& potSum = (*potHandle);
    pot = potSum.totpot;

    fMetaDataTree->Fill();
    return;
  } // FluxReweight::beginSubRun()


  void FluxReweight::analyze(const art::Event& event)
  {

    // Fetch basic event info
    //std::cout<<"============================================"<<std::endl
    //         <<"Run = "<<event.run()<<", SubRun = "<<event.subRun()<<", Event = "<<event.id().event()<<std::endl
    //         <<"============================================"<<std::endl;

    //----------------------------------------------------------------------------------------------------------
    //                                          GETTING PRODUCTS
    //----------------------------------------------------------------------------------------------------------
    // Retrieve all the truth info in the events
    art::ServiceHandle<cheat::ParticleInventoryService> pi_serv;
    art::Handle<std::vector<simb::MCTruth>> genHandle;
    std::vector<art::Ptr<simb::MCTruth>> mctruthList;
    if(event.getByLabel(fGenModuleLabel, genHandle)) art::fill_ptr_vector(mctruthList, genHandle);

    //----------------------------------------------------------------------------------------------------------
    //                                           REWEIGHTING MCTRUTH
    //----------------------------------------------------------------------------------------------------------
      
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
      //if(!fTPCGeo.InFiducial(vertex, 0.)) continue;

      //vtx_x = vertex.X();
      //vtx_y = vertex.Y();
      //vtx_z = vertex.Z();

      if(std::abs(vertex.X()) > 2 || std::abs(vertex.Y()) > 2) continue;

      int pdg = mctruth->GetNeutrino().Nu().PdgCode();
      double energy = mctruth->GetNeutrino().Nu().E();

      if(pdg == 14) hNuMu->Fill(energy);
      if(pdg == -14) hAntiNuMu->Fill(energy);
      if(pdg == 12) hNuE->Fill(energy);
      if(pdg == -12) hAntiNuE->Fill(energy);

      // Copy event to non const type FIXME very naughty
      art::Event& e = const_cast<art::Event&>(event);
      evwgh::MCEventWeight mcwgh = _wgt_manager.Run(e, inu);
      //std::cout<<"Number of weights = "<<mcwgh.fWeight.size()<<"\n";
      for(auto const &kv : mcwgh.fWeight){
        if(std::find(fFluxWeightCalcs.begin(), fFluxWeightCalcs.end(), kv.first) != fFluxWeightCalcs.end()){
          //std::cout<<"Name = "<<kv.first<<" vector size = "<<kv.second.size()<<"\n";
          for(size_t i = 0; i < kv.second.size(); i++){
            if(i >= 100) continue;
            weights[i] *= kv.second[i];
          }
        }
      }
      for(size_t i = 0; i < 100; i++){
        if(pdg == 14) hNuMu_rw[i]->Fill(energy, weights[i]);
        if(pdg == -14) hAntiNuMu_rw[i]->Fill(energy, weights[i]);
        if(pdg == 12) hNuE_rw[i]->Fill(energy, weights[i]);
        if(pdg == -12) hAntiNuE_rw[i]->Fill(energy, weights[i]);
      }
      //fWeightTree->Fill();
    }

  } // FluxReweight::analyze()


  void FluxReweight::endJob(){

  } // FluxReweight::endJob()

  void FluxReweight::ResetWeights(){
    //vtx_x = -99999.;
    //vtx_y = -99999.;
    //vtx_z = -99999.;
    //pdg = -99999;
    //energy = -99999;
    for(size_t i = 0; i < 100; i++){
      weights[i] = 1.;
    }
  }
  

  DEFINE_ART_MODULE(FluxReweight)
} // namespace sbnd
