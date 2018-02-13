////////////////////////////////////////////////////////////////////////
// Class:       Pileup
// Plugin Type: analyzer (art v2_08_04)
// File:        Pileup_module.cc
//
// Generated at Fri Jan 19 03:31:19 2018 by Dominic Brailsford using cetskelgen
// from cetlib version v3_01_01.
////////////////////////////////////////////////////////////////////////
//STL
#include <map>

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

//ROOT
#include "TTree.h"
#include "TH1I.h"

//ART
#include "art/Framework/Services/Optional/TFileService.h"


//LArSoft
#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Cluster.h"




constexpr int kDefInt = -9999;
constexpr int kDefDoub = (double)(kDefInt);
constexpr int kNMaxTrueParticles = 100;
constexpr int kNMaxPFParticles = 100;


class Pileup;


class Pileup : public art::EDAnalyzer {
public:
  explicit Pileup(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  Pileup(Pileup const &) = delete;
  Pileup(Pileup &&) = delete;
  Pileup & operator = (Pileup const &) = delete;
  Pileup & operator = (Pileup &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;

private:

  void CollectTruth(art::Event const & e);
  std::vector<TVector3> CollectTrajectoryPointsInTPC(const art::Ptr<simb::MCParticle> particle);
  double CalculateLength(std::vector<TVector3> points);

  void Reset();
  void BookParticleCountHistogram(TH1I*& hist, int pdg); //Book count histogram for particular pdg
  void FillParticleCountHistograms();
  //Map linking particle flavour to histograms of counts
  std::map<int,TH1I*> fParticleHistMap;
  std::map<int,int> fParticleCountMap;

  // Declare member data here.
  TTree *fTree;
  /*
  int fNPFParticles;
  int fPFPNHits[kNMaxPFParticles];
  double fPFPHitPurity[kNMaxPFParticles];
  double fPFPHitCompleteness[kNMaxPFParticles];
  int fPFPTrueIndex[kNMaxPFParticles];
  int fNTrueParticles;
  int fTrueID[kNMaxTrueParticles];
  int fTruePDG[kNMaxTrueParticles];
  int fTrueMother[kNMaxTrueParticles];
  double fTrueMomX[kNMaxTrueParticles];
  double fTrueMomY[kNMaxTrueParticles];
  double fTrueMomZ[kNMaxTrueParticles];
  double fTrueMomT[kNMaxTrueParticles];
  double fTrueStartX[kNMaxTrueParticles];
  double fTrueStartY[kNMaxTrueParticles];
  double fTrueStartZ[kNMaxTrueParticles];
  double fTrueStartT[kNMaxTrueParticles];
  double fTrueTPCTrajLength[kNMaxTrueParticles];
  //double fTrueEndX[kNMaxTrueParticles];
  //double fTrueEndY[kNMaxTrueParticles];
  //double fTrueEndZ[kNMaxTrueParticles];
  //double fTrueEndT[kNMaxTrueParticles];
  //*/

};


Pileup::Pileup(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p)  // ,
 // More initializers here.
{
  Reset();
  art::ServiceHandle<art::TFileService> tfs;
  fTree = tfs->make<TTree>("pileup","pileup tree");
  /*
  fTree->Branch("NPFParticles",&fNPFParticles);
  fTree->Branch("PFPNHits",fPFPNHits,"PFPNHits[NPFParticles]/I");   
  fTree->Branch("PFPHitPurity",fPFPHitPurity,"PFPHitPurity[NPFParticles]/D");   
  fTree->Branch("PFPHitCompleteness",fPFPHitCompleteness,"PFPHitCompleteness[NPFParticles]/D");   
  fTree->Branch("PFPTrueIndex",fPFPTrueIndex,"PFPTrueIndex[NPFParticles]/I");   
  fTree->Branch("NTrueParticles",&fNTrueParticles);
  fTree->Branch("TrueID",fTrueID,"TrueID[NTrueParticles]/I");   
  fTree->Branch("TruePDG",fTruePDG,"TruePDG[NTrueParticles]/I");   
  fTree->Branch("TrueMother",fTrueMother,"TrueMother[NTrueParticles]/I");
  fTree->Branch("TrueMomX",fTrueMomX,"TrueMomX[NTrueParticles]/D");   
  fTree->Branch("TrueMomY",fTrueMomY,"TrueMomY[NTrueParticles]/D");   
  fTree->Branch("TrueMomZ",fTrueMomZ,"TrueMomZ[NTrueParticles]/D");   
  fTree->Branch("TrueMomT",fTrueMomT,"TrueMomT[NTrueParticles]/D");   
  fTree->Branch("TrueStartX",fTrueStartX,"TrueStartX[NTrueParticles]/D");   
  fTree->Branch("TrueStartY",fTrueStartY,"TrueStartY[NTrueParticles]/D");   
  fTree->Branch("TrueStartZ",fTrueStartZ,"TrueStartZ[NTrueParticles]/D");   
  fTree->Branch("TrueStartT",fTrueStartT,"TrueStartT[NTrueParticles]/D");   
  fTree->Branch("TrueTPCTrajLength",fTrueTPCTrajLength,"TrueTPCTrajLength[NTrueParticles]/D");   
  */
}

void Pileup::analyze(art::Event const & e)
{
  CollectTruth(e);
  FillParticleCountHistograms();
  fTree->Fill();
  Reset();
}

void Pileup::CollectTruth(art::Event const & e){
  art::Handle< std::vector<simb::MCParticle> > particleListHandle;
  std::vector<art::Ptr<simb::MCParticle> > particleList;
  if (e.getByLabel("largeant",particleListHandle)){
    art::fill_ptr_vector(particleList, particleListHandle);
  }

  //std::cout<<"NParticles: " << particleList.size() << std::endl;
  //Loop over particles
  for (unsigned int i_part = 0; i_part < particleList.size(); i_part++){
    art::Ptr<simb::MCParticle> particle = particleList[i_part];
    std::vector<TVector3> particle_tpc_points = CollectTrajectoryPointsInTPC(particle);
    double particle_tpc_length = CalculateLength(particle_tpc_points);
    int pdg = particle->PdgCode();

    //Make checks
    if (std::abs(pdg) == 14 || std::abs(pdg)==12) continue; //No neutrinos
    if (particle_tpc_length < 0.6) continue; //Need a particle to travel at least 3 wires in the TPC volume


    //std::cout<<"--"<<i_part<<":  TrackId: " <<particle->TrackId() << "  Mother: " << particle->Mother() << "  PDG: " << particle->PdgCode() << "  TPC length: " << particle_tpc_length << std::endl; 
    //Book the histo if necessary
    if (!fParticleHistMap[pdg]) {
      BookParticleCountHistogram(fParticleHistMap[pdg], pdg);
      fParticleCountMap[pdg] = 0;
    }
    //Increment the count
    fParticleCountMap[pdg]++;

  }


  return;
}



std::vector<TVector3> Pileup::CollectTrajectoryPointsInTPC(const art::Ptr<simb::MCParticle> particle){
  std::vector<TVector3> points;

  art::ServiceHandle<geo::Geometry> geo_serv;

  //Loop over the particles points
  for (unsigned int i_point = 0; i_point < particle->NumberTrajectoryPoints(); i_point++){
    //Get the position
    TVector3 position = particle->Position(i_point).Vect();
    //Array it
    double pos_array[3];
    pos_array[0] = position.X();
    pos_array[1] = position.Y();
    pos_array[2] = position.Z();
    //Get the TPC at this position
    geo::TPCID tpcid = geo_serv->FindTPCAtPosition(pos_array);
    //If no TPC, move on
    if (!tpcid.isValid) continue;
    points.push_back(position);
  }
  return points;
}

double Pileup::CalculateLength(std::vector<TVector3> points){
  double length = 0;
  //Nothign to calculate if we don't have enough points
  if (points.size() <= 1) return length;

  //Loop over the points.  Yes, this loop starts at 1
  for (unsigned int i_point = 1; i_point < points.size(); i_point++){
    //Accumulate total length from the distance between two adajecnt points
    length += (points[i_point]-points[i_point-1]).Mag();
  }
  return length;
}








void Pileup::Reset(){
  for (std::map<int,int>::iterator it = fParticleCountMap.begin(); it != fParticleCountMap.end(); it++){
    it->second = 0;
  }
  /*
  for (int i = 0; i < kNMaxTrueParticles; i++){
    fTrueID[i] = kDefInt;
    fTruePDG[i] = kDefInt;
    fTrueMother[i] = kDefInt;
    fTrueMomX[i] = kDefDoub;
    fTrueMomY[i] = kDefDoub;
    fTrueMomZ[i] = kDefDoub;
    fTrueMomT[i] = kDefDoub;
    fTrueStartX[i] = kDefDoub;
    fTrueStartY[i] = kDefDoub;
    fTrueStartZ[i] = kDefDoub;
    fTrueStartT[i] = kDefDoub;
    fTrueTPCTrajLength[i] = kDefDoub;
  }
  fNTrueParticles = 0;

  for (int i = 0; i < kNMaxPFParticles; i++){
    fPFPNHits[i] = kDefInt;
    fPFPHitPurity[i] = kDefDoub;
    fPFPHitCompleteness[i] = kDefDoub;
    fPFPTrueIndex[i] = kDefInt;
  }
  fNPFParticles = 0;
  */
  return;
}

void Pileup::BookParticleCountHistogram(TH1I*& hist, int pdg){
  art::ServiceHandle<art::TFileService> tfs;

  TString name = Form("ParticleCount_PDG_%i",pdg);
  hist = tfs->make<TH1I>(name,"",15,0,15);
  return;
}

void Pileup::FillParticleCountHistograms(){

  for (std::map<int,int>::iterator it = fParticleCountMap.begin(); it != fParticleCountMap.end(); it++){
    int pdg = it->first;
    int count = it->second;
    fParticleHistMap[pdg]->Fill(count);
  }

  return;
}

DEFINE_ART_MODULE(Pileup)
