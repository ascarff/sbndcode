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

#include "sbndcode/RecoUtils/RecoUtils.h"



constexpr int kDefInt = -9999;
constexpr int kDefDoub = (double)(kDefInt);
constexpr int kNMaxTrackedParticles = 400;
constexpr int kNMaxNeutrinos = 30;


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
  void endJob() override;

private:

  void CountNeutrinos(art::Event const & e);
  void CountTrackedParticles(art::Event const & e);
  std::vector<TVector3> CollectTrajectoryPointsInTPC(const art::Ptr<simb::MCParticle> particle);
  double CalculateLength(std::vector<TVector3> points);
  void Reset();
  void BookParticleCountHistogram(TH1I*& hist, int pdg); //Book count histogram for particular pdg
  void FillParticleCountHistograms();
  void BookShowerConstituent(const art::Ptr<simb::MCParticle> shower_particle, std::vector<art::Ptr<simb::MCParticle> > const & particle_list);
  double GetShowerEnergyDepositionInTPC(std::vector<art::Ptr<simb::MCParticle> > const & shower);
  double GetParticleEnergyDepositionInTPC(art::Ptr<simb::MCParticle> const & particle);
  const art::Ptr<simb::MCParticle> GetMotherParticle(const art::Ptr<simb::MCParticle> particle, std::vector<art::Ptr<simb::MCParticle> > const & particle_list);
  //Map linking particle flavour to histograms of counts
  std::map<int,TH1I*> fParticleHistMap;
  std::map<int,int> fParticleCountMap;
  TH1I *fHistNNeutrinosPerSpill;
  TH1I *fHistNParticlesPerSpill;

  std::vector<std::vector<art::Ptr<simb::MCParticle> > >fShowers; //A container holding all of the shower constituents of an electron or photon
  // Declare member data here.
  TTree *fTree;
  int fTreeTP_N; //Number of tracked particles
  int fTreeTP_TrackID[kNMaxTrackedParticles];
  int fTreeTP_PDG[kNMaxTrackedParticles];
  int fTreeTP_Counted[kNMaxTrackedParticles]; //Is this particle counted in the pileup calculation
  double fTreeTP_Mom[kNMaxTrackedParticles];
  double fTreeTP_MomT[kNMaxTrackedParticles];
  double fTreeTP_MomX[kNMaxTrackedParticles];
  double fTreeTP_MomY[kNMaxTrackedParticles];
  double fTreeTP_MomZ[kNMaxTrackedParticles];
  double fTreeTP_StartT[kNMaxTrackedParticles];
  double fTreeTP_StartX[kNMaxTrackedParticles];
  double fTreeTP_StartY[kNMaxTrackedParticles];
  double fTreeTP_StartZ[kNMaxTrackedParticles];
  double fTreeTP_EndT[kNMaxTrackedParticles];
  double fTreeTP_EndX[kNMaxTrackedParticles];
  double fTreeTP_EndY[kNMaxTrackedParticles];
  double fTreeTP_EndZ[kNMaxTrackedParticles];
  double fTreeTP_TPCEnergyDep[kNMaxTrackedParticles];
  double fTreeTP_TPCTrajLength[kNMaxTrackedParticles];
};


Pileup::Pileup(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p)  // ,
 // More initializers here.
{
  Reset();
  art::ServiceHandle<art::TFileService> tfs;
  fTree = tfs->make<TTree>("pileup","pileup tree");
  fTree->Branch("TP_N",&fTreeTP_N);
  fTree->Branch("TP_TrackID",fTreeTP_TrackID,"TP_TrackID[TP_N]/I");
  fTree->Branch("TP_PDG",fTreeTP_PDG,"TP_PDG[TP_N]/I");
  fTree->Branch("TP_Counted",fTreeTP_Counted,"TP_Counted[TP_N]/I");
  fTree->Branch("TP_Mom",fTreeTP_Mom,"TP_Mom[TP_N]/D");
  fTree->Branch("TP_MomT",fTreeTP_MomT,"TP_MomT[TP_N]/D");
  fTree->Branch("TP_MomX",fTreeTP_MomX,"TP_MomX[TP_N]/D");
  fTree->Branch("TP_MomY",fTreeTP_MomY,"TP_MomY[TP_N]/D");
  fTree->Branch("TP_MomZ",fTreeTP_MomZ,"TP_MomZ[TP_N]/D");
  fTree->Branch("TP_StartT",fTreeTP_StartT,"TP_StartT[TP_N]/D");
  fTree->Branch("TP_StartX",fTreeTP_StartX,"TP_StartX[TP_N]/D");
  fTree->Branch("TP_StartY",fTreeTP_StartY,"TP_StartY[TP_N]/D");
  fTree->Branch("TP_StartZ",fTreeTP_StartZ,"TP_StartZ[TP_N]/D");
  fTree->Branch("TP_EndT",fTreeTP_EndT,"TP_EndT[TP_N]/D");
  fTree->Branch("TP_EndX",fTreeTP_EndX,"TP_EndX[TP_N]/D");
  fTree->Branch("TP_EndY",fTreeTP_EndY,"TP_EndY[TP_N]/D");
  fTree->Branch("TP_EndZ",fTreeTP_EndZ,"TP_EndZ[TP_N]/D");
  fTree->Branch("TP_TPCEnergyDep",fTreeTP_TPCEnergyDep,"TP_TPCEnergyDep[TP_N]/D");
  fTree->Branch("TP_TPCTrajLength",fTreeTP_TPCTrajLength,"TP_TPCTrajLength[TP_N]/D");

  BookParticleCountHistogram(fHistNNeutrinosPerSpill,0);
  fHistNNeutrinosPerSpill->SetName("ParticleCount_AllNeutrinos");
  fHistNNeutrinosPerSpill->GetXaxis()->SetTitle("No. #nu per spill");
  BookParticleCountHistogram(fHistNParticlesPerSpill,0); // book the total as 0
  fHistNParticlesPerSpill->SetName("ParticleCount_AllTrackedParticles");
  fHistNParticlesPerSpill->GetXaxis()->SetTitle("No. tracked particles per spill");

}

void Pileup::analyze(art::Event const & e)
{
  CountNeutrinos(e);
  CountTrackedParticles(e);
  FillParticleCountHistograms();
  fTree->Fill();
  Reset();
}

void Pileup::endJob(){
  //Loop through the histograms and make sure that we contain the same number of events in each.  Any absent events should go into the 0 bin.  This discrepancy is an artifact of how the histograms are made during processing and not prior to it
  //Get the total number of spills
  int NSpills = fHistNParticlesPerSpill->Integral(0,1000000);
  for (std::map<int,TH1I*>::iterator it = fParticleHistMap.begin(); it != fParticleHistMap.end(); it++){
    TH1I *hist = it->second;
    while (hist->Integral(0,1000000) < NSpills){
      hist->Fill(0);
    }
  }

  return;
}

void Pileup::CountNeutrinos(art::Event const & e){
  art::Handle< std::vector<simb::MCTruth> > truthListHandle;
  std::vector<art::Ptr<simb::MCTruth> > truthList;
  if (e.getByLabel("generator",truthListHandle)){
    art::fill_ptr_vector(truthList, truthListHandle);
  }

  for (unsigned int i_truth = 0; i_truth < truthList.size(); i_truth++){
    art::Ptr<simb::MCTruth> truth = truthList[i_truth];
    const simb::MCNeutrino neutrino = truth->GetNeutrino();
    TVector3 interaction_position = neutrino.Nu().Position(0).Vect();
    bool is_in_tpc = RecoUtils::IsInsideTPC(interaction_position,0); //Is the interaction exactly inside the TPC (no inner buffer)
    if (!is_in_tpc) continue;
    int pdg = neutrino.Nu().PdgCode();
    if (!fParticleHistMap[pdg]) {
      BookParticleCountHistogram(fParticleHistMap[pdg], pdg);
      fParticleCountMap[pdg] = 0;
    }
    fParticleCountMap[pdg]++;
  }
  return;
}


void Pileup::CountTrackedParticles(art::Event const & e){
  art::Handle< std::vector<simb::MCParticle> > particleListHandle;
  std::vector<art::Ptr<simb::MCParticle> > particleList;
  if (e.getByLabel("largeant",particleListHandle)){
    art::fill_ptr_vector(particleList, particleListHandle);
  }

  //std::cout<<"NParticles: " << particleList.size() << std::endl;
  //Reset the number of particles that will go into the tree
  //Loop over particles
  for (unsigned int i_part = 0; i_part < particleList.size(); i_part++){
    art::Ptr<simb::MCParticle> particle = particleList[i_part];
    std::vector<TVector3> particle_tpc_points = CollectTrajectoryPointsInTPC(particle);
    double particle_tpc_length = CalculateLength(particle_tpc_points);
    double tpc_energy_deposition = GetParticleEnergyDepositionInTPC(particle);
    int pdg = particle->PdgCode();

    if (fTreeTP_N < kNMaxTrackedParticles){
      //Fill the tree information
      fTreeTP_TrackID[fTreeTP_N]=particle->TrackId();
      fTreeTP_PDG[fTreeTP_N]=particle->PdgCode();
      fTreeTP_Counted[fTreeTP_N] = 0; //Assume not counted by default
      fTreeTP_Mom[fTreeTP_N]=particle->Momentum(0).Vect().Mag();
      fTreeTP_MomT[fTreeTP_N]=particle->Momentum(0).T();
      fTreeTP_MomX[fTreeTP_N]=particle->Momentum(0).X();
      fTreeTP_MomY[fTreeTP_N]=particle->Momentum(0).Y();
      fTreeTP_MomZ[fTreeTP_N]=particle->Momentum(0).Z();
      fTreeTP_StartT[fTreeTP_N]=particle->Position(0).T();
      fTreeTP_StartX[fTreeTP_N]=particle->Position(0).X();
      fTreeTP_StartY[fTreeTP_N]=particle->Position(0).Y();
      fTreeTP_StartZ[fTreeTP_N]=particle->Position(0).Z();
      fTreeTP_EndT[fTreeTP_N]=particle->EndT();
      fTreeTP_EndX[fTreeTP_N]=particle->EndX();
      fTreeTP_EndY[fTreeTP_N]=particle->EndY();
      fTreeTP_EndZ[fTreeTP_N]=particle->EndZ();
      fTreeTP_TPCEnergyDep[fTreeTP_N]=tpc_energy_deposition;
      fTreeTP_TPCTrajLength[fTreeTP_N]=particle_tpc_length;
      fTreeTP_N++;
    }

    //Book photons or electrons
    if (std::abs(pdg)==11 || std::abs(pdg)==22){
      BookShowerConstituent(particle, particleList);
      continue;
    }

    //Make checks
    if (std::abs(pdg) == 14 || std::abs(pdg)==12) continue; //No neutrinos
    if (std::abs(pdg)==2112 && tpc_energy_deposition == 1e-9) continue; //Only count neutrons if they deposit 1 KeV in the TPC
    if (particle_tpc_length < 0.6 && std::abs(pdg)!=2112) continue; //Need a particle to travel at least 3 wires in the TPC volume (non-neutrons only)


    //std::cout<<"--"<<i_part<<":  TrackId: " <<particle->TrackId() << "  Mother: " << particle->Mother() << "  PDG: " << particle->PdgCode() << "  TPC length: " << particle_tpc_length << std::endl; 
    //Book the histo if necessary
    if (!fParticleHistMap[pdg]) {
      BookParticleCountHistogram(fParticleHistMap[pdg], pdg);
      fParticleCountMap[pdg] = 0;
    }
    //Increment the count
    fParticleCountMap[pdg]++;
    //std::cout<<pdg<<" "<<fTreeTP_PDG[fTreeTP_N-1] << " " << fTreeTP_N-1 << " " << i_part<<std::endl;
    //We need to change the counted status of the particle
    if (i_part <= kNMaxTrackedParticles){
      fTreeTP_Counted[fTreeTP_N-1] = 1;
      if (fTreeTP_PDG[fTreeTP_N-1]==22){
        std::cout<<"OOPS"<<std::endl;
        std::cout<<pdg<<" "<<fTreeTP_PDG[fTreeTP_N-1]<<std::endl;
        std::cout<<fTreeTP_N-1<<" " << fTreeTP_N << " " << i_part << std::endl;

      }
    }
  }

  //Now that we've booked the shower particles into showers, we need to figure out whether to count them in the pileup calculations
  //The rule is, if the shower deposits any energy in the TPC, the entire shower counts as one particle in the pileup calculation
  for (unsigned int i_shower = 0; i_shower < fShowers.size(); i_shower++){
    double tpc_energy_deposition = GetShowerEnergyDepositionInTPC(fShowers[i_shower]);
    double shower_energy = fShowers[i_shower][0]->E(0);
    if (tpc_energy_deposition > shower_energy+1e-9){ //Any energy deposition within 1KeV of the shower energy is fine.
      std::cout<<"FOUND A SHOWER WHICH DEPOSITS MORE ENERGY THAN ITS TOTAL ENERGY.  HOW???  " << tpc_energy_deposition << " GeV deposited vs " << shower_energy << " GeV energy ("<<shower_energy-tpc_energy_deposition<<" GeV difference)"<<std::endl;
    }
    int shower_pdg = fShowers[i_shower][0]->PdgCode();
    if (tpc_energy_deposition > 1e-9){ //Shower must deposit 1 KeV in the detector
      if (!fParticleHistMap[shower_pdg]) {
        BookParticleCountHistogram(fParticleHistMap[shower_pdg], shower_pdg);
        fParticleCountMap[shower_pdg] = 0;
      }
      //Increment the count
      fParticleCountMap[shower_pdg]++;
      //To update the tree to say that we have counted this shower, change the counted flag for the initial shower particle
      for (int i_TP = 0; i_TP < fTreeTP_N; i_TP++){
        if (fTreeTP_TrackID[i_TP]==fShowers[i_shower][0]->TrackId()){
          fTreeTP_Counted[i_TP]=1;
        }
      }
    }
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
  fTreeTP_N = 0;
  for (int i_TP = 0; i_TP < fTreeTP_N; i_TP++){
    fTreeTP_TrackID[i_TP] = kDefInt;
    fTreeTP_PDG[i_TP] = kDefInt;
    fTreeTP_Counted[i_TP] = kDefInt;
    fTreeTP_Mom[i_TP] = kDefDoub;
    fTreeTP_MomT[i_TP] = kDefDoub;
    fTreeTP_MomX[i_TP] = kDefDoub;
    fTreeTP_MomY[i_TP] = kDefDoub;
    fTreeTP_MomZ[i_TP] = kDefDoub;
    fTreeTP_StartT[i_TP] = kDefDoub;
    fTreeTP_StartX[i_TP] = kDefDoub;
    fTreeTP_StartY[i_TP] = kDefDoub;
    fTreeTP_StartZ[i_TP] = kDefDoub;
    fTreeTP_EndT[i_TP] = kDefDoub;
    fTreeTP_EndX[i_TP] = kDefDoub;
    fTreeTP_EndY[i_TP] = kDefDoub;
    fTreeTP_EndZ[i_TP] = kDefDoub;
    fTreeTP_TPCEnergyDep[i_TP] = kDefDoub;
    fTreeTP_TPCTrajLength[i_TP] = kDefDoub;
  }

  for (std::map<int,int>::iterator it = fParticleCountMap.begin(); it != fParticleCountMap.end(); it++){
    it->second = 0;
  }
  for (unsigned i_shower = 0; i_shower < fShowers.size(); i_shower++){
    fShowers[i_shower].clear(); //clear all constituents of the shower
  }
  fShowers.clear(); //Clear all showers
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
  TString x_axis_title = Form("No. particles per spill (PDG=%i)",pdg);
  hist->GetXaxis()->SetTitle(x_axis_title);
  hist->GetYaxis()->SetTitle("No. spills");
  return;
}

void Pileup::FillParticleCountHistograms(){

  int total_neutrino_count = 0;
  int total_tracked_particle_count = 0;
  for (std::map<int,int>::iterator it = fParticleCountMap.begin(); it != fParticleCountMap.end(); it++){
    int pdg = it->first;
    int count = it->second;
    fParticleHistMap[pdg]->Fill(count);
    if (std::abs(pdg)==12 || std::abs(pdg)==14){
      total_neutrino_count+=count;
    }
    else{
      total_tracked_particle_count+=count;
    }
  }
  fHistNNeutrinosPerSpill->Fill(total_neutrino_count);
  fHistNParticlesPerSpill->Fill(total_tracked_particle_count);

  return;
}

void Pileup::BookShowerConstituent(const art::Ptr<simb::MCParticle> shower_particle, std::vector<art::Ptr<simb::MCParticle> > const & particle_list){
  int pdg = shower_particle->PdgCode();
  if (!(std::abs(pdg)==11 || std::abs(pdg)==22)){
    std::cout<<"ATTEMPTING TO BOOK PARTICLE WITH PDG " << pdg << " AS A SHOWER CONSTITUENT!!!! NOT BOOKING"<<std::endl;
    return;
  }
  //Get the mother id of this particle
  int mother_id = shower_particle->Mother();
  //and try and get the mother particle itself
  const art::Ptr<simb::MCParticle> mother_particle = GetMotherParticle(shower_particle, particle_list);
  if (mother_id==0 || mother_id==-1){ //We are dealing with a primary photon (this shouldn't actually happen in neutrino sim, only PG)
    //std::cout<<"Mother of shower has is absent"<< std::endl;

    std::vector<art::Ptr<simb::MCParticle> > new_shower_vector;
    new_shower_vector.push_back(shower_particle);
    fShowers.push_back(new_shower_vector);
    return;
  }
  //If the mother particle pointer is not available, then the mother's energy deposition was probably too low to be recorded.  In this case the heirarchy is broken.  It is safest to ignore this particle completely
  if (!(mother_particle.isAvailable())){
    return;
    //std::vector<art::Ptr<simb::MCParticle> > new_shower_vector;
    //new_shower_vector.push_back(shower_particle);
    //fShowers.push_back(new_shower_vector);
  }
  //Get the pdg of the mother
  int mother_pdg = mother_particle->PdgCode();
  if (!(std::abs(mother_pdg)==11 || std::abs(mother_pdg)==22)){ //Photon came from a non-showering particle, so it's the start of a shower
    //std::cout<<"Mother of shower has pdg: " << mother_pdg << std::endl;
    //Book a new showering particle
    std::vector<art::Ptr<simb::MCParticle> > new_shower_vector;
    new_shower_vector.push_back(shower_particle);
    fShowers.push_back(new_shower_vector);
  }
  else {
    //The shower particle is a shower constituent.  Find its parent shower.
    for (unsigned int i_shower = 0; i_shower < fShowers.size(); i_shower++){
      for (unsigned int i_constituent = 0; i_constituent < fShowers[i_shower].size(); i_constituent++){
        int shower_constituent_id = fShowers[i_shower][i_constituent]->TrackId();
        if (mother_id==shower_constituent_id){
          //We've found the mother of this shower constituent in one of the showers, add this constituent to the same shower
          fShowers[i_shower].push_back(shower_particle);
          return;
        }
      }
    }
    std::cout<<"ERROR: DID NOT MANAGE TO FIND THE PARENT SHOWER OF THIS CONSTITUENT"<<std::endl;
  }
  return;
}

double Pileup::GetShowerEnergyDepositionInTPC(std::vector<art::Ptr<simb::MCParticle> > const & shower){
  double energy_deposition = 0;
  for (unsigned int i_constituent = 0; i_constituent < shower.size(); i_constituent++){
    const art::Ptr<simb::MCParticle> particle = shower[i_constituent];
    energy_deposition += GetParticleEnergyDepositionInTPC(particle);
  }
  return energy_deposition;
}

double Pileup::GetParticleEnergyDepositionInTPC(art::Ptr<simb::MCParticle> const & particle){
  art::ServiceHandle<geo::Geometry> geo_serv;
  double energy_deposition = 0;
  //Nothing to do if there is only one traj point
  if (particle->NumberTrajectoryPoints() < 2) return energy_deposition;
  //Loop over the particles poiints STARTING FROM 1 and NOT 0
  for (unsigned int i_point = 1; i_point < particle->NumberTrajectoryPoints(); i_point++){
    //Get the position of the current point and check if its in the TPC
    TVector3 position = particle->Position(i_point).Vect();
    //Array it
    double pos_array[3];
    pos_array[0] = position.X();
    pos_array[1] = position.Y();
    pos_array[2] = position.Z();
    //Get the TPC at the previous position
    geo::TPCID tpcid = geo_serv->FindTPCAtPosition(pos_array);
    //If no TPC, move on
    if (!tpcid.isValid) continue;
    //The previous point was in the TPC.  So, get the energy of the particle and the previous point AND this point.  The difference in energy is the energy deposition (wow)
    double previous_energy = particle->E(i_point-1);
    double current_energy = particle->E(i_point);
    energy_deposition += (previous_energy-current_energy);
  }
  return energy_deposition;
}


const art::Ptr<simb::MCParticle> Pileup::GetMotherParticle(const art::Ptr<simb::MCParticle> particle, std::vector<art::Ptr<simb::MCParticle> > const & particle_list){
  //Make a blank Ptr.  We will return this if no mother is found.  The user of this function will need to check that the Ptr is OK
  const art::Ptr<simb::MCParticle> blank;

  int mother_id = particle->Mother();
  for (unsigned i_part = 0; i_part < particle_list.size(); i_part++){
    const art::Ptr<simb::MCParticle> current_particle = particle_list[i_part];
    int current_particle_id = current_particle->TrackId();
    if (mother_id == current_particle_id){
      return current_particle;
    }
  }

  return blank;
}


DEFINE_ART_MODULE(Pileup)
