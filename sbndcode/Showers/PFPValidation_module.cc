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

namespace ana {
  class PFPValidation;
}


class ana::PFPValidation : public art::EDAnalyzer {
  public:
    explicit PFPValidation(fhicl::ParameterSet const& p);
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


ana::PFPValidation::PFPValidation(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}
  // More initializers here.
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
}

void ana::PFPValidation::beginJob() {
  Tree->Branch("pfpNeutrinos",     "int",   &pfpNeutrinos);
  Tree->Branch("pfpTracks",        "int",   &pfpTracks);
  Tree->Branch("pfpShowers",       "int",   &pfpShowers);
  Tree->Branch("pfpVertexDistX",   "float", &pfpVertexDistX);
  Tree->Branch("pfpVertexDistY",   "float", &pfpVertexDistY);
  Tree->Branch("pfpVertexDistZ",   "float", &pfpVertexDistZ);
  Tree->Branch("pfpVertexDistMag", "float", &pfpVertexDistMag);
}

void ana::PFPValidation::analyze(art::Event const& e)
{
  //Getting  MC truth information
  art::Handle<std::vector<simb::MCTruth> > truthHandle;
  std::vector<art::Ptr<simb::MCTruth> > truths;
  if(evt.getByLabel(fGenieGenModuleLabel,truthHandle))
  {art::fill_ptr_vector(truths, truthHandle);}

  // Check there is 1 true neutrino and get it
  if (truths.size()==1){
    for (auto truth : truths){
      const simb::MCNeutrino& nuet = truth->GetNeutrino();
      const simb::MCParticle& neutrino = nuet.Nu();
    }
  } else return;
  // Get the neutrino vertex
  const TLorentzVector trueVertexVector = neutrino->Position();
  double trueVtx[3] = {trueVertexVector.X() ,trueVertexVector.Y(), trueVertexVector.Z()};

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

  for (auto const pfp: pfpsMap){
    // Find the pfp Neutrinos
    if ((pfp->PdgCode()==12) ||(pfp->PdgCode()==14)){
      pfpNeutrino = pfp->Self();
      ++pfpNeutrinos;

      // Loop over daughters
      const std::vector<size_t> daughters = pfp->Daughters();
      for (unsigned int daughter_iter=0; daughter_iter< daughters.size(); daughter_iter++) {
        art::Ptr<recob::PFParticle>& daughter = pfpsMap[daughters.at(daughter_iter)];
        if (Daughter->PdgCode() == 13) ++pfpTracks;
        if (Daughter->PdgCode() == 11) ++pfpShowers;
      }
    }
  }

  // Only calculate the vertex distance if we have 1 pfp neutrino
  if (pfpNeutrinos == 1) {
    // Get the neutrino pfp
    art::Ptr<recob::PFParticle>& neutrino = pfpsMap[pfpNeutrino];
    art::Ptr<recob::Vertex> pfpVertex;
    //Get the vertex
    if (fmpfv.isValid()) {
      art::Handle<std::vector<recob::Vertex > > vertexHandle;
      evt.get(fmpfv.at(0).front().id(),vertexHandle);
      if(vertexHandle.isValid()) {
        std::vector< art::Ptr<recob::Vertex> > pfpVertexVector = fmpfv.at(Daughter.key());
        if (pfpVertexVector.size == 1){
          pfpVertex = pfpVertexVector.front();
        } else {
          // Throw exception
        }

      }
    }

    // Get the pfp neutrino vertex
    double pfpVtx[3];
    pfpVertex->XYZ(pfpVtx);

    double pfpVertexDistX   = pfpVtx[0] - trueVtx[0];
    double pfpVertexDistY   = pfpVtx[1] - trueVtx[1];
    double pfpVertexDistZ   = pfpVtx[2] - trueVtx[2];
    double pfpVertexDistMag = TMath::Sqrt(TMath::Power(pfpVertexDistX,2) +
        TMath::Power(pfpVertexDistY,2) + TMath::Power(pfpVertexDistZ,2));
  }

  Tree->Fill();

  DEFINE_ART_MODULE(ana::PFPValidation)
