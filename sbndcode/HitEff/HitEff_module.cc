// Analyser to look at the reco file we just made                                                 

// ##########################                                                                     
// ### Framework includes ###                                                                     
// ##########################                                                                     
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Persistency/Common/FindManyP.h"



// ########################                                                                       
// ### LArSoft includes ###                                                                       
// ########################                                                                       
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireGeo.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/SpacePoint.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardata/Utilities/AssociationUtil.h"
#include "larsim/MCCheater/BackTracker.h"
#include "lardataobj/AnalysisBase/ParticleID.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/MCTrajectory.h"
#include "larsim/MCCheater/BackTracker.h"
#include "lardataobj/AnalysisBase/Calorimetry.h"
#include "lardataobj/MCBase/MCHitCollection.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"

// #####################                                                                          
// ### ROOT includes ###                                                                          
// #####################                                                                          
#include "TComplex.h"
#include "TMath.h"
#include "Math/Functor.h"
#include "Math/Minimizer.h"
#include "Math/Factory.h"
#include "TRandom2.h"
#include "TError.h"
#include "TMinuit.h"
#include "TFile.h"
#include "TH2D.h"
#include "TH1D.h"
#include "TF1.h"
#include "TGraph.h"
#include "TTree.h"
#include "TTimeStamp.h"
#include "TChain.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TGraphAsymmErrors.h"
#include "TSystem.h"
#include "TProfile.h"

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <utility>
#include <map>


namespace ana {
  class HitEff;
}

enum class TrackID   : int { };

//Class for Doms MC  Hit objects.
class MCHits {
  
  int channel;
  int peak_time;
  int TrackID = 0; 
  double total_charge; 
  double peak_charge; 
  std::vector<std::pair<int,double> > time_charge_vec;
  
public:
  MCHits(int,int,double,double, std::vector<std::pair<int,double> >); 
  MCHits(int, std::vector<std::pair<int,double> >);
  MCHits(int, std::vector<std::pair<int,double> >, int);
  int PeakTime(){return peak_time;}
  int Channel(){return channel;}
  int Trackid(){return TrackID;}
  double PeakCharge(){return peak_charge;}
  double TotalCharge(){return total_charge;}
  std::vector<std::pair<int,double> > TimeChargeVec(){return time_charge_vec;}
};

  MCHits::MCHits(int chan, std::vector<std::pair<int,double> > time_char_vec){
    channel = chan;
    time_charge_vec = time_char_vec;
    
    double max_charge =0;
    double max_time = 0;
    double charge=0;
    double sum_charge=0;
 
    for(std::vector<std::pair<int,double> >::iterator iter = time_charge_vec.begin();iter != time_charge_vec.end();++iter){
      if(iter->first == (std::prev(iter))->first){charge =+ iter->second;} 
      else {charge = iter->second;}
      if(charge > max_charge){max_charge = charge; max_time = iter->first;}
      sum_charge += iter->second;
    }

    peak_charge = max_charge;
    total_charge = sum_charge;
    peak_time = max_time;
       
  }

MCHits::MCHits(int chan, std::vector<std::pair<int,double> > time_char_vec, int trackid){
  
  channel = chan;
  time_charge_vec = time_char_vec;

  double max_charge =0;
  double max_time = 0;
  double charge=0;
  double sum_charge=0;

  for(std::vector<std::pair<int,double> >::iterator iter = time_charge_vec.begin();iter != time_charge_vec.end();++iter){
    if(iter->first == (std::prev(iter))->first){charge =+ iter->second;}
    else {charge = iter->second;}
    if(charge > max_charge){max_charge = charge; max_time = iter->first;}
    sum_charge += iter->second;
  }

  peak_charge = max_charge;
  total_charge = sum_charge;
  peak_time = max_time;

  TrackID = trackid ; 
}


MCHits::MCHits(int chan, int peak_t ,double tot_charge, double peak_char,  std::vector<std::pair<int,double> > time_char_vec){
    channel = chan;
    peak_time = peak_t;
    total_charge = tot_charge;
    peak_charge = peak_char; 
    time_charge_vec = time_char_vec;
  }

MCHits MergeHits(MCHits first_hit, MCHits second_hit){

  std::vector<std::pair<int,double> > first_hit_vec = first_hit.TimeChargeVec();
  std::vector<std::pair<int,double> > second_hit_vec = second_hit.TimeChargeVec();
  first_hit_vec.insert(first_hit_vec.end(), second_hit_vec.begin(), second_hit_vec.end()); 
  std::vector<std::pair<int,double> > time_charge_vec =  first_hit_vec;
  int channel = first_hit.Channel();
  int TrackID;
  if(first_hit.Trackid() == second_hit.Trackid()){TrackID = first_hit.Trackid();}
  else{TrackID=0;}
  MCHits MC_hit(channel,time_charge_vec,TrackID);
  return  MC_hit; 
}

bool sortbysec(const std::pair<int,double> &a,
	       const std::pair<int,double> &b)
{
  return (a.second > b.second);
}

std::vector<MCHits> CutHit(MCHits hit,std::vector<std::pair<int,double> > MaxPoints){
  std::vector<MCHits> hit_vec;
  if(MaxPoints.size()<2){hit_vec.push_back(hit); return hit_vec;} 
  std::cout << "#########################" << std::endl;
 
  sort(MaxPoints.begin(), MaxPoints.end(), sortbysec); 
  std::vector<std::pair<int,double> > TimeChargeVector =  hit.TimeChargeVec();
  TH1D *ChargeHist = new TH1D("ChargeHist","Charge on the Channel Hist",3000,0,3000);

  for(std::vector<std::pair<int,double> >::iterator iter=TimeChargeVector.begin(); iter!=TimeChargeVector.end(); ++iter){
    ChargeHist->Fill(iter->first,iter->second);
    std::cout << "time: " << iter->first << " Charge: " << iter->second << std::endl;
  }

  int n = 3;
  std::stringstream sstm1; 
  std::string fit_string;
  std::vector<double> Parameter_vec;
  std::vector<std::pair<double,std::vector<double> > > chi2s;
  std::vector<double> par_min;
  std::vector<double> par_max;

  // for(std::vector<std::pair<int,double> >::iterator iter=MaxPoints.begin(); iter!=MaxPoints.end(); ++iter){
  //   Parameter_vec.push_back(iter->second);//max                                                                                                                              
  //   Parameter_vec.push_back(iter->first);//time position                                                                                                                     
  //   Parameter_vec.push_back(2);//sigma                                                                                                                                       

  //   par.push_back(0);
  //   par.push_back(10);
  //   par.push_back(1);
  //}

  int lump_num = 0;
  for(std::vector<std::pair<int,double> >::iterator iter=MaxPoints.begin(); iter!=MaxPoints.end(); ++iter){

    //	  if(iter==MaxPoints.begin()){sstm1 << "([" << n-3 <<"]*TMath::Exp(-0.5*std::pow(((x-["<<n-2<<"])/["<<n-1<<"]),2))/(["<<n-1<<"] *sqrt(2*TMath::Pi()))) ";}
    //	  else{sstm1 << "+ ([" << n-3 <<"]*TMath::Exp(-0.5*std::pow(((x-["<<n-2<<"])/["<<n-1<<"]),2))/(["<<n-1<<"] *sqrt(2*TMath::Pi())))";}
    lump_num = lump_num +1;
    if(iter==MaxPoints.begin()){sstm1 << "([" << n-3 <<"]*TMath::Exp(-0.5*std::pow(((x-["<<n-2<<"])/["<<n-1<<"]),2))) ";}                   
    else{sstm1 << "+ ([" << n-3 <<"]*TMath::Exp(-0.5*std::pow(((x-["<<n-2<<"])/["<<n-1<<"]),2))) ";}    
	  fit_string = sstm1.str();          
	  const char* fit_name = fit_string.c_str();
	  std::cout << "test 1 " <<std::endl;
	  //	  TF1 *gausFit = new TF1( "gausfit", fit_name,TimeChargeVector.begin()->first,std::prev(TimeChargeVector.end())->first);           

	  Parameter_vec.push_back(iter->second);//max                                                                                                                          
	  Parameter_vec.push_back(iter->first);//time position                                                                                                                 
	  Parameter_vec.push_back(2);//sigma                                            
 
	  par_min.push_back(MaxPoints[MaxPoints.size()-1].second);
	  //	  par_min.push_back(TimeChargeVector[0].first);
	  par_min.push_back(2);  
	  par_min.push_back(1);

	  par_max.push_back(MaxPoints[0].second);
          //par_max.push_back(TimeChargeVector[TimeChargeVector.size()-1].first);
	  par_max.push_back(2); 
	  par_max.push_back(1);

	  if(iter != MaxPoints.begin() && iter != MaxPoints.end()){
	    for(std::vector<std::pair<int,double> >::iterator jter=iter; jter!=MaxPoints.end(); ++jter){
	      TF1 *gausFit = new TF1( "gausfit", fit_name,TimeChargeVector.begin()->first,std::prev(TimeChargeVector.end())->first);
                Parameter_vec[n-3] = jter->second;//max                                                                                    
	        Parameter_vec[n-2] = jter->first;//time position                                                                           
	        //Parameter_vec[n-1].push_back(2);//sigma                                                                                            
		

		for(Int_t i=0; i<n; ++i){gausFit->SetParameter(i,Parameter_vec[i]); gausFit->SetParLimits(i, par_min[i], par_max[i]);}        
                                 

	    ChargeHist->Fit(gausFit,"R");                                                                                                  
                                  
            std::cout << "Chi2: " << gausFit->GetChisquare() <<std::endl;                                                                  
	    std::vector<double> Parameters;                                
	    for(Int_t i=0; i<n; ++i){std::cout << "Parameter " << i << ": "  << gausFit->GetParameter(i) <<std::endl; Parameters.push_back(gausFit->GetParameter(i));}
	    ChargeHist->Write();     
	    chi2s.push_back(std::make_pair(gausFit->GetChisquare(),Parameters));
   

	    delete gausFit;   
	    }
	  }
	    else{  std::cout << "Max:  " << iter->second << " Time: " << iter->first << std::endl;
	      TF1 *gausFit = new TF1( "gausfit", fit_name,TimeChargeVector.begin()->first,std::prev(TimeChargeVector.end())->first);
	      
	      for(Int_t i=0; i<n; ++i){gausFit->SetParameter(i,Parameter_vec[i]); gausFit->SetParLimits(i, par_min[i], par_max[i]);}
	      
	      ChargeHist->Fit(gausFit,"R");

	      std::cout << "Chi2: " << gausFit->GetChisquare() <<std::endl;
	      std::vector<double> Parameters;
	      for(Int_t i=0; i<n; ++i){std::cout << "Parameter " << i << ": "  << gausFit->GetParameter(i) <<std::endl;
		Parameters.push_back(gausFit->GetParameter(i));}
	      ChargeHist->Write();
	      chi2s.push_back(std::make_pair(gausFit->GetChisquare(),Parameters));
	      delete gausFit;
	    }

	    n=n+3;
	   
  }
            delete ChargeHist;

  double min_chi2 = 1e9;
  std::pair<double,std::vector<double > > min_chi2_info;

  for(std::vector< std::pair<double,std::vector<double > > >::iterator iter=chi2s.begin(); iter!=chi2s.end(); ++iter){
    if(iter->first < min_chi2){min_chi2 = iter->first; min_chi2_info = *iter; }
  }

  double gaus;

  for(unsigned int n=3; n<(min_chi2_info.second).size() +1; n = n+ 3){
    std::vector<std::pair<int,double> > TimeChargeVector_new;

    for(std::vector<std::pair<int,double> >::iterator iter=TimeChargeVector.begin(); iter!=TimeChargeVector.end(); ++iter){

      gaus =  min_chi2_info.second.at(n-3)*TMath::Exp(-0.5*std::pow(((iter->first - min_chi2_info.second.at(n-2))/ min_chi2_info.second.at(n-1)),2)); 
      TimeChargeVector_new.push_back(std::make_pair(iter->first, gaus));
    }
    
      MCHits  Split_hit(hit.Channel(),TimeChargeVector_new,hit.Trackid());
      hit_vec.push_back(Split_hit);


      std::vector<std::pair<int,double> > TimeChargeVector_temp = Split_hit.TimeChargeVec();
      TH1D *ChargeHist = new TH1D("ChargeHist","Charge on the Channel Hist",3000,0,3000);
      for(std::vector<std::pair<int,double> >::iterator jjter=TimeChargeVector_temp.begin(); jjter!=TimeChargeVector_temp.end(); ++jjter){
	ChargeHist->Fill(jjter->first,jjter->second);
      }
      ChargeHist->Write();
      delete ChargeHist;


  }

  return hit_vec;
}
 

Double_t GausFit(Double_t *x, Double_t *par){
  return  par[0]*TMath::Exp(-0.5*std::pow(((x[0]-par[1])/par[2]),2))/(par[2] *sqrt(2*TMath::Pi()));
}


Double_t NGausFit(Double_t *x, Double_t *par){
  std::cout << "test of NGausFit" << std::endl;
  //  if(GiveN == 3){return GausFit(x,par);}
  return GausFit(x,par) +  NGausFit(x,&par[3]);
}

class ana::HitEff : public art::EDAnalyzer {
public:

  HitEff(const fhicl::ParameterSet& pset);

  void analyze(const art::Event& evt);
  void endJob();
private:

  std::string fHitsModuleLabel;
  std::string fMCHitsModuleLabel;

  art::ServiceHandle<cheat::BackTracker> backtracker;
  std::map<int,const simb::MCParticle*> trueParticles;
  std::map<double,std::vector<std::vector<double> > > Efficency_para_map;  //angle the key gives a vector of events with vector of planes 
  std::map<double,std::vector<std::vector<double> > > Efficency_true_map;
  std::map<double,std::vector<std::vector<double> > > Efficiency_hot_map;
  std::map<double,std::vector<std::vector<double> > > Efficiency_muondelta_map;
  int event =0;
  std::map<geo::PlaneID,std::vector<double> > Efficiency_plane_map;
  std::map<geo::PlaneID, std::map<double,std::vector<double> > > Efficiency_angle_map;

  std::vector<double> E1 = {0,0,0};
  std::vector<double> E2 = {0,0,0};
  std::vector<double> E3 = {0,0,0};
  std::vector<double> E4 = {0,0,0};

  //conversion 1ADC = 174.11 electrons
  double ADCconversion = 174.11;
};

ana::HitEff::HitEff(const fhicl::ParameterSet& pset) : EDAnalyzer(pset) {
  fHitsModuleLabel = pset.get<std::string>("HitsModuleLabel");
  fMCHitsModuleLabel = pset.get<std::string>("MCHitsModuleLabel");
  }


void ana::HitEff::analyze(const art::Event& evt) {
  event = event +1;

  TFile *f = new TFile("TimeVsWire","RECREATE");
 
  // const detinfo::DetectorClocks* ts = lar::providerFrom<detinfo::DetectorClocksService>();
  
  //Get the geometry                                                                                                                                                              
  art::ServiceHandle<geo::Geometry> geom;


  //get the reco hits
  art::Handle<std::vector<recob::Hit> > hitHandle;
  std::vector<art::Ptr<recob::Hit> > hits;

   
  if(evt.getByLabel(fHitsModuleLabel, hitHandle)){
    art::fill_ptr_vector(hits, hitHandle);
    std::cout << "There are Hits " << hits.size() << std::endl;
  }  

  //Get the MC hit collections - The collections are all the voxel hits on a wire
  art::Handle<std::vector<sim::MCHitCollection> > MCHitHandle;
  std::vector<art::Ptr<sim::MCHitCollection> > MChits;

  if(evt.getByLabel(fMCHitsModuleLabel, MCHitHandle)){
    art::fill_ptr_vector(MChits, MCHitHandle);
    std::cout << "There are Hits " << MChits.size() << std::endl;
  }


  // Save true tracks                                                                                                                                                            
  int MuonID;
  trueParticles.clear();
  const sim::ParticleList& particles = backtracker->ParticleList();
  for (sim::ParticleList::const_iterator particleIt = particles.begin(); particleIt != particles.end(); ++particleIt) {
    const simb::MCParticle *particle = particleIt->second;
    trueParticles[particle->TrackId()] = particle;
    double mother = particle->Mother();
    if (mother == 0){MuonID = particle->TrackId();}

  }


  TH1D *BckTrackTotChargeHist = new TH1D("TotChargeHist","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
  TH1D *BckTrackTotChargeHist0 = new TH1D("TotChargeHist0","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
  TH1D *BckTrackTotChargeHist1 = new TH1D("TotChargeHist1","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
  TH1D *BckTrackTotChargeHist2 = new TH1D("TotChargeHist2","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);

  int test2=0;
  // Assign a track id to a hit. Now there is contributions from delta rays (id=-1) and the muon (id=1)                                                                          
  std::map<art::Ptr<recob::Hit>,int >    likelyTrackID;

  //std::map<int,art::Ptr<recob::Hit> > MuonOnlyHits;

  // Loop over hits.                                                                                                                                                              
  for(std::vector<art::Ptr<recob::Hit> >::iterator hitIt = hits.begin(); hitIt != hits.end(); ++hitIt){
  double particleEnergy = 0;



    art::Ptr<recob::Hit> hit = *hitIt;
    auto const wire_id = hit->WireID();
    if(wire_id.Plane ==2){++test2;}
    std::vector<sim::TrackIDE> trackIDs = backtracker->HitToTrackID(hit);
    // true_reco_hit ++                                                                                                                                                          
     double  mc_qsum=0;

     // if(trackIDs.size() == 1 && trackIDs.at(0).trackID){MuonOnlyHits.push_back(hit);}
     
    for (unsigned int idIt = 0; idIt < trackIDs.size(); ++idIt) {

      
      //   if(trackIDs.at(idIt).trackID != 1 && trackIDs.at(idIt).numElectrons < 348.22){MuonOnlyHits.push_back(hit);}
 
      //      const simb::MCParticle * MotherParticle = backtracker->TrackIDToMotherParticle(trackIDs.at(idIt).trackID);                                                         
      mc_qsum  +=  trackIDs.at(idIt).numElectrons;

   
      //Assign the track ID to hit that contributes the most energy.                                                                                                             
      if (trackIDs.at(idIt).energy > particleEnergy) {
	particleEnergy = trackIDs.at(idIt).energy;
	likelyTrackID[hit] = trackIDs.at(idIt).trackID;
      }
    }

    double      reco_tot = hit->Integral();
    double      totalchargediff;

    if( wire_id.Plane == 0){
      totalchargediff = (mc_qsum*0.02354  - reco_tot)/(mc_qsum*0.02354 );
      BckTrackTotChargeHist0->Fill(totalchargediff);
    }
    else if( wire_id.Plane == 2){
      totalchargediff = (mc_qsum*0.02354  - reco_tot)/(mc_qsum*0.02354 );
      BckTrackTotChargeHist2->Fill(totalchargediff);
    }
    else{
      totalchargediff = (mc_qsum*0.0213  - reco_tot)/(mc_qsum*0.0213);
      BckTrackTotChargeHist1->Fill(totalchargediff);
    }


    BckTrackTotChargeHist->Fill(totalchargediff);

  }



  BckTrackTotChargeHist->Draw("same");
  BckTrackTotChargeHist->SetName("BckTrackTotalCharge");
  BckTrackTotChargeHist->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  BckTrackTotChargeHist->SetTitle("BckTrackTotal Charge ");
  BckTrackTotChargeHist->Write();

  BckTrackTotChargeHist0->Draw("same");
  BckTrackTotChargeHist0->SetName("BckTrackTotalCharge 0");
  BckTrackTotChargeHist0->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  BckTrackTotChargeHist0->SetTitle("BckTrackTotal Charge0 ");
  BckTrackTotChargeHist0->Write();

  BckTrackTotChargeHist1->Draw("same");
  BckTrackTotChargeHist1->SetName("BckTrackTotalCharge 1");
  BckTrackTotChargeHist1->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  BckTrackTotChargeHist1->SetTitle("BckTrackTotal Charge ");
  BckTrackTotChargeHist1->Write();

  BckTrackTotChargeHist2->Draw("same");
  BckTrackTotChargeHist2->SetName("BckTrackTotalCharge 2");
  BckTrackTotChargeHist2->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  BckTrackTotChargeHist2->SetTitle("BckTrackTotal Charge ");
  BckTrackTotChargeHist2->Write();





  //Define the graphs that show the WireVsTime
  gInterpreter->GenerateDictionary("std::map<geo::PlaneID,TGraph*>","TGraph.h;map");
  std::map<geo::PlaneID,TGraph*> Graph_map;
  std::map<geo::PlaneID,TGraph*> Graph_mapm1;
  std::map<geo::PlaneID,TGraph*> Graph_map1;


  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
    Graph_map[plane_id] = new TGraph(0);
    Graph_mapm1[plane_id] = new TGraph(0);
    Graph_map1[plane_id] = new TGraph(0);
    }
  std::cout << "Graph map Size: " <<Graph_map.size() << std::endl;


  std::vector<double> channels;
  std::vector<double> times;

 
  
  const std::vector<sim::MCHitCollection> &mchits_v = *MCHitHandle;
    
  int mchit_num=0;
  int channel_num;
  int logic_int;

  double peak_time=0;
  double charge_cut=348.22; //2ADCs in Electrons 
  int dt_threshold =2;
  int test=0;
  
  std::map<unsigned int,std::vector<MCHits> > channel_MCHit_map; 
  std::vector<MCHits> MC_vec;
  
  //loop over the mchit collections (i.e loop over all the wires with MC hits) and find how many true hits we have.
  for(auto const& mchits : mchits_v) {
    
    TH1D *ChargeHist = new TH1D("ChargeHist","Charge on the Channel Hist",3000,0,3000);

   
    geo::PlaneID plane = geom->ChannelToWire(mchits.Channel()).at(0).planeID();
    channel_num = mchits.Channel();
    channels.push_back(channel_num);
   
    // Set the logic to 0 for the collection this allows that one hit is counted from the collection
       logic_int=0;


    std::vector<std::pair<int,double> > total_time_charge_vec;
    std::vector<std::pair<int,double> > time_charge_vec;
    std::map<int, double> charge_map;
    std::map<int,std::vector<std::pair<int,double> > > TrackIDtoMCHitMap;

    //loop of the MChits in the collection  
    for(auto const& mchit : mchits){

      ++test;
      total_time_charge_vec.push_back(std::make_pair(mchit.PeakTime(),mchit.Charge(true))); 
      charge_map[mchit.PeakTime()] =  charge_map[mchit.PeakTime()] +  mchit.Charge(true);
      ChargeHist->Fill(mchit.PeakTime(),mchit.Charge(true));
      //      std::cout << " Charge: " << mchit.Charge(true) << " Time: " << mchit.PeakTime() << " Track ID: " << mchit.PartTrackId()<< " Channel: " << channel_num << std::endl;
      TrackIDtoMCHitMap[mchit.PartTrackId()].push_back(std::make_pair(mchit.PeakTime(),mchit.Charge(true)));
      //      if(mchit.Charge(true)>charge_cut){MCHitVec.push_back(mchit);}  
      peak_time = mchit.PeakTime();

        // Add points for the WireVsTime Graph 
	 if(mchit.PartTrackId() == -1){
	   Graph_mapm1[plane]->SetPoint(Graph_mapm1[plane]->GetN(),channel_num, peak_time);
	   // std::cout<< "channel_num" << channel_num<< " ID " << mchit.PartTrackId() << " Peak Time " << peak_time  << std::endl;
	 }
      
	 if(mchit.PartTrackId() == 1){
	   Graph_map1[plane]->SetPoint(Graph_map1[plane]->GetN(),channel_num, peak_time);
	   // std::cout << "channel_num" << channel_num << " ID " << mchit.PartTrackId() << " Peak Time " << peak_time  << std::endl; 
	 }
	 //	 std::cout << "channel_num " << channel_num << " ID " << mchit.PartTrackId() << " Peak Time " << peak_time  << " Charge: " << mchit.Charge(true)<<std::endl;
	   Graph_map[plane]->SetPoint(Graph_map[plane]->GetN(),channel_num, peak_time);
      

    } //MC Hit Loop
    



    // std::cout << " ###########################################" << std::endl;
    // std::cout << "channel_num " << channel_num << std::endl; 

    //This method for defining true hits was to find the chi2 of NGaussian Fits until the chi2 was no longer getting smaller. The Parameters need to be defined to get this to work not sure how
    //###################################################################################################  
    //    double chi2 = 99999999999999;
    //    double chi2_new = 0;
    //    int n = 3;
    //    std::vector<double> Parameter_vec;
    //    Double_t nextmax = ChargeHist->GetMaximumBin();
    //    std::cout << "GetMaxBin " << nextmax <<std::endl;
    //    Double_t max = ChargeHist->GetMaximum();
    //    std::cout << "GetMax " << max << std::endl;
    //    TF1 *gausFit = new TF1( "gausfit", GausFit,total_time_charge_vec.begin()->first,std::prev(total_time_charge_vec.end())->first,n);
    //    // Parameter_vec.push_back(nextmax);
    //    //Parameter_vec.push_back(1);
    //    //Parameter_vec.push_back(nextmax);
    //    //for(Int_t i=0; i<n; ++i){gausFit->SetParameter(i,Parameter_vec[i]);}
    //    std::cout << "Fit Test" << std::endl;    
    //    ChargeHist->Fit(gausFit,"R");
    // std::string fit_string;
    // std::stringstream sstm1;
    // sstm1 << "([" << n-3 <<"]*TMath::Exp(-0.5*std::pow(((x-["<<n-2<<"])/["<<n-1<<"]),2))/(["<<n-1<<"] *sqrt(2*TMath::Pi()))) ";
    //    while(chi2 > chi2_new){ 
    //      n = n+3;
    //      chi2 = gausFit->GetChisquare();
    //      sstm1 <<  "+ ([" << n-3 <<"]*TMath::Exp(-0.5*std::pow(((x-["<<n-2<<"])/["<<n-1<<"]),2))/(["<<n-1<<"] *sqrt(2*TMath::Pi())))";
    //      fit_string = sstm1.str();
    //      const char* fit_name = fit_string.c_str();
    //      TF1 *gausFit = new TF1( "gausfit", fit_name,total_time_charge_vec.begin()->first,std::prev(total_time_charge_vec.end())->first);
    //      //Parameter_vec.push_back(nextmax);
    //      //Parameter_vec.push_back(1);
    //      //Parameter_vec.push_back(nextmax);
    //      //for(Int_t i=0; i<n; ++i){gausFit->SetParameter(i,Parameter_vec[i]);}
    //      ChargeHist->Fit(gausFit,"R");
    //      std::cout << "Chi2: " << gausFit->GetChisquare() <<std::endl;
    //      nextmax = ChargeHist->GetMaximum(nextmax);
    //      chi2_new = gausFit->GetChisquare();
    //    }
    //    ChargeHist->Write();
    //    delete ChargeHist;
    //    delete gausFit;


    //This method takes mutliple cuts of the hits to try to take a true hit which is mimics a reco hit
    //###################################################################################################  
    // if(total_time_charge_vec.size() == 0){continue;}
    // std::vector<std::pair<int,double> >::iterator prev_iter=total_time_charge_vec.begin();
    // std::vector<std::pair<int,double> >::iterator next_iter;
    // std::vector<std::pair<int,double> >::iterator prev_iter_temp;
    
    // for(std::vector<std::pair<int,double> >::iterator iter=total_time_charge_vec.begin(); iter != total_time_charge_vec.end(); ++iter){


    //   next_iter =std::next(iter);

    //   if(prev_iter ==total_time_charge_vec.begin()){prev_iter = iter; continue;}
    //   else if(charge_map[std::prev(iter)->first]  < charge_cut && std::prev(prev_iter)->first != iter->first && iter != std::prev(total_time_charge_vec.end())){}
    //   else if(std::prev(prev_iter)->first == iter->first){prev_iter = std::prev(prev_iter);}
    //   else{prev_iter = std::prev(iter);}

    //   time_charge_vec.push_back(*iter);

    //   if(charge_map[iter->first]  < charge_cut  && iter != std::prev(total_time_charge_vec.end())){continue;}
    
    //   if(logic_int ==0){prev_iter = std::prev(iter);}
    //   logic_int=1;

    //   //If there is a gap of 1 tick make a new hit
    // 	if(prev_iter->first < iter->first - 1){
    // 	  MCHits MC_hit(channel_num,time_charge_vec,iter->first); channel_MCHit_map[channel_num].push_back(MC_hit); MC_vec.push_back(MC_hit); time_charge_vec.clear(); 
    // 	} 
    //   //If at the end of the charge vector this a new hit
    // 	else if(iter == std::prev(total_time_charge_vec.end())){
    // 	  MCHits  MC_hit(channel_num,time_charge_vec,iter->first); channel_MCHit_map[channel_num].push_back(MC_hit); MC_vec.push_back(MC_hit); time_charge_vec.clear();
    // 	}
    //   //If there is a minimum point create a new hit 
    // 	 else if(charge_map[prev_iter->first] >= charge_map[(iter)->first] && charge_map[next_iter->first] >= charge_map[(iter)->first]){

    // 	   prev_iter_temp = prev_iter;
    // 	   while(charge_map[next_iter->first] == charge_map[(iter)->first]){ next_iter = std::next(next_iter);}
    // 	   while(charge_map[prev_iter_temp->first] == charge_map[(iter)->first]){ prev_iter_temp = std::prev(prev_iter_temp);}
    // 	   if(charge_map[prev_iter_temp->first] > charge_map[(iter)->first] && charge_map[next_iter->first] > charge_map[(iter)->first]){
    //        if(prev_iter->first == iter->first){continue;}
    // 	   MCHits MC_hit(channel_num,time_charge_vec,iter->first); channel_MCHit_map.push_back(MC_hit); MC_vec.push_back(MC_hit);time_charge_vec.clear();
    // 	   }
    // 	 }
    // }
    
    //This method takes mutliple takes delta ray and muon contribution seperately placing a cut on the dt of the muon and charge threshold.                                       
    //###################################################################################################

  //   if(total_time_charge_vec.size() == 0){continue;}                                                                                                                             

    for(std::map<int,std::vector<std::pair<int,double> > >::iterator jter=TrackIDtoMCHitMap.begin(); jter!=TrackIDtoMCHitMap.end(); ++jter){

      std::vector<MCHits> MC_vec_temp;
      std::vector<std::pair<int,double> >::iterator prev_iter=(jter->second).begin();
      std::vector<std::pair<int,double> >::iterator next_iter;
            
      for(std::vector<std::pair<int,double> >::iterator iter=(jter->second).begin(); iter!=(jter->second).end(); ++iter){ 
      
      next_iter =std::next(iter);                                                                                                                                                
      if(iter == (jter->second).begin()){prev_iter = iter; if(iter->second  > charge_cut){time_charge_vec.push_back(*iter);} continue;}                                            
      else if(std::prev(prev_iter)->first == iter->first && prev_iter != (jter->second).begin()){prev_iter = std::prev(prev_iter);}                                               
      else{prev_iter = std::prev(iter);}                                                                                                                                        


         if(charge_map[iter->first]  < charge_cut  && iter != std::prev((jter->second).end())){continue;}                                                                  
	 //    if(iter->second  < charge_cut  && iter != std::prev((jter->second).end())){continue;}
      else if(iter->second  < charge_cut  && iter == std::prev((jter->second).end())){}
      else{time_charge_vec.push_back(*iter);}//std::cout<< "Time: " << iter->first << " Charge: " << iter->second << std::endl; }

      if(logic_int ==0){prev_iter = std::prev(iter);}                                                                                                                          
      logic_int=1;                                                                                                                                                               


      //If at the end of the charge vector this a new hit                                                                                                                        
       if(iter == std::prev((jter->second).end())){                                                                                                                 
       if(time_charge_vec.size() == 0){continue;}
       MCHits  MC_hit(channel_num,time_charge_vec,jter->first); //channel_MCHit_map[channel_num].push_back(MC_hit); 
       MC_vec_temp.push_back(MC_hit); time_charge_vec.clear();
       }
     //If there is a gap of 1 tick make a new hit                                                                                                                                   
     else  if(next_iter->first - iter->first > 1){
       MCHits MC_hit(channel_num,time_charge_vec,jter->first); //channel_MCHit_map[channel_num].push_back(MC_hit);                                                                
       MC_vec_temp.push_back(MC_hit); time_charge_vec.clear();
     }                                                                                                                                                                           
    


     }//iter
  
     for(unsigned int i=0; i< MC_vec_temp.size(); ++i){

       std::vector<std::pair<int,double> > timeChargeVec = MC_vec_temp[i].TimeChargeVec();
       std::vector<std::pair<int,double> > MaxPoints;
       std::vector<std::pair<int,double> >::iterator prev_kter;
       std::vector<std::pair<int,double> >::iterator next_kter;
       for(std::vector<std::pair<int,double> >::iterator kter = timeChargeVec.begin(); kter!=timeChargeVec.end(); ++kter){
       

  	 if(kter == timeChargeVec.begin()){continue;}
  	 else{prev_kter = std::prev(kter);}

  	 if(kter == std::prev(timeChargeVec.end())){continue;}    
  	 else{next_kter = std::next(kter);}

  	   if(prev_kter->second < kter->second  && next_kter->second < kter->second){
  	     MaxPoints.push_back(std::make_pair(kter->first,kter->second));
  	   }
       }
  	 std::vector<MCHits> MC_hits = CutHit(MC_vec_temp[i],MaxPoints);
  	 channel_MCHit_map[channel_num].insert(channel_MCHit_map[channel_num].end(),  MC_hits.begin(),  MC_hits.end());
  	 //	 MC_vec.insert(MC_vec.end(),  channel_MCHit_map[channel_num].begin(),  channel_MCHit_map[channel_num].end());

     }
       
  }
     

  
  

  
    //  for(std::map<int,std::vector<std::pair<int,double> > >::iterator iter=TrackIDtoMCHitMap.begin(); iter!=TrackIDtoMCHitMap.end(); ++iter){
      
    // MCHits MC_hit(channel_num,iter->second,iter->first); channel_MCHit_map[channel_num].push_back(MC_hit); //MC_vec.push_back(MC_hit);

    //}
    std::vector<std::vector<MCHits>::iterator> erase_hits_vec;
    std::vector<MCHits> merged_hits;

    // if(channel_MCHit_map[channel_num].size()<2){continue;}

    if(channel_MCHit_map[channel_num].size()>1){
    for(std::vector<MCHits>::iterator iter=channel_MCHit_map[channel_num].begin(); iter !=channel_MCHit_map[channel_num].end(); ++iter){ 
      if(TMath::Abs(iter->PeakTime() - std::prev(iter)->PeakTime()) < dt_threshold && iter!=channel_MCHit_map[channel_num].begin()){
	//	std::cout << "(iter->PeakTime(): " << iter->PeakTime() << "   std::prev(iter)->PeakTime(): " << std::prev(iter)->PeakTime() <<std::endl;  
	MCHits MergeHit =  MergeHits(*iter,*std::prev(iter));
	

	if (std::find(erase_hits_vec.begin(), erase_hits_vec.end(), iter) == erase_hits_vec.end()) {erase_hits_vec.push_back(iter);}
	if (std::find(erase_hits_vec.begin(), erase_hits_vec.end(), std::prev(iter)) == erase_hits_vec.end()) {erase_hits_vec.push_back(std::prev(iter));}

	merged_hits.push_back(MergeHit);
      }

    }
    

    for(std::vector<std::vector<MCHits>::iterator>::iterator iter=erase_hits_vec.begin(); iter!=erase_hits_vec.end();++iter){

      channel_MCHit_map[channel_num].erase(*iter);
      }
    }
    channel_MCHit_map[channel_num].insert(channel_MCHit_map[channel_num].end(),  merged_hits.begin(),  merged_hits.end());
    MC_vec.insert(MC_vec.end(),  channel_MCHit_map[channel_num].begin(),  channel_MCHit_map[channel_num].end());
   
    int MUON_Hits =0;
    if(channel_MCHit_map[channel_num].size()>1){
    for(std::vector<MCHits>::iterator iter=channel_MCHit_map[channel_num].begin(); iter !=channel_MCHit_map[channel_num].end(); ++iter){
      if(iter->Trackid() != -1){ ++MUON_Hits;}
     
    }}
    if(MUON_Hits>1){std::cout<<"This is the event!"<<std::endl;}
  } //MC HitCollection Loop


  std::vector<MCHits> MuonMCHits;
  std::vector<MCHits>::iterator iter=MC_vec.begin();
  //  for(std::vector<MCHits>::iterator iter=MC_vec.begin(); iter!=MC_vec.end(); ++iter){
  while(iter != MC_vec.end()) {  
    if(iter->PeakCharge() < charge_cut){iter = MC_vec.erase(iter); continue;}
    std::cout << " Peak Time:" << iter->PeakTime() << " Peak Charge: " << iter->PeakCharge() << " Channel: " << iter->Channel() << "TrackID: " << iter->Trackid()  <<std::endl;
    std::vector<std::pair<int,double> > timechargevec = iter->TimeChargeVec();
    for(std::vector<std::pair<int,double> >::iterator  jter=timechargevec.begin(); jter!=timechargevec.end(); ++jter){
         std::cout<< "Charge " << jter->second << " Time: " << jter->first << std::endl;
    }

    if(iter->Trackid() != -1){MuonMCHits.push_back(*iter);}
    ++iter;
  }

  //Create the the names of the of the graphs and write to file. 
  std::string map_string;
  std::string map_stringm1;
  std::string map_string1;


  std::cout << "Graph map Size: " <<Graph_map.size() << std::endl;
  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
    
    std::stringstream sstm1;
    sstm1 << "PlaneID:" << plane_id;
    map_string = sstm1.str();
    const char* name = map_string.c_str();

    std::stringstream sstm2;
    sstm2 << " -1 PlaneID:" << plane_id;
    map_stringm1 = sstm2.str();
    const char* namem1 = map_stringm1.c_str();

    std::stringstream sstm3;
    sstm3 << " 1 PlaneID:" << plane_id;
    map_string1 = sstm3.str();
    const char* name1 = map_string1.c_str();

    Graph_map[plane_id]->Draw("AP");
    Graph_map[plane_id]->SetName(name);
    Graph_map[plane_id]->GetXaxis()->SetTitle("Channel Number");
    Graph_map[plane_id]->GetYaxis()->SetTitle("Time (Ticks?)");
    Graph_map[plane_id]->SetTitle(name);
    Graph_map[plane_id]->Write();

    Graph_mapm1[plane_id]->Draw("AP");
    Graph_mapm1[plane_id]->SetName(namem1);
    Graph_mapm1[plane_id]->GetXaxis()->SetTitle("Channel Number");
    Graph_mapm1[plane_id]->GetYaxis()->SetTitle("Time (Ticks?)");
    Graph_mapm1[plane_id]->SetTitle(namem1);
    Graph_mapm1[plane_id]->Write();

    Graph_map1[plane_id]->Draw("AP");
    Graph_map1[plane_id]->SetName(name1);
    Graph_map1[plane_id]->GetXaxis()->SetTitle("Channel Number");
    Graph_map1[plane_id]->GetYaxis()->SetTitle("Time (Ticks?)");
    Graph_map1[plane_id]->SetTitle(name1);
    Graph_map1[plane_id]->Write();


  }


  TGraph* ADCGraph = new TGraph(0);  
  TH1D *PeakChargeHist = new TH1D("PeakChargeHist","Ratio difference between reco and MC Peak Amp",500,-1.5,1.5);
  TH1D *TotChargeHist = new TH1D("TotChargeHist","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);  
  TH1D *TotChargeHist0 = new TH1D("TotChargeHist0","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
  TH1D *TotChargeHist1 = new TH1D("TotChargeHist1","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
  TH1D *TotChargeHist2 = new TH1D("TotChargeHist2","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);

  double peakchargediff;
  double totalchargediff;

  std::vector<art::Ptr<recob::Hit> > noise_hits;
  std::map<art::Ptr<recob::Hit>,int>  reco_to_MC_map;

  int test1=0;
  // Compare RecoHits to True Hits.
  // Loop over RecoHit                                                                                                                                                             
  for(size_t hit_index=0; hit_index<hits.size(); ++hit_index) {


    auto const hit = hits.at(hit_index);
    auto const wire_id = hit->WireID();


    // Figure out channel & retrieve MCHitCollection for this channel                                                                                                              
    auto ch = geom->PlaneWireToChannel(wire_id.Plane,
				      wire_id.Wire,
				      wire_id.TPC,
				      wire_id.Cryostat);

    if(mchits_v.size() <= ch )
      throw cet::exception(__PRETTY_FUNCTION__)
	<< "Channel "<<ch<<" not found in MCHit vector!"<<std::endl;

    
    // Found MCHitCollection                                                                                                                                                       
    auto const& mchits = mchits_v.at(ch);
    std::vector<MCHits> MCHit_vec = channel_MCHit_map[ch];

    if( ch != mchits.Channel() )
      throw cet::exception(__PRETTY_FUNCTION__)
	<< "MCHit channel & vector index mismatch: "
	<< mchits.Channel() << " != " << ch << std::endl;



    double reco_q  = hit->PeakAmplitude();
    double reco_tot = hit->Integral();
    double mc_qsum = 0;
    double mc_q    = 0;
    //double   dt_min  = 0;
    double abs_dt_min = 1e9;

    
//    std::cout << "####################################"  << std::endl;
    //std::cout << "Channel: " << ch  << std::endl;
    //    std::cout << "Start tick: " << hit->StartTick() << " End TicK: " << hit->EndTick() << " Peak Time " << hit->PeakTime()<<std::endl;

    for(std::vector<MCHits>::iterator iter=  MCHit_vec.begin(); iter !=  MCHit_vec.end(); ++iter){
      
      //      if((*start_iter).PartTrackId() == likelyTrackID[hit]){

      double dt =  hit->PeakTime() - iter->PeakTime();
      double abs_dt = dt;
      // std::cout << " The reco peak time is: " << peak_time.PeakTime() << std::endl;
      //std::cout << " ID " <<  (*start_iter).PartTrackId() <<  " The MC Peak time is: " << (*start_iter).PeakTime() << std::endl;
      //std::cout << "dt " << dt << " Charge: " << (*start_iter).Charge(true) << std::endl;
      
      if(abs_dt<0) abs_dt *= -1;
      //if(abs_dt>5){std::cout << "interesting event"<<std::endl;}

      if(abs_dt < abs_dt_min) {
	abs_dt_min = abs_dt;
	//	dt_min     = dt;
	mc_q   = iter->PeakCharge(); 
	mc_qsum = iter->TotalCharge(); 
      }
      else if( abs_dt == abs_dt_min) {
	mc_q = mc_q + iter->PeakCharge();
        mc_qsum = mc_qsum + iter->TotalCharge();
      }
      

      //}
      //Find the MCHits that have been matched to a reco hit and get rid of them. 
      //      mcbighits.erase(*start_iter);
      
 
      //      std::cout << "mc_q: " << mc_q << " sum: " << mc_qsum << std::endl;


   
    }// MC Hits Loop
    
    //    reco_to_MC_map[hit] =  mcbighits[std::make_pair(MCHit_peak,ch)]; 
    std::cout << "abds_td_min: " << abs_dt_min << "Charge: " << mc_q <<"  channel  " <<  ch <<std::endl;
    //Take Note of any Noise Hits.
    if(abs_dt_min > 20){
      std::cout << "intersting event" <<std::endl;
      noise_hits.push_back(hit);
    }
    else{

      if( wire_id.Plane == 0){
      peakchargediff = (mc_q* 0.02354 - reco_q)/(mc_q* 0.02354);
      totalchargediff = (mc_qsum*0.02354  - reco_tot)/(mc_qsum*0.02354 );
      TotChargeHist0->Fill(totalchargediff);
      }
      else if( wire_id.Plane == 2){
	++test1;
	peakchargediff = (mc_q* 0.02354 - reco_q)/(mc_q* 0.02354);
	totalchargediff = (mc_qsum*0.02354  - reco_tot)/(mc_qsum*0.02354 );
	TotChargeHist2->Fill(totalchargediff);
      }
	else{
	peakchargediff = (mc_q* 0.0213 - reco_q)/(mc_q* 0.0213);
	totalchargediff = (mc_qsum*0.0213  - reco_tot)/(mc_qsum*0.0213);
	TotChargeHist1->Fill(totalchargediff);
      }

      PeakChargeHist->Fill(peakchargediff);
      TotChargeHist->Fill(totalchargediff);
      if( wire_id.Plane == 0){
      ADCGraph->SetPoint(ADCGraph->GetN(),mc_qsum, reco_tot);
      }
      }
  
    // std::cout << "Total Charge: " << mc_qsum*0.0224426  << " Reco Charge: " << reco_tot <<" channel num " << ch << std::endl;
    //    if(mc_qsum*0.0224426 <  reco_tot - 100){std::cout << "mcCharge under"<< std::endl;}
    //if(mc_qsum*0.0224426 >  reco_tot + 100){std::cout <<"mcCharge over"<< std::endl;}
    //    std::cout << "Wire ID: " << wire_id << std::endl;

  } // Reco Hits Loop

  std::cout << "test1 " << test1 << " test2 " << test2 <<std::endl;

  ADCGraph->Draw("P");
  ADCGraph->SetName("ADCGraph");
  ADCGraph->GetXaxis()->SetTitle("MC Charge (ADC)");
  ADCGraph->GetYaxis()->SetTitle("Reco Charge (ADC)");
  ADCGraph->SetTitle("ADC Graph");
  TF1 *fit = new TF1("fit","pol1");
  ADCGraph->Fit(fit,"+rob=0.75");
  std::cout << fit->GetParameter(0) << std::endl;
  ADCGraph->Write();

  PeakChargeHist->Draw("same");
  PeakChargeHist->SetName("Peak Charge");
  PeakChargeHist->GetXaxis()->SetTitle("(mc_peak - reco_peak)/mc_peak");
  PeakChargeHist->SetTitle("Peak Charge");
  PeakChargeHist->Write();

  TotChargeHist->Draw("same");
  TotChargeHist->SetName("TotalCharge");
  TotChargeHist->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  TotChargeHist->SetTitle("Total Charge ");
  TotChargeHist->Write();

  TotChargeHist0->Draw("same");
  TotChargeHist0->SetName("TotalCharge 0");
  TotChargeHist0->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  TotChargeHist0->SetTitle("Total Charge ");
  TotChargeHist0->Write();

  TotChargeHist1->Draw("same");
  TotChargeHist1->SetName("TotalCharge 1");
  TotChargeHist1->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  TotChargeHist1->SetTitle("Total Charge ");
  TotChargeHist1->Write();

  TotChargeHist2->Draw("same");
  TotChargeHist2->SetName("TotalCharge 2");
  TotChargeHist2->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  TotChargeHist2->SetTitle("Total Charge ");
  TotChargeHist2->Write();


  //  f->Close();


 //  //Collect the MC hits missed in the reconstruction. 
 //  std::map<int, std::vector<std::pair<sim::MCHit,int> > > missed_hits;

 //  //The MCHits left are the ones not associated to a reco hit. Collect ones together assigned to my definition of a MC Hit.
 //  for(std::map<std::pair<sim::MCHit,int>, int>::iterator it = mcbighits.begin(); it!=mcbighits.end(); ++it){
 //      missed_hits[it->second].push_back(it->first); 
 //      //     std::cout << "Hit number is: " << it->second << std::endl;
 // }
 //  std::cout << "noise hits: " << noise_hits.size() << std::endl;
 //  std::cout << "mcbighits: " << mcbighits.size() << " test " << test << " missed_hits " << missed_hits.size() <<std::endl;
 //  TH1D *TotChargeHist0new = new TH1D("TotChargeHist0new","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
 //  TH1D *TotChargeHist1new = new TH1D("TotChargeHist1new","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
 //  TH1D *TotChargeHist2new = new TH1D("TotChargeHist2new","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);  


// for(size_t hit_index=0; hit_index<hits.size(); ++hit_index) {

//     double Charge=0;
//     auto const hit = hits.at(hit_index);
//     auto const wire_id = hit->WireID();   
//     double reco_tot = hit->Integral();
//     double totalchargediff;

//    for(unsigned int it=0; it<missed_hits[reco_to_MC_map[hit]].size(); ++it){
//      Charge = Charge +   missed_hits[reco_to_MC_map[hit]].at(it).first.Charge(true);
//      std::cout << "Charge: " << missed_hits[reco_to_MC_map[hit]].at(it).first.Charge(true) << std::endl;
//    }
//    std::cout << "Total MC Charge is: " << Charge  << " Reco charge: " << reco_tot << " WireID: " << wire_id <<  " hit num: " << reco_to_MC_map[hit]  << std::endl; 
//    std::cout << "total charge diff: " <<  (Charge*0.02354  - reco_tot)/(Charge*0.02354 ) << std::endl;
//      if( wire_id.Plane == 0){
//        totalchargediff = (Charge*0.02354  - reco_tot)/(Charge*0.02354 );
//        TotChargeHist0new->Fill(totalchargediff);
//      }
//      else if( wire_id.Plane == 2){
//        totalchargediff = (Charge*0.02354  - reco_tot)/(Charge*0.02354 );
//        TotChargeHist2new->Fill(totalchargediff);
//      }
//      else{
//        totalchargediff = (Charge*0.0213  - reco_tot)/(Charge*0.0213);
//        TotChargeHist1new->Fill(totalchargediff);
//      }


   
//   }


 // TotChargeHist0new->Draw("same");
 // TotChargeHist0new->SetName("TotalCharge 0 new");
 // TotChargeHist0new->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
 // TotChargeHist0new->SetTitle("Total Charge ");
 // TotChargeHist0new->Write();

 // TotChargeHist1new->Draw("same");
 // TotChargeHist1new->SetName("TotalCharge 1 new");
 // TotChargeHist1new->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
 // TotChargeHist1new->SetTitle("Total Charge ");
 // TotChargeHist1new->Write();

 // TotChargeHist2new->Draw("same");
 // TotChargeHist2new->SetName("TotalCharge 2 new");
 // TotChargeHist2new->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
 // TotChargeHist2new->SetTitle("Total Charge new");
 // TotChargeHist2new->Write();


 f->Close();


  // double Total_charge_missed = 0;

  // // Sum up the Charge Missed and the number of hits missed. 
  // for(std::map<int,std::vector<std::pair<sim::MCHit,int> > >::iterator it = missed_hits.begin(); it!=missed_hits.end(); ++it){
  //   for(unsigned int iter=0; iter<(it->second).size(); ++iter){
  //     //      std::cout << "The Charge missed is: " << (it->second).at(iter).first.Charge(true) << std::endl;
  //     Total_charge_missed = Total_charge_missed + (it->second).at(iter).first.Charge(true);
  //   }
  // }
  // std::cout << "The number of hits that were missed: " <<  missed_hits.size() << std::endl;
  // std::cout << "Charge Missed: " << Total_charge_missed << std::endl;

  //Find the trajectory of the particle;
  const simb::MCParticle * Muon =  backtracker->TrackIDToParticle(MuonID);
  
  //Start and End positions of the particle.
  const double EndY   = Muon->EndY();
  const double EndZ   = Muon->EndZ();
  const double StartY = Muon->Vy();
  const double StartZ = Muon->Vz();

  //Use the true muon momentum to find the angle theta yz 
  const double PY = Muon->Py();
  const double PZ = Muon->Pz();
  const double angle = TMath::ATan(PY/PZ); 



  //Get the number of Traj points to loop over 
  unsigned int TrajPoints = Muon->NumberTrajectoryPoints();

  //Define the the start position and end position in the TPC
  double StartY_TPC=StartY;
  double StartZ_TPC=StartZ;
  double EndY_TPC=EndY;
  double EndZ_TPC=EndZ;

  std::map<geo::TPCID,std::vector<double> > StartCood_TPCs; 
  std::map<geo::TPCID,std::vector<double> > EndCood_TPCs;

  
  
  for (geo::TPCID const& tpcID: geom->IterateTPCIDs()){
    
    StartCood_TPCs[tpcID].push_back(0);
    StartCood_TPCs[tpcID].push_back(0);

    //Loop over the trajectory points (they are in order). Loop to find the start point.  
    for(unsigned int TrajPoints_it=0; TrajPoints_it<TrajPoints; ++TrajPoints_it){
    
      //Find the vertex of the vector 
      const TLorentzVector PositionTrajP = Muon->Position(TrajPoints_it);
      double vtx[3] = {PositionTrajP.X() ,PositionTrajP.Y(), PositionTrajP.Z()};
   
      //Find in the vertex is in the TPC. If so make it the start point. Change the logic_Int so that the start point remains the first vertex that went into the tpc  
      geo::TPCID idtpc = geom->FindTPCAtPosition(vtx);
      
      if(idtpc != tpcID){continue;}
      
      if (geom->HasTPC(idtpc)){
	
	StartCood_TPCs[tpcID].at(0) = PositionTrajP.Y();
	StartCood_TPCs[tpcID].at(1) = PositionTrajP.Z();
	break;
      }
    }
  }


  for (geo::TPCID const& tpcID: geom->IterateTPCIDs()){

    EndCood_TPCs[tpcID].push_back(0);
    EndCood_TPCs[tpcID].push_back(0);
    //Loop backwards through the trajectory points to find the end point
    for(unsigned int TrajPoints_it=TrajPoints; TrajPoints_it>0; --TrajPoints_it){

      //Find the vertex of the vector 
      const TLorentzVector PositionTrajP = Muon->Position(TrajPoints_it);
      double vtx[3] = {PositionTrajP.X(), PositionTrajP.Y(), PositionTrajP.Z()};

      //Find in the vertex is in the TPC. If so make it the end point. Change the logic_Int so that the start point remains the first vertex that went into the tpc    
      geo::TPCID idtpc = geom->FindTPCAtPosition(vtx);
      if(idtpc != tpcID){continue;}
      if (geom->HasTPC(idtpc)){
	EndCood_TPCs[tpcID].at(0) = PositionTrajP.Y();
	EndCood_TPCs[tpcID].at(1) = PositionTrajP.Z();
	break;
      }
    }
  }

  
  // Metrics to determine if the track crosses the wires. 
  double u;
  double t;
  double rcrossx;
  std::map<geo::PlaneID,std::vector<int> > WiresCrossed_map; //Plane as key, Wires as vector 
  
  int Wirenum=0;
  // Loop through the Wires determine the start and end poisition and compare vector to vector of the true track.
  for(geo::WireID const& wire_id: geom->IterateWireIDs()){
    
    
    const geo::WireGeo * Wire = &(geom->Wire(wire_id));
    const TVector3 WireStart =  Wire->GetStart();
    const TVector3 WireEnd = Wire->GetEnd();
    const geo::TPCID TPC = wire_id.asTPCID();
   
    EndY_TPC = EndCood_TPCs[TPC].at(0);
    EndZ_TPC = EndCood_TPCs[TPC].at(1);
    StartY_TPC = StartCood_TPCs[TPC].at(0);
    StartZ_TPC = StartCood_TPCs[TPC].at(1);
    
    rcrossx = ((EndZ_TPC - StartZ_TPC)*(WireEnd.Y()-WireStart.Y())) - ((WireEnd.Z()-WireStart.Z())*(EndY_TPC - StartY_TPC)); 

    if(rcrossx == 0){continue;}
    u = (((WireStart.Z()- StartZ_TPC)*(EndY_TPC-StartY_TPC)) - ((EndZ_TPC-StartZ_TPC)*(WireStart.Y()-StartY_TPC)))/rcrossx;
    t = (((WireStart.Z() - StartZ_TPC)*(WireEnd.Y()-WireStart.Y())) - ((WireEnd.Z()-WireStart.Z())*(WireStart.Y()-StartY_TPC)))/rcrossx;


    //    std::cout <<"WireStartY = " << WireStart.Y() << " WireEndY = " << WireEnd.Y() << "PlaneID = " << wire_id  << std::endl;
    //std::cout <<"WireStartZ = "<< WireStart.Z() << " WireEndZ = " << WireEnd.Z() << "PlaneID = " << wire_id  << std::endl ;

    
    if(u<= 1 && t<=1 && u>= 0 && t>=0){

      // add wire to the wirecrossed map if it crosses  
      //if(wire_id.TPC == 1){  
      int wire_no = wire_id.Wire;
      geo::PlaneID planeID = wire_id.planeID(); 
      WiresCrossed_map[planeID].push_back(wire_no);
      ++Wirenum;
      //}
    
    }
   

  }



  int channels_see_hit=0;

  std::vector<std::vector<double> >Efficency_para_vec(3, std::vector<double>(0));
  std::vector<std::vector<double> >Efficency_true_vec(3, std::vector<double>(0));
  std::vector<std::vector<double> >Efficiency_hot_vec(3, std::vector<double>(0));
  std::vector<std::vector<double> >Efficiency_muondelta_vec(3, std::vector<double>(0));

  double  Efficiency_wire;
  double  Efficiency_hot;
  double TRUEHITS=0;

  int numberoftruehits =0;

  //run over all planes
  for(geo::PlaneID plane_it: geom->IteratePlaneIDs()){
  

  raw::ChannelID_t channel;

  double number_of_wires_with_morethan_one_hit=0;
  // double number_of_wires_with_morethan_one_truehit=0;
  double number_of_wires_with_one_hit; 
  // double number_of_wires_with_one_truehit;
  //  double colltruehits=0;
  double collhits=0;
  //  double true_muon_hits=0;
  double true_HITS=0;

  std::vector< std::vector< art::Ptr< recob::Hit > > > truehits_vec;
  std::vector< art::Ptr< recob::Hit > > TrueHitsMuon;
  std::vector<int> Total_true_trackIDS;

  std::map<int,std::vector< art::Ptr< recob::Hit > > > Total_true_hits_map;
  std::map<int,int> channelmap;
  std::map<int,int> truechannelmap;


  for(std::map<art::Ptr<recob::Hit>,int >::iterator hitIt = likelyTrackID.begin(); hitIt != likelyTrackID.end(); ++hitIt){                                                      
 
    geo::WireID wireid = (hitIt->first)->WireID();                                                                                                                              
    geo::PlaneID PlaneID = wireid.planeID();
  
    //if(plane_it!=2 && wireid.TPC == 0){ int NewPlaneID = PlaneID-1; PlaneID = TMath::Abs(NewPlaneID);}
    if(PlaneID != plane_it){continue;}
    channel = wireid.Wire;
    truechannelmap[channel] = ++truechannelmap[channel];  
    ++true_HITS;
  }   



  
  // //Find all the hits that come from the truth particle in the plane. Place them in a channel map to look for wires with more than 2 hits associated to the muon (this should not happen).  
  // for(std::vector<art::Ptr<recob::Hit> >::iterator hitIt = TrueHitsMuon.begin(); hitIt != TrueHitsMuon.end(); ++hitIt){
    
  //   art::Ptr<recob::Hit> hit = *hitIt;
  //   geo::WireID wireid = hit->WireID();

  //   //int Plane = wireid.Plane;
  //   //    if(wireid.TPC == 0){ 
  //   //  if(Plane =! 2){ 
  //   if(wireid.Plane != plane_it){continue;} 
 
  //   colltruehits++;
  //   //  channel = wireid.Wire;
  //   // truechannelmap[channel] = ++truechannelmap[channel];
      
  //   }

  // numberoftruehits = numberoftruehits+colltruehits;

  //Find the number of hits in the plane in total and place in a channel map to look for wires with more than 2 hits.
  for(std::vector<art::Ptr<recob::Hit> >::iterator hitIt = hits.begin(); hitIt != hits.end(); ++hitIt){
    art::Ptr<recob::Hit> hit = *hitIt;
    geo::WireID wireid = hit->WireID();
    geo::PlaneID  PlaneID = wireid.planeID();
    // if(plane_it!=2 && wireid.TPC == 0){std::cout << "The plane is switched" << std::endl;;int NewPlaneID = PlaneID-1; PlaneID = TMath::Abs(NewPlaneID);}
    if(PlaneID != plane_it){continue;}
    collhits++;
     
    channel = wireid.Wire;
    channelmap[channel] = ++channelmap[channel];
  
   }



  // //Find the number of channels with more than one hit from the true muon. 
  // for(std::map<int,int>::iterator channelIt = truechannelmap.begin(); channelIt != truechannelmap.end(); ++channelIt){
  //    if(channelIt->second != 1){
  //      ++number_of_wires_with_morethan_one_truehit;
  //      hits_to_be_removed = hits_to_be_removed + channelIt->second;
  //      //      std::cout << "The Channel: " << channelIt->first << " has more than one true hit. It has true hits: " << channelIt->second << std::endl;
  //   }
  // }

  std::map<int,int> mt1_channelmap;  
//Find the number of channels with more than one hit from all the hits.
for(std::map<int,int>::iterator channelIt = channelmap.begin(); channelIt != channelmap.end(); ++channelIt){
  if(channelIt->second != 1){
    
    mt1_channelmap[channelIt->first] = channelIt->second;
    if(channelIt->first != 0){mt1_channelmap[(channelIt->first) - 1] = channelIt->second;}
    int channel_map_size_m1 = channelmap.size() - 1;
      if(channelIt->first != channel_map_size_m1){mt1_channelmap[(channelIt->first) + 1] = channelIt->second;}
  
    ++number_of_wires_with_morethan_one_hit;
  }  //    std::cout << "The Channel: " << channelIt->first << " has more than one hit. It has hits: " << channelIt->second << std::endl;
 

 }
    



  double  number_of_wire_that_are_hit = channelmap.size(); 
  double  wires_that_are_crossed = WiresCrossed_map[plane_it].size();
  
  // number_of_wires_with_one_truehit = number_of_wire_that_are_truehit - number_of_wires_with_morethan_one_truehit;
  number_of_wires_with_one_hit = number_of_wire_that_are_hit - number_of_wires_with_morethan_one_hit; 

  double number_of_wires_not_excluded =  number_of_wire_that_are_hit-mt1_channelmap.size();
  std::cout << "number_of_wires_not_excluded = " << number_of_wires_not_excluded << std::endl;
 //Efficiencies 

     //The number of wires which see one hit/ number of wires cross  - number of wires which see more than one hit.
      Efficiency_wire = (number_of_wires_with_one_hit)/ (wires_that_are_crossed-number_of_wires_with_morethan_one_hit);

     //The number of hits per plane/number of true hits per plane
       Efficiency_hot        = collhits/true_HITS;
    
     // The number of muon hits + noise / number of true muon hits per plane
      
     
     // The number of muon hits / number of true muon hits per plane (should be the same above unless there are true hits from other particles) 
  

     

  
  
   E1[plane_it.Plane]= E1[plane_it.Plane]+Efficiency_wire;
   //E2[plane_it]=E2[plane_it]+Efficency_true;
   E3[plane_it.Plane]= E3[plane_it.Plane] +Efficiency_hot;
  

 channels_see_hit = channels_see_hit + truechannelmap.size();
  
 if(wires_that_are_crossed != 0){
   // (Efficiency_angle_map[angle])[plane_it].push_back(Efficiency_hot);
 (Efficiency_angle_map[plane_it])[angle].push_back(Efficiency_wire);
 Efficency_para_vec[plane_it.Plane].push_back(Efficiency_wire);
 Efficiency_hot_vec[plane_it.Plane].push_back(Efficiency_hot);
 }


 std::cout << " #############################################" << std::endl;
 std::cout << "WiresCrossed = " << WiresCrossed_map[plane_it].size() << std::endl;
 std::cout << "Efficiency Wire = " <<  Efficiency_wire << std::endl; 
 std::cout << "Number of wires with one hit: " << number_of_wires_with_one_hit << " number_of_wires_with_morethan_one_hit: " << number_of_wires_with_morethan_one_hit << std::endl;
 std::cout << " #############################################" << std::endl;

  }//plane loop
 
  std::cout << " MuonMCHits.size(): " << MuonMCHits.size() <<std::endl;
  std::cout << "MC_vec.size() " << MC_vec.size() << std::endl;
  std::cout << "The channels that see true hits: " << channels_see_hit << " Number of wires truly crossed: " << Wirenum << std::endl;
  std::cout << "The total number of noise hits is: " << noise_hits.size() << std::endl;
  std::cout << "angle: " <<  angle <<std::endl;

    (Efficency_para_map[angle]) = Efficency_para_vec; 
    (Efficiency_hot_map[angle])   = Efficiency_hot_vec;
  std::cout << "numberoftruehits:" << numberoftruehits << " TRUEHITS: " <<  TRUEHITS << " likelyTrackID.size: " <<  likelyTrackID.size() << " hitssize: " << hits.size() << " mchit num: " << mchit_num <<std::endl;

 
return;
}
 

void ana::HitEff::endJob() {
  // std::cout << event << std::endl;

  // for(int i=0; i<3; ++i){
  //   std::cout << E1[i]/event << " " << i << std::endl;
  //   std::cout << E2[i]/event << " " << i << std::endl;
  //   std::cout << E3[i]/event << " " << i << std::endl;
  //   std::cout << E4[i]/event << " " << i << std::endl;
  
  // }

  // TFile *file = new TFile("EfficiencyGraphs","RECREATE");

  // for( std::map<geo::PlaneID, std::map<double,std::vector<double> > >::iterator i = Efficiency_angle_map.begin(); i != Efficiency_angle_map.end(); ++i){
    
  //   TGraphErrors* Efficiency_para_graph = new TGraphErrors(0);

  //   for(std::map<double,std::vector<double> >::iterator j = (i->second).begin();  j != (i->second).end(); ++j){
    
  //     double angle=j->first;
  //     std::cout << "angle: "<< angle << std::endl;
  //     double mean   = accumulate( (j->second).begin(), (j->second).end(), 0.0)/ (j->second).size();
  //     double sq_sum = std::inner_product((j->second).begin(), (j->second).end(), (j->second).begin(), 0.0);
  //     double stdev  = std::sqrt(sq_sum / (j->second).size() - mean * mean);
  //     double stderrmean = stdev/std::sqrt((j->second).size());
  //     std::cout << mean << std::endl;
  //     std::cout << "stderrmean: " << stderrmean << std::endl;
  //     Efficiency_para_graph->SetPoint(Efficiency_para_graph->GetN(),angle, mean);
  //     Efficiency_para_graph->SetPointError(Efficiency_para_graph->GetN(),0, stderrmean);
  //   }
  
  //   std::string map_string;
  //   std::stringstream sstm1;
  //   sstm1 << "Efficiency_Para PlaneID:" << (i->first);
  //   map_string = sstm1.str();
  //   const char* name = map_string.c_str();

  //   Efficiency_para_graph->SetMarkerColor(4);
  //   Efficiency_para_graph->SetMarkerStyle(21);
  //   Efficiency_para_graph->Draw("AP");
  //   Efficiency_para_graph->SetName(name);
  //   Efficiency_para_graph->GetXaxis()->SetTitle("Angle");
  //   Efficiency_para_graph->GetYaxis()->SetTitle("Efficiency Para");
  //   Efficiency_para_graph->SetTitle(name);
  //   Efficiency_para_graph->Write();

  // }

  // file->Close();

 //    TCanvas *c1 = new TCanvas("c1","Profile histogram example",200,10,700,500);
//     TProfile * hprof  = new TProfile("hprof","Profile of pz versus px",100,-4,4,0,20);

//     for std::map<double,std::vector<std::vector<double> > >::iterator it = Efficency_para_map.begin(); it != Efficency_para_map.end(); ++it){

//     std::vector<double> Efficiency_para_vec =  (it->second).at(0);
//     for(int i=0; i<Efficiency_para_vec; ++i){
//       hprof->Fill(it->second,Efficiency_para_vec[0],1)
//     }
//     hprof->Draw();
// }
  return;  
}

DEFINE_ART_MODULE(ana::HitEff)


