////////////////////////////////////////////////////////////////////////
// Class:       PFPValidation
// Plugin Type: analyzer (art v3_02_06)
// File:        PFPValidation_module.cc
//
// Generated at Wed Oct  2 03:27:09 2019 by Edward Tyley using cetskelgen
// from cetlib version v3_07_02.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art_root_io/TFileService.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"

//LArSoft Includes
#include "nusimdata/SimulationBase/MCTruth.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Vertex.h"

//Root Includes
#include "TMath.h"
#include "TTree.h"
#include <vector>
#include <iostream>

namespace ana {
  class PFPValidation;
}


class ana::PFPValidation : public art::EDAnalyzer {
  public:
    explicit PFPValidation(fhicl::ParameterSet const& pset);
    // The compiler-generated destructor is fine for non-base
    // classes without bare pointers or other resource use.

    // Plugins should not be copied or assigned.
    PFPValidation(PFPValidation const&) = delete;
    PFPValidation(PFPValidation&&) = delete;
    PFPValidation& operator=(PFPValidation const&) = delete;
    PFPValidation& operator=(PFPValidation&&) = delete;

    // Required functions.
    void analyze(art::Event const& e) override;
    void beginJob();

  private:

    std::string fGenieGenModuleLabel;
    std::string fPFParticleLabel;
    art::ServiceHandle<art::TFileService> tfs;

    // Declare member data here.
    TTree* Tree;

    int pfpNeutrinos;
    int pfpTracks;
    int pfpShowers;
    float pfpVertexDistX;
    float pfpVertexDistY;
    float pfpVertexDistZ;
    float pfpVertexDistMag;
};


ana::PFPValidation::PFPValidation(fhicl::ParameterSet const& pset)
  : EDAnalyzer{pset}
{
  fPFParticleLabel     = pset.get<std::string>("PFParticleLabel");
  fGenieGenModuleLabel = pset.get<std::string>("GenieGenModuleLabel");
}

void ana::PFPValidation::beginJob() {
  Tree = tfs->make<TTree>("MetricTree", "Tree Holding all metric information");
  Tree->Branch("pfpNeutrinos",     &pfpNeutrinos);
  Tree->Branch("pfpTracks",        &pfpTracks);
  Tree->Branch("pfpShowers",       &pfpShowers);
  Tree->Branch("pfpVertexDistX",   &pfpVertexDistX);
  Tree->Branch("pfpVertexDistY",   &pfpVertexDistY);
  Tree->Branch("pfpVertexDistZ",   &pfpVertexDistZ);
  Tree->Branch("pfpVertexDistMag", &pfpVertexDistMag);
}

void ana::PFPValidation::analyze(art::Event const& evt)
{

  //Getting  MC truth information
  art::Handle<std::vector<simb::MCTruth> > truthHandle;
  std::vector<art::Ptr<simb::MCTruth> > truths;
  if(evt.getByLabel(fGenieGenModuleLabel,truthHandle))
  {art::fill_ptr_vector(truths, truthHandle);}

  // Check there is 1 true neutrino and get it
  double trueVtx[3] = {-999, -999, -999};
  if (truths.size()==1){
    for (auto truth : truths){
      const simb::MCNeutrino& nuet = truth->GetNeutrino();
      const simb::MCParticle& neutrino = nuet.Nu();
      const TLorentzVector trueVertexVector = neutrino.Position();
      trueVtx[0] = trueVertexVector.X();
      trueVtx[1] = trueVertexVector.Y();
      trueVtx[2] = trueVertexVector.Z();
    }
  } else {
    std::cout<<"More that 1 thruth neutrino. Returning."<<std::endl;
    return;
  }
  // Get the neutrino vertex

  // Get assns
  art::Handle<std::vector<recob::PFParticle> > pfpListHandle;
  std::vector<art::Ptr<recob::PFParticle> > pfps;
  if(evt.getByLabel(fPFParticleLabel,pfpListHandle))
  {art::fill_ptr_vector(pfps,pfpListHandle);}

  art::FindManyP<recob::Vertex> fmpfv(pfpListHandle, evt, fPFParticleLabel);

  // Re-Initialise branches
  pfpNeutrinos     = 0;
  pfpTracks        = 0;
  pfpShowers       = 0;
  pfpVertexDistX   = -999;
  pfpVertexDistY   = -999;
  pfpVertexDistZ   = -999;
  pfpVertexDistMag = -999;

  // Create a map between PFParticles and their IDs
  std::map<int, art::Ptr<recob::PFParticle> > pfpsMap;
  for (unsigned int i=0; i<pfps.size();++i){
    art::Ptr<recob::PFParticle>& pfp = pfps.at(i);
    pfpsMap[pfp->Self()] = pfp;
  }

  int pfpNeutrino; // ID of the pfp neutrino
  for (auto const pfp: pfps){
    // Find the pfp Neutrinos
    if ((pfp->PdgCode()==12) ||(pfp->PdgCode()==14)){
      pfpNeutrino = pfp->Self();
      ++pfpNeutrinos;

      // Loop over daughters
      const std::vector<size_t> daughters = pfp->Daughters();
      for (unsigned int daughter_iter=0; daughter_iter< daughters.size(); daughter_iter++) {
        art::Ptr<recob::PFParticle>& daughter = pfpsMap[daughters.at(daughter_iter)];
        if (daughter->PdgCode() == 13) ++pfpTracks;
        if (daughter->PdgCode() == 11) ++pfpShowers;
      }
    }
  }

  // Only calculate the vertex distance if we have 1 pfp neutrino
  if (pfpNeutrinos == 1) {
    // Get the neutrino pfp
    art::Ptr<recob::PFParticle>& neutrino = pfpsMap[pfpNeutrino];
    //Get the vertex
    if (fmpfv.isValid() && fmpfv.size()!=0 && (fmpfv.at(0)).size()!=0) {
      art::Handle<std::vector<recob::Vertex > > vertexHandle;
      evt.get(fmpfv.at(0).front().id(),vertexHandle);
      if(vertexHandle.isValid()) {

        std::vector< art::Ptr<recob::Vertex> > pfpVertexVector = fmpfv.at(neutrino.key());

        if (pfpVertexVector.size()==0) {
          std::cout<<"No pfp verticies. Returning."<<std::endl;
          return;
        }

        art::Ptr<recob::Vertex> pfpVertex = pfpVertexVector.front();

        // Get the pfp neutrino vertex
        double pfpVtx[3];
        pfpVertex->XYZ(pfpVtx);

        pfpVertexDistX   = pfpVtx[0] - trueVtx[0];
        pfpVertexDistY   = pfpVtx[1] - trueVtx[1];
        pfpVertexDistZ   = pfpVtx[2] - trueVtx[2];
        pfpVertexDistMag = TMath::Sqrt(TMath::Power(pfpVertexDistX,2) +
            TMath::Power(pfpVertexDistY,2) + TMath::Power(pfpVertexDistZ,2));

      }
    }
  }
  Tree->Fill();
}

DEFINE_ART_MODULE(ana::PFPValidation)
