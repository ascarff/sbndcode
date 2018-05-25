// Framework includes                                                                
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/View.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOneP.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

//Larsoft includes 
#include "larcore/Geometry/Geometry.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/MCFlux.h"
#include "lardataobj/Simulation/SimChannel.h"
#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"
#include "lardataobj/AnalysisBase/ParticleID.h"
#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/BeamInfo.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "larcoreobj/SummaryData/POTSummary.h"
#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/EndPoint2D.h"
#include "lardataobj/RecoBase/Vertex.h"
#include "lardataobj/RecoBase/Shower.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "lardataobj/AnalysisBase/CosmicTag.h"
#include "lardataobj/AnalysisBase/FlashMatch.h"
#include "sbndcode/RecoUtils/RecoUtils.h"

//Root Includes
#include "TFile.h"
#include "TMath.h"
#include "TTree.h"
#include "TGraph.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TH1D.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TROOT.h"
#include "TStyle.h"

//C++ Includes 
#include <vector>
#include <iostream>


namespace ana {
  class ShowerValidation;
}

class ana::ShowerValidation : public art::EDAnalyzer {
public:

  ShowerValidation(const fhicl::ParameterSet& pset);

  void analyze(const art::Event& evt);
  void endJob();
  void beginJob();
private:

  std::string fGenieGenModuleLabel;
  std::string fLArGeantModuleLabel;
  std::string fHitsModuleLabel;
  std::string fTrackModuleLabel;

  bool fUseBiggestShower;

  std::vector<std::string> fShowerModuleLabels;
  
  std::map<std::string,TH1F*> ShowerDirection_X_HistMap;
  std::map<std::string,TH1F*> ShowerDirection_Y_HistMap;
  std::map<std::string,TH1F*> ShowerDirection_Z_HistMap;
  std::map<std::string,TH1F*> ShowerStart_X_HistMap;
  std::map<std::string,TH1F*> ShowerStart_Y_HistMap;
  std::map<std::string,TH1F*> ShowerStart_Z_HistMap;
  std::map<std::string,TH1F*> ShowerLength_HistMap;
  std::map<std::string,TH1F*> ShowerEnergyDiff_HistMap;
  std::map<std::string,TH1F*> ShowerdEdx_HistMap;
  std::map<std::string,TH1F*> EventSeggy_HistMap;
  std::map<std::string,TH1F*> ShowerCompleteness_HistMap;
  std::map<std::string,TH1F*> ShowerPurity_HistMap;
  std::map<std::string,TH1F*> ShowerEnergy_HistMap;
  std::map<std::string,TH1F*> ShowerHitNum_HistMap;
  std::map<std::string,TH1F*> ShowerTotalEnergyDiff_HistMap;
  std::map<std::string,TH1F*> HitEnergy_HistMap;

  std::map<std::string,TGraph*> EnergyRes_MeanMap;
  std::map<std::string,TGraph*> EnergyRes_RMSMap;

  TMultiGraph*  EnergyRes_MeanMulti;
  TMultiGraph* EnergyRes_RMSMulti;

  std::map<std::string,std::map<int,TH1F*> > EnergyHist_map;
  std::vector<int> Energies = {200, 400, 600, 1000};

  art::ServiceHandle<geo::Geometry> geom;
  art::ServiceHandle<art::TFileService> tfs;

  TCanvas* ShowerDirection_X_canvas;
  TCanvas* ShowerDirection_Y_canvas;
  TCanvas* ShowerDirection_Z_canvas; 
  TCanvas* ShowerStart_X_canvas;
  TCanvas* ShowerStart_Y_canvas;
  TCanvas* ShowerStart_Z_canvas;
  TCanvas* ShowerLength_canvas;
  TCanvas* ShowerEnergyDiff_canvas;
  TCanvas* ShowerTotalEnergyDiff_canvas;
  TCanvas* ShowerdEdx_canvas;
  TCanvas* EventSeggy_canvas;
  TCanvas* ShowerCompleteness_canvas;
  TCanvas* ShowerPurity_canvas;
  TCanvas* ShowerEnergy_canvas;
  TCanvas* ShowerHitNum_canvas;
  TCanvas* HitEnergy_canvas;
  TCanvas* EnergyRes_Mean_canvas;
  TCanvas* EnergyRes_RMS_canvas;

};

ana::ShowerValidation::ShowerValidation(const fhicl::ParameterSet& pset) : EDAnalyzer(pset){

  fGenieGenModuleLabel = pset.get<std::string>("GenieGenModuleLabel");
  fLArGeantModuleLabel = pset.get<std::string>("LArGeantModuleLabel"); 
  fHitsModuleLabel     = pset.get<std::string>("HitsModuleLabel");
  fTrackModuleLabel    = pset.get<std::string>("TrackModuleLabel");
  fShowerModuleLabels  = pset.get<std::vector<std::string> >("ShowerModuleLabels");
  fUseBiggestShower    = pset.get<bool>("UseBiggestShower");

  //  TFile output_file("showervalidationGraphs.root","RECREATE");
  for(unsigned int j=0; j<fShowerModuleLabels.size(); ++j){

    std::string ShowerDirection_X_string = "ShowerDirection X " + fShowerModuleLabels[j];
    std::string ShowerDirection_Y_string = "ShowerDirection Y " + fShowerModuleLabels[j];
    std::string ShowerDirection_Z_string = "ShowerDirection Z " + fShowerModuleLabels[j];

    std::string ShowerStartX_string = "Shower Start X " + fShowerModuleLabels[j];
    std::string ShowerStartY_string = "Shower Start Y " + fShowerModuleLabels[j];
    std::string ShowerStartZ_string = "Shower Start Z " + fShowerModuleLabels[j];

    std::string ShowerLength_string          = "Shower Length " + fShowerModuleLabels[j];
    std::string ShowerEnergyDiff_string      =  "Shower Energy Diff " + fShowerModuleLabels[j];
    std::string ShowerTotalEnergyDiff_string = "Shower Total Energy Diff " + fShowerModuleLabels[j];
    std::string ShowerdEdX_string            = "Shower dEdX initial track" + fShowerModuleLabels[j];
    std::string EventSeggy_string            = "Seggy " + fShowerModuleLabels[j];
    std::string ShowerCompleteness_string    = "Shower Completeness " + fShowerModuleLabels[j];
    std::string ShowerPurity_string          = "Shower Purity " + fShowerModuleLabels[j];
    std::string ShowerEnergy_string          = "Shower Energy " + fShowerModuleLabels[j];
    std::string HitEnergy_string             = "Shower Hit Energy " +  fShowerModuleLabels[j];
    std::string ShowerHitNum_string          = "Shower Hit Num " + fShowerModuleLabels[j];
    std::string EnergyRes_Mean_string        = "Energy Res Mean " + fShowerModuleLabels[j];
    std::string EnergyRes_RMS_string         = "Energy Res RMS " +  fShowerModuleLabels[j];

    const char* ShowerDirection_X_name     =  ShowerDirection_X_string.c_str();
    const char* ShowerDirection_Y_name     =  ShowerDirection_Y_string.c_str();
    const char* ShowerDirection_Z_name     =  ShowerDirection_Z_string.c_str();
    const char* ShowerStartX_name          =  ShowerStartX_string.c_str();
    const char* ShowerStartY_name          =  ShowerStartY_string.c_str();
    const char* ShowerStartZ_name          =  ShowerStartZ_string.c_str();
    const char* ShowerLength_name          =  ShowerLength_string.c_str();
    const char* ShowerEnergyDiff_name      =  ShowerEnergyDiff_string.c_str();
    const char* ShowerTotalEnergyDiff_name = ShowerTotalEnergyDiff_string.c_str();
    const char* ShowerdEdX_name            = ShowerdEdX_string.c_str();
    const char* EventSeggy_name            = EventSeggy_string.c_str();
    const char* ShowerCompleteness_name    = ShowerCompleteness_string.c_str();
    const char* ShowerPurity_name          = ShowerPurity_string.c_str();
    const char* ShowerEnergy_name          = ShowerEnergy_string.c_str();
    const char* ShowerHitNum_name          = ShowerHitNum_string.c_str();
    const char* HitEnergy_name             = HitEnergy_string.c_str();
    //    const char* EnergyRes_Mean_name        = EnergyRes_Mean_string.c_str();
    //const char* EnergyRes_RMS_name         = EnergyRes_RMS_string.c_str();
    

    ShowerDirection_X_HistMap[fShowerModuleLabels[j]]  = new TH1F(ShowerDirection_X_name, ShowerDirection_X_name, 100, -10, 10);
    ShowerDirection_Y_HistMap[fShowerModuleLabels[j]]  = new TH1F(ShowerDirection_Y_name, ShowerDirection_Y_name, 100, -10, 10);
    ShowerDirection_Z_HistMap[fShowerModuleLabels[j]]  = new TH1F(ShowerDirection_Z_name, ShowerDirection_Z_name, 100, -10, 10);
    
    ShowerStart_X_HistMap[fShowerModuleLabels[j]]  = new TH1F(ShowerStartX_name, ShowerStartX_name, 400, -200, 200);
    ShowerStart_Y_HistMap[fShowerModuleLabels[j]]  = new TH1F(ShowerStartY_name, ShowerStartY_name, 400, -200, 200);
    ShowerStart_Z_HistMap[fShowerModuleLabels[j]]  = new TH1F(ShowerStartZ_name, ShowerStartZ_name, 600, -100, 100);
    
    ShowerLength_HistMap[fShowerModuleLabels[j]]          = new TH1F(ShowerLength_name, ShowerLength_name, 200, -20, 20);
    ShowerEnergyDiff_HistMap[fShowerModuleLabels[j]]      = new TH1F(ShowerEnergyDiff_name, ShowerEnergyDiff_name, 500, -5, 5);
    ShowerTotalEnergyDiff_HistMap[fShowerModuleLabels[j]] = new TH1F(ShowerTotalEnergyDiff_name, ShowerTotalEnergyDiff_name, 500, -5, 5);
    ShowerdEdx_HistMap[fShowerModuleLabels[j]]            = new TH1F(ShowerdEdX_name, ShowerdEdX_name, 500, -5, 5);
    EventSeggy_HistMap[fShowerModuleLabels[j]]            = new TH1F(EventSeggy_name, EventSeggy_name, 200, -10, 10);
    ShowerCompleteness_HistMap[fShowerModuleLabels[j]]    = new TH1F(ShowerCompleteness_name, ShowerCompleteness_name, 100, 0, 1);
    ShowerPurity_HistMap[fShowerModuleLabels[j]]          = new TH1F(ShowerPurity_name, ShowerPurity_name, 100, 0, 1);
    ShowerEnergy_HistMap[fShowerModuleLabels[j]]          = new TH1F(ShowerEnergy_name, ShowerEnergy_name, 20, 0, 5000);
    ShowerHitNum_HistMap[fShowerModuleLabels[j]]          = new TH1F(ShowerHitNum_name, ShowerHitNum_name, 12000, 0, 12000);
    HitEnergy_HistMap[fShowerModuleLabels[j]]             = new TH1F(HitEnergy_name, HitEnergy_name,100,-1,1);

    EnergyRes_MeanMap[fShowerModuleLabels[j]] = new TGraph(0);
    EnergyRes_RMSMap[fShowerModuleLabels[j]]  = new TGraph(0);
								  
    for(unsigned int i=0; i<Energies.size(); ++i){
      std::string EnergyHist_string;
      std::stringstream sstm;
      sstm << "Histogram with Particle Energy Generated with: " << Energies[i] << " MeV " << fShowerModuleLabels[j] ;
      EnergyHist_string = sstm.str();
      const char* name=  EnergyHist_string.c_str();
      EnergyHist_map[fShowerModuleLabels[j]][Energies[i]] =  tfs->make<TH1F>(name, name, 200, -2, 2);
    }
							     
  }//Shower Label Loop

  EnergyRes_MeanMulti = tfs->makeAndRegister<TMultiGraph>("EnergyRes_MeanMulti","EnergyRes_MeanMulti");
  EnergyRes_RMSMulti  = tfs->makeAndRegister<TMultiGraph>("EnergyRes_RMSMulti","EnergyRes_RMSMulti");


  ShowerDirection_X_canvas = tfs->makeAndRegister<TCanvas>("ShowerDirection_X_canvas","Shower Direction X");
  ShowerDirection_Y_canvas = tfs->makeAndRegister<TCanvas>("ShowerDirection_Y_canvas","Shower Direction Y");
  ShowerDirection_Z_canvas = tfs->makeAndRegister<TCanvas>("ShowerDirection_Z_canvas","Shower Direction Z");

  ShowerStart_X_canvas = tfs->makeAndRegister<TCanvas>("ShowerStart_X_canvas","Shower Start X");
  ShowerStart_Y_canvas = tfs->makeAndRegister<TCanvas>("ShowerStart_Y_canvas","Shower Start Y");
  ShowerStart_Z_canvas = tfs->makeAndRegister<TCanvas>("ShowerStart_Z_canvas","Shower Start Z");

  ShowerLength_canvas = tfs->makeAndRegister<TCanvas>("ShowerLength_canvas","Shower Length");
  ShowerEnergyDiff_canvas = tfs->makeAndRegister<TCanvas>("ShowerEnergyDiff_canvas","Shower EnergyDiff");
  ShowerTotalEnergyDiff_canvas = tfs->makeAndRegister<TCanvas>("ShowerTotalEnergyDiff_canvas","Shower Total Energy Diff");
  ShowerdEdx_canvas = tfs->makeAndRegister<TCanvas>("ShowerdEdx_canvas","Shower dEdx");
  EventSeggy_canvas = tfs->makeAndRegister<TCanvas>("EventSeggy_canvas","Event Seggy");
  ShowerCompleteness_canvas = tfs->makeAndRegister<TCanvas>("ShowerCompleteness_canvas","Shower Completeness");
  ShowerPurity_canvas = tfs->makeAndRegister<TCanvas>("ShowerPurity_canvas","Shower Purity");
  ShowerEnergy_canvas = tfs->makeAndRegister<TCanvas>("ShowerEnergy_canvas","Shower Energy");
  ShowerHitNum_canvas = tfs->makeAndRegister<TCanvas>("ShowerHitNum_canvas","Shower Hit Num");
  HitEnergy_canvas = tfs->makeAndRegister<TCanvas>("HitEnergy_canvas","Hit Energy");

  EnergyRes_Mean_canvas = tfs->makeAndRegister<TCanvas>("EnergyRes_Mean_canvas","Energy Resolution Mean");
  EnergyRes_RMS_canvas =  tfs->makeAndRegister<TCanvas>("EnergyRes_RMS_canvas","Energy Resolution RMS");

}
  
void ana::ShowerValidation::beginJob() {

}


void ana::ShowerValidation::analyze(const art::Event& evt) {

  std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;

  //Service handles 
  art::ServiceHandle<cheat::BackTrackerService> backtracker;
  art::ServiceHandle<cheat::ParticleInventoryService> particleInventory;
  art::ServiceHandle<geo::Geometry> geom;

  //  auto const* detprop = lar::providerFrom<detinfo::DetectorPropertiesService>();

  //Getting  MC truth information
  art::Handle< std::vector<simb::MCTruth> > mctruthListHandle;
  std::vector<art::Ptr<simb::MCTruth> > mclist;
  if(evt.getByLabel(fGenieGenModuleLabel,mctruthListHandle))
    {art::fill_ptr_vector(mclist, mctruthListHandle);}
  
  //Getting the SimWire Information
  //Get the SimChannels so that we can find the IDEs deposited on them.                                                                          
  art::Handle<std::vector<sim::SimChannel> > simChannelHandle;
  std::vector<art::Ptr<sim::SimChannel> > simchannels;
  if(evt.getByLabel(fLArGeantModuleLabel,simChannelHandle))
    {art::fill_ptr_vector(simchannels, simChannelHandle);}


  // Getting the Hit Information 
  art::Handle<std::vector<recob::Hit> > hitListHandle;
  std::vector<art::Ptr<recob::Hit> > hits;
  if(evt.getByLabel(fHitsModuleLabel,hitListHandle))
    {art::fill_ptr_vector(hits, hitListHandle);}

  for(unsigned int shwrlab_it=0; shwrlab_it<fShowerModuleLabels.size(); ++shwrlab_it){ 

    //Getting the Shower Information
    art::Handle<std::vector<recob::Shower> > showerListHandle;
    std::vector<art::Ptr<recob::Shower> > showers;
    std::string fShowerModuleLabel = fShowerModuleLabels[shwrlab_it];
    std::cout << "fShowerModuleLabel: " << fShowerModuleLabel << std::endl;
    if(evt.getByLabel(fShowerModuleLabel,showerListHandle))
      {art::fill_ptr_vector(showers,showerListHandle);}
    
    //Association between Showers and 2d Hits
    art::FindManyP<recob::Hit>  fmh(showerListHandle,   evt, fShowerModuleLabel);
    
    //Get the energy deposited in the hits. 
    float EnergyinHits = RecoUtils::TotalEnergyDepinHits(hits);
    
    
    //List the particles in the event
    const sim::ParticleList& particles = particleInventory->ParticleList();
    
    //Loop over the particles 
    std::map<int,const simb::MCParticle*> trueParticles;
    std::map<int,const simb::MCParticle*> trueInitialParticles;
    std::map<int,float> trueParticleEnergy;
    int num_of_showers_viaEcut = 0;
    int num_of_showers_viaDensitycut = 0;
    float simenergy=-99999999;
    
    for (sim::ParticleList::const_iterator particleIt = particles.begin(); particleIt != particles.end(); ++particleIt){
      const simb::MCParticle *particle = particleIt->second;
      trueParticleEnergy[particle->TrackId()] = 0;
      trueParticles[particle->TrackId()] = particle;
      std::cout << "Particle TrackID: " << particle->TrackId() << " PdgCode: " << particle->PdgCode() << " Mother: " << particle->Mother() << "Energy: "  << particle->E() << std::endl;
      
      //Particles with mother 0 are the initial particles (neutrino events this is the particles generated after the interaction. Keep note of these. 
      if(particle->Mother() == 0){
	
	simenergy = particle->E(); 
	trueInitialParticles[particle->TrackId()] = particle;
	HitEnergy_HistMap[fShowerModuleLabel]->Fill((simenergy*1000 - EnergyinHits)/1000*simenergy);
	
	//Check to see if the particle is contained. 
	
	//Find the trajectory of the particle;                                                                                                                                            
	//Get the number of Traj points to loop over       
	unsigned int TrajPoints = particle->NumberTrajectoryPoints();
	
	//Get the startpoistion so we can get the initial tpc.
	const TLorentzVector StartPositionTrajP = particle->Position(0);
	double start_vtx[3] = {StartPositionTrajP.X() ,StartPositionTrajP.Y(), StartPositionTrajP.Z()};
	geo::TPCID init_idtpc = geom->FindTPCAtPosition(start_vtx);
	
	//Loop over the trajectory points (they are in order). Loop to find the start point. 
	for(unsigned int TrajPoints_it=0; TrajPoints_it<TrajPoints; ++TrajPoints_it){
	  
	  //Find the vertex of the vector                                                            
	  const TLorentzVector PositionTrajP = particle->Position(TrajPoints_it);
	  double vtx[3] = {PositionTrajP.X() ,PositionTrajP.Y(), PositionTrajP.Z()};
	  
	  std::cout << "X: " << PositionTrajP.X() << " Y: " << PositionTrajP.Y() << " Z: " << PositionTrajP.Z() << std::endl;
	  
	  //Find if the vertex is in the TPC. If so make it the start point.                 
	  geo::TPCID idtpc = geom->FindTPCAtPosition(vtx);
	
	  if(idtpc != init_idtpc ){std::cout <<"Particle outside the TPC" << std::endl; continue;}
	  
	}
      }
      
      //I've read that pair production starts to dominate at around ~100 MeV so to find how many showers we expect loop over the mother particle. Pi0=143.97 MeV min gammas = 71.985 MeV which is greater than that from electrons at ~100MeV so pi0 should always shower? So cut on anything below 100MeV in energy.
      
      //It ain't a shower I'm interested in if it didn't start with a pi0 or electron...probably.
      int pdgcode = particle->PdgCode();
      if(pdgcode == 11 || pdgcode == 22){
	
	if(particle->E() > 0.1){
	  ++num_of_showers_viaEcut;
	}
	
	
	//using the RecoUtil function calculate the number of hits that see a charge deposition from the track.
	std::map<geo::PlaneID,int> Hit_num_map = RecoUtils::NumberofHitsThatContainEnergyDepositedByTrack(particle->TrackId(), hits);
	std::map<geo::PlaneID,int> Wire_num_map = RecoUtils::NumberofMCWiresHit(particle->TrackId(),simchannels);
	
	//Compare hit density on the collection plane;
	for(std::map<geo::PlaneID,int>::iterator Hitnum_iter=Hit_num_map.begin(); Hitnum_iter!=Hit_num_map.end(); ++Hitnum_iter){
	  if(Wire_num_map[Hitnum_iter->first] == 0){continue;} 
	  double Hit_num = (Hitnum_iter->second);
	  double Wire_num = Wire_num_map[Hitnum_iter->first];
	  double Hit_Density = Hit_num/Wire_num;
	  if(Hit_Density > 1){++num_of_showers_viaDensitycut; break;} 
	}
      }
    }
    
    std::cout << "num_of_showers_viaDensitycut: " << num_of_showers_viaDensitycut << " num_of_showers_viaEcut: " << num_of_showers_viaEcut << std::endl;
    
    std::vector< art::Ptr<recob::Hit> > showerhits; //hits in the shower    
    unsigned int max_hitnum=0;
    unsigned int biggest_shower_iter = 9999;

    for(unsigned int shower_iter = 0; shower_iter < showers.size(); ++shower_iter){
      //Get the shower  
      art::Ptr<recob::Shower>& shower = showers.at(shower_iter);
      
      //Get the hits vector from the shower
      showerhits = fmh.at(shower.key());
      
      if(showerhits.size() > max_hitnum){
	max_hitnum = showerhits.size();
	biggest_shower_iter = shower_iter;
      }
    }

    //Loop over hits associated with the shower add up the IDEs energy for each of the "track ID" and find the purity and compare other properites.
    
    std::cout << "Num of Showers: " << showers.size()<< " biggest_shower_iter: " << biggest_shower_iter << " max hit: " << max_hitnum << std::endl; 
    //Loop over the showers in the event
    for(unsigned int shower_iter = 0; shower_iter < showers.size(); ++shower_iter){
      
      if(fUseBiggestShower == true){
	if(shower_iter != biggest_shower_iter){continue;}
      }

      std::cout << "test shower" << std::endl;
      //Get the shower 
      art::Ptr<recob::Shower>& shower = showers.at(shower_iter);
      
      //Get the hits vector from the shower                                                          
      showerhits = fmh.at(shower.key());
      if(showerhits.size() == 0) {continue;}
      if(showerhits.size() < 200) {continue;}
      
      //Function from RecoUtils, finds the most probable track ID associated with the set of hits from there true energy depositons. 
      int ShowerTrackID = RecoUtils::TrueParticleIDFromTotalTrueEnergy(showerhits);
      
      
      //For all the particles that orginate from the showering particle add up the energy.
      double TotalTrueEnergyDepFromShowerDaughters = 0; 
      double TrueEnergyDep_FromShower = RecoUtils::TrueEnergyDepositedFromMCTrack(ShowerTrackID, simchannels);
      
      for (sim::ParticleList::const_iterator particleIt = particles.begin(); particleIt != particles.end(); ++particleIt){
	const simb::MCParticle *particle = particleIt->second;
      if(particle->TrackId() == ShowerTrackID){TotalTrueEnergyDepFromShowerDaughters += RecoUtils::TrueEnergyDepositedFromMCTrack(particle->TrackId(), simchannels); continue;}
      
      int motherID = particle->Mother();
      while(motherID != ShowerTrackID &&  motherID != 0){
    	const simb::MCParticle * particle_prev =  trueParticles[motherID]; 
	
    	//Function from RecoUtils, adds up all the energy deposited from that Track ID.
    	//	double TrueEnergyDep_Fromtrack = RecoUtils::TrueEnergyDepositedFromMCTrack(ShowerTrackID, simchannels);
    	motherID = particle_prev->Mother();
    	if(motherID == ShowerTrackID){TotalTrueEnergyDepFromShowerDaughters += RecoUtils::TrueEnergyDepositedFromMCTrack(particle->TrackId(), simchannels);}
      }
      }

      //Energy deposited within set of hits assigned to the shower by reconstruction from the true track id.
      double TrueEnergyDep_Fromtrack_WithinShower = 0;
      
      //Energy deposited within the set of Hits associated to the shower.
      double TrueEnergyDep_WithinShower = 0; 
      
      //Loop over the hits and find the IDEs 
      for(std::vector< art::Ptr<recob::Hit> >::iterator hitIt=showerhits.begin(); hitIt!=showerhits.end(); ++hitIt){
	
	//Get the plane ID 
	geo::WireID wireid = (*hitIt)->WireID();
	int PlaneID = wireid.Plane;
	if(PlaneID != 2){continue;}
	
	//Split the Hit into its IDE for each track it associates with.                               
	std::vector<sim::TrackIDE> trackIDEs = backtracker->HitToTrackIDEs((*hitIt));
	
      for (unsigned int idIt = 0; idIt < trackIDEs.size(); ++idIt){ 
	
  	//Find the true total energy deposited in a set of hits. 
  	TrueEnergyDep_WithinShower += trackIDEs.at(idIt).energy;
	
  	//Add up the contribution from the shower trackID. 
  	if(TMath::Abs(trackIDEs.at(idIt).trackID) == ShowerTrackID){
	  TrueEnergyDep_Fromtrack_WithinShower += trackIDEs.at(idIt).energy;
  	}
      }
      }//Hit Loop
      
      double completeness =  (TrueEnergyDep_FromShower - TrueEnergyDep_Fromtrack_WithinShower) /TrueEnergyDep_FromShower;
      double purity       =  (TrueEnergyDep_FromShower - TrueEnergyDep_Fromtrack_WithinShower)/TrueEnergyDep_WithinShower;
      
      //Add to the respective hitograms.
      ShowerCompleteness_HistMap[fShowerModuleLabel]->Fill(completeness);
      ShowerPurity_HistMap[fShowerModuleLabel]->Fill(purity);
      
      //Find the MCParticle this shower associates to
      const simb::MCParticle* MCShowerParticle = trueParticles.at(ShowerTrackID);
      
      //Find the Energy of the particle: 
      //double Energy = MCShowerParticle->E();
      
      //Get the number of Traj points to loop over                                                    
      unsigned int TrajPoints = MCShowerParticle->NumberTrajectoryPoints();
      
      //Find the start and end points of the initial particle in order to compare the track length. 
      const TLorentzVector PositionTrajStart =  MCShowerParticle->Position(0);
      const TLorentzVector PositionTrajEnd   =  MCShowerParticle->Position(TrajPoints-1);
      
      //The three vecotor for track length is the shower direction 
      TVector3  TrueShowerDirection = (PositionTrajEnd - PositionTrajStart).Vect();
      
      //Initial track lentgh of the shower.
      double TrueTrackLength = TrueShowerDirection.Mag();
      
      //Get the information for the shower  
      const int ShowerBest_Plane                       = shower->best_plane(); 
      const TVector3 & ShowerDirection                 = shower->Direction();
      const TVector3 & ShowerStart                     = shower->ShowerStart();
      const double   & ShowerTrackLength               = shower->Length();       
      const std::vector< double > & ShowerEnergyPlanes = shower->Energy();
      const std::vector< double > & ShowerdEdX_vec     = shower->dEdx(); 

      //Get the Fraction off the true value and fill the histograms.
      ShowerDirection_X_HistMap[fShowerModuleLabel]->Fill(TMath::Abs(TrueShowerDirection.X()-ShowerDirection.X()));
      ShowerDirection_Y_HistMap[fShowerModuleLabel]->Fill(TMath::Abs(TrueShowerDirection.Y()-ShowerDirection.Y()));
      ShowerDirection_Z_HistMap[fShowerModuleLabel]->Fill(TMath::Abs(TrueShowerDirection.Z()-ShowerDirection.Z()));
      ShowerStart_X_HistMap[fShowerModuleLabel]->Fill(TMath::Abs(PositionTrajStart.X()-ShowerStart.X()));
      ShowerStart_Y_HistMap[fShowerModuleLabel]->Fill(TMath::Abs(PositionTrajStart.Y()-ShowerStart.Y()));
      ShowerStart_Z_HistMap[fShowerModuleLabel]->Fill(TMath::Abs(PositionTrajStart.Z()-ShowerStart.Z()));
      ShowerLength_HistMap[fShowerModuleLabel]->Fill(TMath::Abs(TrueTrackLength-ShowerTrackLength));
      std::cout << "ShowerBest_Plane: " << ShowerBest_Plane << std::endl;
      std::cout << "ShowerEnergyPlanes.size() " << ShowerEnergyPlanes.size() << std::endl;
      std::cout << "TrueTrackLength: " << TrueTrackLength << " ShowerTrackLength: " << ShowerTrackLength << std::endl;
      std::cout << " ShowerEnergyPlanes[ShowerBest_Plane]: " << ShowerEnergyPlanes[ShowerBest_Plane] << std::endl;
      ShowerEnergyDiff_HistMap[fShowerModuleLabel]->Fill((TrueEnergyDep_FromShower-ShowerEnergyPlanes[ShowerBest_Plane]/3)/TrueEnergyDep_FromShower);
      
      ShowerTotalEnergyDiff_HistMap[fShowerModuleLabel]->Fill((TotalTrueEnergyDepFromShowerDaughters-ShowerEnergyPlanes[ShowerBest_Plane]/3)/TotalTrueEnergyDepFromShowerDaughters);
      ShowerEnergy_HistMap[fShowerModuleLabel]->Fill(ShowerEnergyPlanes[ShowerBest_Plane]/3);
      ShowerHitNum_HistMap[fShowerModuleLabel]->Fill(showerhits.size());
      
      //EnergyHist_map[fShowerModuleLabel][simenergy*1000]->Fill((TrueEnergyDep_FromShower-ShowerEnergyPlanes[ShowerBest_Plane]/3)/TrueEnergyDep_FromShower);

      std::cout << "#################################################" << std::endl;
      std::cout << "True Shower ID: " <<  ShowerTrackID << std::endl;
      std::cout << "X Poisition: " <<  ShowerStart.X() << "Y Position " << ShowerStart.Y() << " Z Poistion: " << ShowerDirection.Z() << std::endl;
      std::cout << "TrueEnergyDep_Fromtrack_WithinShower: " <<  TrueEnergyDep_Fromtrack_WithinShower << std::endl;
      std::cout << "TrueEnergyDep_FromShower: " << TrueEnergyDep_FromShower << std::endl;
      std::cout << "TrueEnergy deposited by hits in shower: " <<  TrueEnergyDep_WithinShower << std::endl;
      std::cout << "Purity: " << purity << " completeness: " << completeness << std::endl;
      std::cout << "Hit Size: " << showerhits.size() << std::endl;
      std::cout << "ShowerEnergyPlanes: " << ShowerEnergyPlanes[ShowerBest_Plane]/3 << std::endl;
      std::cout << "#################################################" <<std::endl;
      
      for(std::vector< double >::const_iterator dEdx=ShowerdEdX_vec.begin();dEdx!=ShowerdEdX_vec.end(); ++dEdx){
	ShowerdEdx_HistMap[fShowerModuleLabel]->Fill((*dEdx));
      }
      
    }//Shower Loop 
    
    //Whats the segementyness of the event. 
    EventSeggy_HistMap[fShowerModuleLabel]->Fill(num_of_showers_viaDensitycut - showers.size());
    
    for (sim::ParticleList::const_iterator particleIt = particles.begin(); particleIt != particles.end(); ++particleIt){
      const simb::MCParticle *particle = particleIt->second;
      std::cout << "Particle TrackID: " << particle->TrackId() << " PdgCode: " << particle->PdgCode() << " Mother: " << particle->Mother() << "Energy: "  << particle->E() << "test" << std::endl;
    }
    
  }
    


  // //This is for Rhiannon 
  // for(std::vector<art::Ptr<simb::MCTruth> >::const_iterator genielist = mclist.begin(); genielist != mclist.end(); ++genielist){ 
  //   std::cout << "Origin: " << (*genielist)->Origin() << std::endl;
  //   int num_genieparticles = (*genielist)->NParticles();
    
  //   simb::MCNeutrino  neutrino = (*genielist)->GetNeutrino();
  //   std::cout << "Neutrino interaction: " << neutrino.InteractionType()<< std::endl;
    
    
  //   for(int i=0; i<num_genieparticles; ++i){
  //     const simb::MCParticle & particle = (*genielist)->GetParticle(i);
  //     std::cout << "Particle TrackID: " << particle.TrackId() << " PdgCode: " << particle.PdgCode() << " Mother: " << particle.Mother() << std::endl;
  //   }
  // }

  
  return; 
}



void ana::ShowerValidation::endJob() {
  
  //  gStyle->SetOptTitle(kFALSE);
  gROOT->Reset();
  gROOT->SetStyle("Modern");
  gStyle->SetOptStat(0);
  gStyle->SetPalette(1);

  //  TFile *f = new TFile("ShowerModuleComparisions.root","RECREATE");
  std::vector<int> colours; 
    for(unsigned int j=0; j<fShowerModuleLabels.size(); ++j){
      colours.push_back(j+2);
    }
 
  //Create the legend                                   
  TLegend *leg = new TLegend(0.65, 0.1, 0.9, 0.35);
  //leg->SetFillColor(0);
  //leg->SetHeader("Shower Modules");
  //leg->SetBorderSize(1);
  //leg->SetTextSize(0.025);
  
  int fillstyle = 3000;

  for(unsigned int j=0; j<fShowerModuleLabels.size(); ++j){

    ++fillstyle; 

    for(unsigned int i=0; i<Energies.size(); ++i){
      float mean = EnergyHist_map[fShowerModuleLabels[j]][Energies[i]]->GetMean();
      float RMS = EnergyHist_map[fShowerModuleLabels[j]][Energies[i]]->GetRMS();
      EnergyRes_MeanMap[fShowerModuleLabels[j]]->SetPoint(EnergyRes_MeanMap[fShowerModuleLabels[j]]->GetN(), Energies[i], mean);
      EnergyRes_RMSMap[fShowerModuleLabels[j]]->SetPoint(EnergyRes_RMSMap[fShowerModuleLabels[j]]->GetN(), Energies[i], RMS);
    }
    
    EnergyRes_MeanMap[fShowerModuleLabels[j]]->SetMarkerColor(colours[j]);
    EnergyRes_MeanMap[fShowerModuleLabels[j]]->SetMarkerStyle(8);

    EnergyRes_RMSMap[fShowerModuleLabels[j]]->SetMarkerColor(colours[j]);
    EnergyRes_RMSMap[fShowerModuleLabels[j]]->SetMarkerStyle(8);

    EnergyRes_MeanMulti->Add(EnergyRes_MeanMap[fShowerModuleLabels[j]]);
    EnergyRes_RMSMulti->Add(EnergyRes_RMSMap[fShowerModuleLabels[j]]);

    ShowerDirection_X_canvas->cd();
    ShowerDirection_X_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerDirection_X_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerDirection_X_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerDirection_X_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerDirection_X_HistMap[fShowerModuleLabels[j]]->Draw("SAME");

    const char* name_legend = fShowerModuleLabels[j].c_str();
    leg->AddEntry(ShowerDirection_X_HistMap[fShowerModuleLabels[j]], name_legend);


    ShowerDirection_Y_canvas->cd();
    ShowerDirection_Y_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerDirection_Y_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerDirection_Y_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerDirection_Y_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerDirection_Y_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerDirection_Z_canvas->cd();
    ShowerDirection_Z_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerDirection_Z_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerDirection_Z_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerDirection_Z_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerDirection_Z_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerStart_X_canvas->cd();
    ShowerStart_X_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerStart_X_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerStart_X_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerStart_X_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerStart_X_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();    
  
    ShowerStart_Y_canvas->cd();
    ShowerStart_Y_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerStart_Y_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerStart_Y_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerStart_Y_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerStart_Y_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerStart_Z_canvas->cd();
    ShowerStart_Z_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerStart_Z_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerStart_Z_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerStart_Z_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerStart_Z_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerLength_canvas->cd();
    ShowerLength_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerLength_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerLength_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerLength_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerLength_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerEnergyDiff_canvas->cd();
    ShowerEnergyDiff_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerEnergyDiff_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerEnergyDiff_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerEnergyDiff_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerEnergyDiff_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerTotalEnergyDiff_canvas->cd();
    ShowerTotalEnergyDiff_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerTotalEnergyDiff_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerTotalEnergyDiff_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerTotalEnergyDiff_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerTotalEnergyDiff_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();
    
    ShowerdEdx_canvas->cd();
    ShowerdEdx_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerdEdx_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerdEdx_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerdEdx_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerdEdx_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    EventSeggy_canvas->cd();
    EventSeggy_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    EventSeggy_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    EventSeggy_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    EventSeggy_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    EventSeggy_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerCompleteness_canvas->cd();
    ShowerCompleteness_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerCompleteness_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerCompleteness_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerCompleteness_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerCompleteness_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerPurity_canvas->cd();
    ShowerPurity_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerPurity_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerPurity_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerPurity_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerPurity_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerEnergy_canvas->cd();
    ShowerEnergy_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerEnergy_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerEnergy_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerEnergy_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerEnergy_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    ShowerHitNum_canvas->cd();
    ShowerHitNum_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    ShowerHitNum_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    ShowerHitNum_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    ShowerHitNum_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    ShowerHitNum_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

    HitEnergy_canvas->cd();
    HitEnergy_HistMap[fShowerModuleLabels[j]]->SetDirectory(0);
    HitEnergy_HistMap[fShowerModuleLabels[j]]->SetFillColor(colours[j]);
    HitEnergy_HistMap[fShowerModuleLabels[j]]->SetFillStyle(fillstyle);
    HitEnergy_HistMap[fShowerModuleLabels[j]]->SetLineColor(1);
    HitEnergy_HistMap[fShowerModuleLabels[j]]->Draw("SAME");
    leg->Draw();

  }


}


DEFINE_ART_MODULE(ana::ShowerValidation)
