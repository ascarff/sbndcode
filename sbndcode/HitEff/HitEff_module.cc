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
#include "TMultiGraph.h"
#include "TLegend.h"

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
  MCHits();
  MCHits(int,int,double,double, std::vector<std::pair<int,double> >); 
  MCHits(int, std::vector<std::pair<int,double> >);
  MCHits(int, std::vector<std::pair<int,double> >, int);
  MCHits& operator=(const MCHits& a){
    channel = a.channel;
    peak_time = a.peak_time;
    TrackID = a.TrackID;
    total_charge = a.total_charge;
    peak_charge = a.peak_charge;
    time_charge_vec = a.time_charge_vec;
    return *this;
  } 
  //friend bool operator< (const MCHits& rhs);
  friend bool operator<(const MCHits a, const  MCHits b);
  friend bool operator== (const MCHits &a, const MCHits &b);

  int PeakTime(){return peak_time;}
  int Channel(){return channel;}
  int Trackid(){return TrackID;}
  double PeakCharge(){return peak_charge;}
  double TotalCharge(){return total_charge;}
  std::vector<std::pair<int,double> > TimeChargeVec(){return time_charge_vec;}
};

//bool operator< (const MCHits& rhs)
//{
//  return peak_time < rhs.peak_time || (peak_time == rhs.peak_time && peak_charge < rhs.peak_charge);
//}

bool operator<(const MCHits a, const  MCHits b)
{
  return a.peak_time  > b.peak_time  || (a.peak_time == b.peak_time && a.peak_charge < b.peak_charge);
}


bool operator== (const MCHits &a, const MCHits &b)
{
  return (a.peak_time == b.peak_time && b.peak_charge == b.peak_charge && a.TrackID == b.TrackID && a.channel == b.channel && a.total_charge == b.total_charge);
}


MCHits::MCHits(){
  channel = 1e9;
  peak_charge = 1e9;
  peak_time = 1e9;
  total_charge = 1e9;
  TrackID = 1e9;
}

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

//Merge Hits takes two hits and merges them into one hit. It does not merge the time-charged vector by time it just takes both vectors.
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

//Sorts a map from highest to lowest of the second paramater in the map 
bool sortbysec(const std::pair<int,double> &a,
	       const std::pair<int,double> &b)
{
  return (a.second > b.second);
}

bool sortbyPeakTime(MCHits &a, MCHits &b)
{return a.PeakTime() < b.PeakTime();} 


//Cut Hit looks a specifici hit and its maximum points. It fits permutations of all the possible  gaussians fits to the hit and takes returns the number of gaussians that gives the miniumum chi-2 
std::vector<MCHits> CutHit(MCHits hit,std::vector<std::pair<int,double> > MaxPoints, double fGausWidth, double fSigmaGausWidth,double fSigmaTimeWidth, double charge_cut){
  std::vector<MCHits> hit_vec;

  //If there is only one (or no) max points just return the hit again - no cutting occurs.
  if(MaxPoints.size()<2){hit_vec.push_back(hit); return hit_vec;} 

  //Sort the Max points so the highest point has the gaussian fitted first (now not necessary).
  sort(MaxPoints.begin(), MaxPoints.end(), sortbysec); 

  //Accesss the time Time Charge vector of the hit.
  std::vector<std::pair<int,double> > TimeChargeVector =  hit.TimeChargeVec();
  
  //Create The histrogram for entire drift window - Change for ICARUS and fill wieghting by the charge.
  TH1D *ChargeHist = new TH1D("ChargeHist","Charge on the Channel Hist",3000,0,3000);
  for(std::vector<std::pair<int,double> >::iterator iter=TimeChargeVector.begin(); iter!=TimeChargeVector.end(); ++iter){
    ChargeHist->Fill(iter->first,iter->second);
  }

  //if the fcl parameter that sets the lowest width of the fitted gaussian is higher than the RMS of the hist obviously we will have a bad fit on the graph for one gaus. Change accordingly.
  double hist_RMS = ChargeHist->GetRMS();
  if(fGausWidth > hist_RMS){fGausWidth = hist_RMS;}


  //Initialse start on n=3 as at least the one gaussian fit will occur. 
  int n = 3;
  std::stringstream sstm1; 
  std::string fit_string;
  std::vector<double> Parameter_vec;
  std::vector<std::pair<double,std::vector<double> > > chi2s;
  std::vector<double> par_min;
  std::vector<double> par_max;


  int lump_num = 0;

  //Loop over the maximum points fitting  
  for(std::vector<std::pair<int,double> >::iterator iter=MaxPoints.begin(); iter!=MaxPoints.end(); ++iter){


    lump_num = lump_num +1;
    
    //Add to the strign stream another gaussian 
    if(iter==MaxPoints.begin()){sstm1 << "([" << n-3 <<"]*TMath::Exp(-0.5*std::pow(((x-["<<n-2<<"])/["<<n-1<<"]),2))) ";}                   
    else{sstm1 << "+ ([" << n-3 <<"]*TMath::Exp(-0.5*std::pow(((x-["<<n-2<<"])/["<<n-1<<"]),2))) ";}    
    
    fit_string = sstm1.str();          
    const char* fit_name = fit_string.c_str();

    //Add the initial fit paramters to the vector based off the MaxPoints info 
    Parameter_vec.push_back(iter->second);//max                                                                                                                          
    Parameter_vec.push_back(iter->first);//time position                                                                                                                 
    Parameter_vec.push_back(fGausWidth);//sigma                                            

    //Give a range for the fits, the highest and lowest maxmium points set the range.  
    // par_min.push_back(MaxPoints[MaxPoints.size()-1].second);
    if(iter->second - charge_cut < charge_cut){par_min.push_back(iter->second);}
    else{par_min.push_back(iter->second - charge_cut);} 
    
    if(iter->first - fSigmaTimeWidth<0){par_min.push_back(0);}
    else{par_min.push_back(iter->first - fSigmaTimeWidth);}  
    par_min.push_back(fGausWidth);
    //par_min.push_back(fSigmaTimeWidth);                                                                                                                            
    //par_min.push_back(fSigmaGausWidth);

    //par_max.push_back(MaxPoints[0].second);
    par_max.push_back(iter->second + charge_cut);
    par_max.push_back(iter->first + fSigmaTimeWidth); 
    par_max.push_back(fGausWidth + fSigmaGausWidth);
    //par_max.push_back(fSigmaTimeWidth);                                                                                                                            
    //par_max.push_back(fSigmaGausWidth);    


    if(iter != MaxPoints.begin() && iter != MaxPoints.end()){
      //Try permutations of the fitting for the gaussian fits which involve more than one gaussian but not a fit too all the peaks. 
      for(std::vector<std::pair<int,double> >::iterator jter=iter; jter!=MaxPoints.end(); ++jter){
	
	//Set the fit range to start and end times of the hit and change the fit paramter according to which peak we are fitting over. 
	TF1 *gausFit = new TF1( "gausfit", fit_name,TimeChargeVector.begin()->first,std::prev(TimeChargeVector.end())->first);

	std::vector<double>  Parameter_vec_temp =  Parameter_vec;
	std::vector<double>  par_min_temp = par_min;
	std::vector<double>  par_max_temp = par_max;

	Parameter_vec_temp[n-3] = jter->second;//max                                                                                    
	Parameter_vec_temp[n-2] = jter->first;//time position

	if(jter->second - charge_cut< charge_cut){par_min_temp[n-3] =jter->second;}
	else{par_min_temp[n-3] = jter->second - charge_cut;}
	par_max_temp[n-3] = jter->second + charge_cut;
	if(jter->first - fSigmaTimeWidth<0){par_min_temp[n-2] =0;}
	else{par_min_temp[n-2] = jter->first - fSigmaTimeWidth;}
	par_max_temp[n-2] = jter->first + fSigmaTimeWidth;                                                                           
	//Parameter_vec[n-1].push_back(2);//sigma                                                                                            
	
	for(Int_t i=0; i<n; ++i){gausFit->SetParameter(i,Parameter_vec_temp[i]); gausFit->SetParLimits(i, par_min_temp[i], par_max_temp[i]);}        

	//Fit the parameters within range specified above. 
	ChargeHist->Fit(gausFit,"QR");                                                                                                  
                    
	//Get the chi2s and store with the parameters as an identifier.
	std::vector<double> Parameters;                                
	for(Int_t i=0; i<n; ++i){Parameters.push_back(gausFit->GetParameter(i));}
	
	std::stringstream sstm2;
	sstm2 << "Channel: " << hit.Channel() ;
	std::string map_string = sstm2.str();
	const char* name = map_string.c_str();
	ChargeHist->SetName(name);
	ChargeHist->Write();     
	
	chi2s.push_back(std::make_pair(gausFit->GetChisquare(),Parameters));
   
	delete gausFit;   
      }//Permutation loop
    }//If 
    else{
      //If it is the first and last fit do the same thing as above just don't repeat the permutations.  
      TF1 *gausFit = new TF1( "gausfit", fit_name,TimeChargeVector.begin()->first,std::prev(TimeChargeVector.end())->first);
	      
      for(Int_t i=0; i<n; ++i){gausFit->SetParameter(i,Parameter_vec[i]); gausFit->SetParLimits(i, par_min[i], par_max[i]);}
      ChargeHist->Fit(gausFit,"QR");
	      
      std::vector<double> Parameters;
      for(Int_t i=0; i<n; ++i){
	Parameters.push_back(gausFit->GetParameter(i));}
      
      std::stringstream sstm2;
      sstm2 << "Channel: " << hit.Channel() ;
      std::string map_string = sstm2.str();
      const char* name = map_string.c_str();
      ChargeHist->SetName(name);
      ChargeHist->Write();


      chi2s.push_back(std::make_pair(gausFit->GetChisquare(),Parameters));
      
      delete gausFit;
    }//Else
    n=n+3; //Add three onto the n for the next 3 paramters in the fit.
  }//Max Points Loop 
  
  delete ChargeHist;

  double min_chi2 = 1e9;
  std::pair<double,std::vector<double > > min_chi2_info;

  //loop over chi2s and find the minimum and its paramter info.
  for(std::vector< std::pair<double,std::vector<double > > >::iterator iter=chi2s.begin(); iter!=chi2s.end(); ++iter){
    if(iter->first < min_chi2){min_chi2 = iter->first; min_chi2_info = *iter; }
  }

  double gaus;

  //Loop over the number of hits to be split e.g. (min_chi2_info.second).size()=6 for 2 hits.
  for(unsigned int n=3; n<(min_chi2_info.second).size() +1; n = n+ 3){
    std::vector<std::pair<int,double> > TimeChargeVector_new;

    //Loop over the TimeCharge vector and multipy by the charge by gaus factor that resulted in the minimum chi2
    for(std::vector<std::pair<int,double> >::iterator iter=TimeChargeVector.begin(); iter!=TimeChargeVector.end(); ++iter){
      gaus =  min_chi2_info.second.at(n-3)*TMath::Exp(-0.5*std::pow(((iter->first - min_chi2_info.second.at(n-2))/ min_chi2_info.second.at(n-1)),2)); 
      TimeChargeVector_new.push_back(std::make_pair(iter->first, gaus));
    }
    
    //Create a new hit (Start and End time of the hit times are not really true but you can't seperate this info this way). Add new hit. 
    MCHits  Split_hit(hit.Channel(),TimeChargeVector_new,hit.Trackid());
    hit_vec.push_back(Split_hit);

    // Create new histogram showing the seperated hit - not needed for analysis. More a debugging tool.  
    std::vector<std::pair<int,double> > TimeChargeVector_temp = Split_hit.TimeChargeVec();
    TH1D *ChargeHist = new TH1D("ChargeHist","Charge on the Channel Hist",3000,0,3000);
    for(std::vector<std::pair<int,double> >::iterator jjter=TimeChargeVector_temp.begin(); jjter!=TimeChargeVector_temp.end(); ++jjter){
      ChargeHist->Fill(jjter->first,jjter->second);
    }

    std::stringstream sstm2;
    sstm2 << "Channel: " << hit.Channel() ;
    std::string map_string = sstm2.str();
    const char* name = map_string.c_str();
    ChargeHist->SetName(name);
    ChargeHist->Write();
    delete ChargeHist;


  }//Split Hits Loops 

  return hit_vec;
}
 


class ana::HitEff : public art::EDAnalyzer {
public:

  HitEff(const fhicl::ParameterSet& pset);

  void analyze(const art::Event& evt);
  void endJob();
private:

  std::string fHitsModuleLabel;
  std::string fMCHitsModuleLabel;
  bool   fThetaXZinstead;
  std::vector<double> fChargeCut;
  std::vector<double> fdtThreshold;

  double  fGausWidth;
  double  fSigmaGausWidth;
  double  fSigmaTimeWidth;
  double  fADC;


    art::ServiceHandle<cheat::BackTracker> backtracker;
  
  //  art::detail::ServiceHelper<cheat::BackTracker> backtracker;
  std::map<int,const simb::MCParticle*> trueParticles;

  //First plane, then thresholds, then angle, finally vector of Efficiencies. 
  std::map<geo::PlaneID, std::map<std::pair<double,double>, std::map<double,std::vector<double> > > > Efficiency_wire_map;  
  std::map<geo::PlaneID, std::map<std::pair<double,double>, std::map<double,std::vector<double> > > > Efficiency_nodelta_map;
  std::map<geo::PlaneID, std::map<std::pair<double,double>, std::map<double,std::vector<double> > > > Efficiency_seperatedelta_map;

  //First second threshold_pair finally vector of Efficiencies. 
  std::map<std::pair<double,double>,std::map<double, std::vector<double> > > Efficiency_total_wire_map;
  std::map<std::pair<double,double>,std::map<double, std::vector<double> > > Efficiency_total_nodelta_map;
  std::map<std::pair<double,double>,std::map<double, std::vector<double> > > Efficiency_total_seperatedelta_map;

  //conversion 1ADC = 174.11 electrons
  double ADCconversion = 174.11;
};

ana::HitEff::HitEff(const fhicl::ParameterSet& pset) : EDAnalyzer(pset) {
  fHitsModuleLabel = pset.get<std::string>("HitsModuleLabel");
  fMCHitsModuleLabel = pset.get<std::string>("MCHitsModuleLabel");
  fChargeCut = pset.get<std::vector<double> >("ChargeCut"); //2ADC is min
  fGausWidth = pset.get<double>("GausWidth");
  fSigmaGausWidth = pset.get<double>("SigmaGausWidth");
  fSigmaTimeWidth = pset.get<double>("SigmaTimeWidth");
  fThetaXZinstead= pset.get<bool>("ThetaXZinstead");
  fADC=pset.get<double>("ADC");
  fdtThreshold = pset.get<std::vector<double> >("dtThreshold");
  }


void ana::HitEff::analyze(const art::Event& evt) {

  for(std::vector<double>::iterator fChargeCut_iter = fChargeCut.begin(); fChargeCut_iter != fChargeCut.end(); ++fChargeCut_iter){  
  for(std::vector<double>::iterator fdtThreshold_iter = fdtThreshold.begin(); fdtThreshold_iter != fdtThreshold.end(); ++fdtThreshold_iter){ 
    
    int num_merged_n_not=0;
    int num_merged_hits=0;
   //Make a pair for the legend labels; 
    std::pair<double,double> threshold_pair = std::make_pair(*fChargeCut_iter, *fdtThreshold_iter);

  TFile *f = new TFile("TimeVsWire","RECREATE");
 
  //IMPORTANT: Change the Charge cut from ADCs to electrons 
  double charge_cut = *fChargeCut_iter*ADCconversion;  

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

  //##################################
  //### MC-Reco Charge Comparision ###           
  //##################################

  //Initialse the Reco/MC Charge Comparision Graphs.
  TH1D *BckTrackTotChargeHist = new TH1D("TotChargeHist","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
  TH1D *BckTrackTotChargeHist0 = new TH1D("TotChargeHist0","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
  TH1D *BckTrackTotChargeHist1 = new TH1D("TotChargeHist1","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);
  TH1D *BckTrackTotChargeHist2 = new TH1D("TotChargeHist2","Ratio difference between reco and MC Total Charge",500,-1.5,1.5);

    
  // Assign a track id to a hit. Now there is contributions from delta rays (id=-1) and the muon (id=1)/                                                                          
  std::map<art::Ptr<recob::Hit>,int >    likelyTrackID;

  
  //Initialse a map of the reco hits on each channel. 
  std::map<unsigned int,std::vector<art::Ptr<recob::Hit> > > channel_RecoHit_map;

  // Loop over hits to put into a channel map .                                                                                    
  for(std::vector<art::Ptr<recob::Hit> >::iterator hitIt = hits.begin(); hitIt != hits.end(); ++hitIt){

  
    art::Ptr<recob::Hit> hit = *hitIt;
    auto const wire_id = hit->WireID();
    
    //Get the channel 
    unsigned int ch = geom->PlaneWireToChannel(wire_id.Plane,
				       wire_id.Wire,
				       wire_id.TPC,
				       wire_id.Cryostat);

    channel_RecoHit_map[ch].push_back(hit);
    
    //Split the Hit into its IDE for each track it associates with.  
    std::vector<sim::TrackIDE> trackIDs = backtracker->HitToTrackID(hit);

     double  mc_qsum=0;
     double particleEnergy = 0;
     
    for (unsigned int idIt = 0; idIt < trackIDs.size(); ++idIt) {
     
      //Add up the number of Electrons in the Hit. 
      mc_qsum  +=  trackIDs.at(idIt).numElectrons;

      //Assign the track ID to hit that contributes the most energy.                                                                                                             
      if (trackIDs.at(idIt).energy > particleEnergy) {
	particleEnergy = trackIDs.at(idIt).energy;
	likelyTrackID[hit] = trackIDs.at(idIt).trackID;
      }
    }

    //Get the Charge information from GausHitFinder
    double      reco_tot = hit->Integral();
    double      totalchargediff;

    //Plot the MC Charge and Reco charge in ADCs. The conversion factor comes from caloimetry - MIGHT NOT WORK FOR ICARUS 
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
  BckTrackTotChargeHist->Delete();

  BckTrackTotChargeHist0->Draw("same");
  BckTrackTotChargeHist0->SetName("BckTrackTotalCharge 0");
  BckTrackTotChargeHist0->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  BckTrackTotChargeHist0->SetTitle("BckTrackTotal Charge0 ");
  BckTrackTotChargeHist0->Write();
  BckTrackTotChargeHist0->Delete();

  BckTrackTotChargeHist1->Draw("same");
  BckTrackTotChargeHist1->SetName("BckTrackTotalCharge 1");
  BckTrackTotChargeHist1->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  BckTrackTotChargeHist1->SetTitle("BckTrackTotal Charge ");
  BckTrackTotChargeHist1->Write();
  BckTrackTotChargeHist1->Delete();

  BckTrackTotChargeHist2->Draw("same");
  BckTrackTotChargeHist2->SetName("BckTrackTotalCharge 2");
  BckTrackTotChargeHist2->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  BckTrackTotChargeHist2->SetTitle("BckTrackTotal Charge ");
  BckTrackTotChargeHist2->Write();
  BckTrackTotChargeHist2->Delete();


  //##########################
  //### Create Dom MC Hits ###
  //#########################


  //Define the graphs that show the WireVsTime - These are similar to the event display graphs - This is for debugging 
  gInterpreter->GenerateDictionary("std::map<geo::PlaneID,TGraph*>","TGraph.h;map");
  std::map<geo::PlaneID,TGraph*> Graph_map;
  std::map<geo::PlaneID,TGraph*> Graph_mapm1;
  std::map<geo::PlaneID,TGraph*> Graph_map1;


  for(geo::PlaneID plane_id: geom->IteratePlaneIDs()){
    Graph_map[plane_id] = new TGraph(0);
    Graph_mapm1[plane_id] = new TGraph(0);
    Graph_map1[plane_id] = new TGraph(0);
    }





 
  //Get the Handle for the MCHit Collections 
  const std::vector<sim::MCHitCollection> &mchits_v = *MCHitHandle;
    
  //Loop Paramters 
  int mchit_num=0;
  int channel_num;
  double peak_time=0;
  std::vector<double> channels;
  std::vector<double> times;

  // Where the MCHits will be stored
  std::map<unsigned int,std::vector<MCHits> > channel_MCHit_map; 
  std::vector<MCHits> MC_vec;
  
  //loop over the mchit collections (i.e loop over all the wires with MC hits) and find how many true hits we have.
  for(auto const& mchits : mchits_v) {
    
    geo::PlaneID plane = geom->ChannelToWire(mchits.Channel()).at(0).planeID();
    channel_num = mchits.Channel();


    std::vector<std::pair<int,double> > total_time_charge_vec;
    std::vector<std::pair<int,double> > time_charge_vec;
    std::map<int, double> charge_map;
    std::map<int,std::vector<std::pair<int,double> > > TrackIDtoMCHitMap;

    //loop of the MChits in the collection  
    for(auto const& mchit : mchits){


      total_time_charge_vec.push_back(std::make_pair(mchit.PeakTime(),mchit.Charge(true))); 
      charge_map[mchit.PeakTime()] =  charge_map[mchit.PeakTime()] +  mchit.Charge(true);
      
     
      //      std::cout << " Charge: " << mchit.Charge(true) << " Time: " << mchit.PeakTime() << " Track ID: " << mchit.PartTrackId()<< " Channel: " << channel_num << std::endl;

      // Create the Time-Charge Vector for each channel used to create the hit  
      TrackIDtoMCHitMap[mchit.PartTrackId()].push_back(std::make_pair(mchit.PeakTime(),mchit.Charge(true)));

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
	   Graph_map[plane]->SetPoint(Graph_map[plane]->GetN(),channel_num, peak_time);
      

    } //MC Hit Loop
    



    
    //This method takes mutliple takes delta ray and muon contribution seperately placing a cut on the dt of the muon and charge threshold.                                       
    //###################################################################################################

    //Only create a hit if there is charge there. 
    if(charge_map.size() == 0){continue;}                                                                                                                            

     channels.push_back(channel_num);
    // Loop over all the Tracks IDs on the wire 
    for(std::map<int,std::vector<std::pair<int,double> > >::iterator jter=TrackIDtoMCHitMap.begin(); jter!=TrackIDtoMCHitMap.end(); ++jter){

      int num_voxels_iter=0;
      std::vector<MCHits> MC_vec_temp;
      std::vector<std::pair<int,double> >::iterator prev_iter = (jter->second).begin();
      std::vector<std::pair<int,double> >::iterator now_iter  = (jter->second).begin();
      std::vector<std::pair<int,double> >::iterator next_iter;
      
      //Loop over the mchits (i.e.voxels) looking for spaces between hits 
      for(std::vector<std::pair<int,double> >::iterator iter=(jter->second).begin(); iter!=(jter->second).end(); ++iter){ 
      
      next_iter =std::next(iter);                                         

      //This allows for the only hits above threshold to be compared for the times so two peaks can be split at this stage (if there is a gap). 
      if(charge_map[iter->first]  > charge_cut){
	now_iter = iter;
	//Only Save the hit if there is a accummultive sum of charge at that point
	++num_voxels_iter;
	//Set the previous iter if there is yet to be one set. This passes the tick test and looks at the end test. 
	//	if(prev_iter==(jter->second).end()){prev_iter = now_iter;}
           }
      
      // else if(iter == std::prev((jter->second).end())){ if(charge_map[iter->first]  > charge_cut){time_charge_vec.push_back(*iter);} }
      //else{time_charge_vec.push_back(std::make_pair(iter->first,0));continue;}

      //time_charge_vec.push_back(*iter);

      //Set the previous iter if there is yet to be one set. this passes the tick test and looks at the end test.
      //if(prev_iter==(jter->second).end()){prev_iter = now_iter;}

      //      std::cout << " Charge: " << iter->second << " Time: " << iter->first << " Track ID: " << jter->first<< " Channel: " << channel_num << std::endl;
      //      std::cout << "next_iter->first: " << next_iter->first << "now_iter->first: " << now_iter->first << std::endl;

      
      //If there is a gap of 1 tick make a new hit.                                                                                                                              
      if(iter->first - prev_iter->first > 1){
        //std::cout << "#############TIME############ " << std::endl;                                                                                                              
	if(num_voxels_iter == 0){continue;}
	MCHits MC_hit(channel_num,time_charge_vec,jter->first);
	MC_vec_temp.push_back(MC_hit); time_charge_vec.clear();
      }
      //If at the end of the charge vector this a new hit.  
      else if(iter == std::prev((jter->second).end())){                                                                                        
   	//The last charge will be missed need to check its not above the charge threshold.
	if(charge_map[iter->first]  > charge_cut){time_charge_vec.push_back(*iter);}
	if(num_voxels_iter == 0){continue;}
	//	std::cout << "#############END############ " << std::endl;
	MCHits  MC_hit(channel_num,time_charge_vec,jter->first); 
	MC_vec_temp.push_back(MC_hit); time_charge_vec.clear();
	continue;
      }
      time_charge_vec.push_back(*iter);
      prev_iter= iter;                                                                                                                                                                    }//iter - Loop over the voxels.
  
      //Look at the Hit (MC_vec_temp.size() should be 1) and find the maximum points then see if it should be hit should be split into multiple hits.    
     for(unsigned int i=0; i< MC_vec_temp.size(); ++i){


       std::vector<std::pair<int,double> > timeChargeVec = MC_vec_temp[i].TimeChargeVec();
       std::vector<std::pair<int,double> > MaxPoints;
       std::vector<std::pair<int,double> >::iterator prev_kter;
       std::vector<std::pair<int,double> >::iterator next_kter;

       // Loop over the time-charge vec of hit looking for maximum points in the data.
       for(std::vector<std::pair<int,double> >::iterator kter = timeChargeVec.begin(); kter!=timeChargeVec.end(); ++kter){
       

    	 if(kter == timeChargeVec.begin()){continue;}
    	 else{prev_kter = std::prev(kter);}

    	 if(kter == std::prev(timeChargeVec.end())){continue;}    
    	 else{next_kter = std::next(kter);}

	 if(prev_kter->second <= kter->second  && next_kter->second <= kter->second && charge_map[kter->first] > charge_cut){
    	     MaxPoints.push_back(std::make_pair(kter->first,kter->second));
	   }
       }

       std::vector<MCHits> MC_hits = CutHit(MC_vec_temp[i],MaxPoints, fGausWidth,fSigmaGausWidth, fSigmaTimeWidth, charge_cut);
       //Add the cut hits to the channel-MCHit map. 
       channel_MCHit_map[channel_num].insert(channel_MCHit_map[channel_num].end(),  MC_hits.begin(),  MC_hits.end());
     }//MC_Vec_temp loop
    } // Loop of TrackIDtoMCHitMap 
     

    // Use this if you don't want to use the gaussian method
     //        for(std::map<int,std::vector<std::pair<int,double> > >::iterator iter=TrackIDtoMCHitMap.begin(); iter!=TrackIDtoMCHitMap.end(); ++iter){
     //MCHits MC_hit(channel_num,iter->second,iter->first); channel_MCHit_map[channel_num].push_back(MC_hit); //MC_vec.push_back(MC_hit) for debugging purposes;
     // }

    //if(channel_MCHit_map[channel_num].size()<2){continue;} // for debugging purposes 
    num_merged_n_not = num_merged_n_not + channel_MCHit_map[channel_num].size();
    // Only Merge if there is more than one hit.                                                                                                                                 
    if(channel_MCHit_map[channel_num].size()>1){

   ++num_merged_hits;
   //Now see if any of hits should be merged becuase they are ontop of each other.  
    std::vector<std::vector<MCHits>::iterator> erase_hits_vec;
    std::vector<MCHits> merged_hits;
    std::vector<MCHits*> merged_hits_ptrs;
    std::map<MCHits,MCHits> merged_hits_map;
          
    sort(channel_MCHit_map[channel_num].begin(), channel_MCHit_map[channel_num].end(),sortbyPeakTime);

    //Loop over the hits on the channel
    for(std::vector<MCHits>::iterator iter=channel_MCHit_map[channel_num].begin(); iter !=channel_MCHit_map[channel_num].end(); ++iter){
      
      // If the peak time is of the previous hit is within the threshold merge together 
      if(TMath::Abs(iter->PeakTime() - std::prev(iter)->PeakTime()) < *fdtThreshold_iter && iter!=channel_MCHit_map[channel_num].begin()){

	// If the previous hit has not already been merged then merge the hits and take not that they have been merged into the particular hit.
	if (std::find(erase_hits_vec.begin(), erase_hits_vec.end(), std::prev(iter)) == erase_hits_vec.end()){
	  //std::cout << "(iter->PeakTime(): " << iter->PeakTime() << "   std::prev(iter)->PeakTime(): " << std::prev(iter)->PeakTime() <<std::endl;  
	  MCHits MergeHit =  MergeHits(*iter,*std::prev(iter));
	  merged_hits_map.insert( std::pair<MCHits,MCHits>(*iter,MergeHit));
	  merged_hits_map.insert( std::pair<MCHits,MCHits>(*std::prev(iter),MergeHit ));
	}
	// If the previous has been merged then merge the hit with hit which was created from the previous merging. Change the merge hit that is associated to the previous hit to the new one.
	else{
	  if(merged_hits_map.find(*std::prev(iter)) == merged_hits_map.end() )
	    throw cet::exception(__PRETTY_FUNCTION__)
	      << "Trying to Merge a MCHit with one that doesn't exist.";
	 
	  MCHits MergeHit =  MergeHits(*iter, merged_hits_map[*std::prev(iter)]);
	  merged_hits_map.insert( std::pair<MCHits,MCHits>(*iter,MergeHit ));
	  for(std::map<MCHits,MCHits>::iterator jter=merged_hits_map.begin(); jter!=merged_hits_map.end(); ++jter){
	    if(&jter->second == &merged_hits_map[*std::prev(iter)]){merged_hits_map[jter->first] = MergeHit;}
	  }
	}
	    
	//Add the iterator to the ones that will be erased because they have been merged. 
	if (std::find(erase_hits_vec.begin(), erase_hits_vec.end(), iter) == erase_hits_vec.end()) {erase_hits_vec.push_back(iter);}
	if (std::find(erase_hits_vec.begin(), erase_hits_vec.end(), std::prev(iter)) == erase_hits_vec.end()) {erase_hits_vec.push_back(std::prev(iter));}
      }
    }
          
    // Remove the hits that have been merged.
    for(std::vector<std::vector<MCHits>::iterator>::iterator iter=erase_hits_vec.begin(); iter!=erase_hits_vec.end();++iter){
     channel_MCHit_map[channel_num].erase(*iter);
    }

    //Loop over the hits and get out the merged hits.                                                                                                                              
    for(std::map<MCHits,MCHits>::iterator jter=merged_hits_map.begin(); jter!=merged_hits_map.end();++jter){
      if(std::find(merged_hits.begin(), merged_hits.end() , jter->second) == merged_hits.end()){merged_hits.push_back(jter->second);}   
    }

    //Add the merged hits to MCHits vector/map                                                                                                                                  
    channel_MCHit_map[channel_num].insert(channel_MCHit_map[channel_num].end(),  merged_hits.begin(),  merged_hits.end());

    }//if(channel_MCHit_map[channel_num].size()>1) 
   
} //MC HitCollection Loop

  std::map<unsigned int,std::vector<MCHits> >::iterator kter=channel_MCHit_map.begin();

  while(kter != channel_MCHit_map.end()) {
    std::vector<MCHits>::iterator jter=kter->second.begin();
    while(jter != kter->second.end()){
      if(jter != std::prev(kter->second.end())){
        //If two insignificant hits are created and then merged with a greater hit they look like the same hit. Remove these (there shouldn't be any)                                                                       
        if(jter->PeakCharge() == std::next(jter)->PeakCharge() && jter->PeakTime() ==  std::next(jter)->PeakTime()){
          jter = kter->second.erase(jter); continue;
        }
      }
      if(jter->PeakCharge()<charge_cut){jter = kter->second.erase(jter); continue;}

      MC_vec.push_back(*jter);
      ++jter;
    }  
      //Remove any channels which no longer have MCHits due to the charge cut.                                                                                                      
      if(kter->second.size() == 0){kter = channel_MCHit_map.erase(kter); continue;}
      ++kter;
   }

  std::vector<MCHits> MuonMCHits;
  std::vector<MCHits>::iterator iter=MC_vec.begin();
  std::vector<MCHits> TrkID1MCHits;
  std::map<int,int> DeltaMCChannels;

  //Go through the MCHits and take note of muon hits, delta ray hits and merged muon hits 
  while(iter != MC_vec.end()) {
    if(iter->PeakCharge() < charge_cut){iter = MC_vec.erase(iter); continue;}
    //   std::cout << "iter->Trackid(): " << iter->Trackid() << " iter->PeakTime():" << iter->PeakTime() << " iter->Charge(): " << iter->PeakCharge() << " iter->Channel(): " << iter->Channel() <<std::endl; 
    if(iter->Trackid() == 1){TrkID1MCHits.push_back(*iter);}
    if(iter->Trackid() == -1){DeltaMCChannels[iter->Channel()]=1;} //The one is irrelevent
    if(iter->Trackid() != -1){MuonMCHits.push_back(*iter);}
    ++iter;
  }

  //Take note of channels either side channels that see delta ray hits. 
  std::map<int,int> DeltaMCChannels_Eitherside;
  for(std::map<int,int>::iterator iter=DeltaMCChannels.begin(); iter!=DeltaMCChannels.end(); ++iter){
    //    DeltaMCChannels_Eitherside[iter->first + 1] = 1;
    //   DeltaMCChannels_Eitherside[iter->first - 1] = 1;
    DeltaMCChannels_Eitherside[iter->first] = 1;
  }

  
  //Create the the names of the of the TimeVs Wire (event display stype) graphs and write to file. 
  std::string map_string;
  std::string map_stringm1;
  std::string map_string1;

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
    Graph_map[plane_id]->Delete();

    Graph_mapm1[plane_id]->Draw("AP");
    Graph_mapm1[plane_id]->SetName(namem1);
    Graph_mapm1[plane_id]->GetXaxis()->SetTitle("Channel Number");
    Graph_mapm1[plane_id]->GetYaxis()->SetTitle("Time (Ticks?)");
    Graph_mapm1[plane_id]->SetTitle(namem1);
    Graph_mapm1[plane_id]->Write();
    Graph_mapm1[plane_id]->Delete();

    Graph_map1[plane_id]->Draw("AP");
    Graph_map1[plane_id]->SetName(name1);
    Graph_map1[plane_id]->GetXaxis()->SetTitle("Channel Number");
    Graph_map1[plane_id]->GetYaxis()->SetTitle("Time (Ticks?)");
    Graph_map1[plane_id]->SetTitle(name1);
    Graph_map1[plane_id]->Write();
    Graph_map1[plane_id]->Delete();

  }

//################################################################################################  
//### Compare MCHits Charge against Reco Charge - Needs Work if you want to continue with this ###
//################################################################################################ 


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
    //std::cout << "chan: " << ch << " wire: " << wire_id << std::endl;
    if(mchits_v.size() <= ch )
      throw cet::exception(__PRETTY_FUNCTION__)
	<< "Channel "<<ch<<" not found in MCHit vector!"<<std::endl;

    
    // Found MCHitCollection                                                                                                                                                       
    auto const& mchits = mchits_v.at(ch);
    if(channel_MCHit_map.find(ch) == channel_MCHit_map.end()){noise_hits.push_back(hit); continue;}    

    std::vector<MCHits> MCHit_vec = channel_MCHit_map[ch];
    if( ch != mchits.Channel() )
      throw cet::exception(__PRETTY_FUNCTION__)
	<< "MCHit channel & vector index mismatch: "
	<< mchits.Channel() << " != " << ch << std::endl;



    double reco_q  = hit->PeakAmplitude();
    double reco_tot = hit->Integral();
    double mc_qsum = 0;
    double mc_q    = 0;
    double abs_dt_min = 1e9;

    
    //Loop over the MCHits on the channel to find the closest in time to the reco charge.
    for(std::vector<MCHits>::iterator iter=  MCHit_vec.begin(); iter !=  MCHit_vec.end(); ++iter){
            
      double dt =  hit->PeakTime() - iter->PeakTime();
      double abs_dt = dt;
            
      if(abs_dt<0) abs_dt *= -1;
      
      if(abs_dt < abs_dt_min) {
	abs_dt_min = abs_dt;
	mc_q   = iter->PeakCharge(); 
	mc_qsum = iter->TotalCharge(); 
      }
      else if( abs_dt == abs_dt_min) {
	mc_q = mc_q + iter->PeakCharge();
        mc_qsum = mc_qsum + iter->TotalCharge();
      }
    }// MC Hits Loop
    

    //Take Note of any Noise Hits.
    if(abs_dt_min > 5){
      noise_hits.push_back(hit);
    }
    //Otherwise plot a graphs to show the difference between reco and MCHit charge. mc_q is scaled via the calorimetry module - Might neeed Changing for ICARUS 
    else{

      if( wire_id.Plane == 0){
      peakchargediff = (mc_q* 0.02354 - reco_q)/(mc_q* 0.02354);
      totalchargediff = (mc_qsum*0.02354  - reco_tot)/(mc_qsum*0.02354 );
      TotChargeHist0->Fill(totalchargediff);
      }
      else if( wire_id.Plane == 2){
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
  
      ADCGraph->SetPoint(ADCGraph->GetN(),mc_qsum, reco_tot);
      }
  
    // std::cout << "Total Charge: " << mc_qsum*0.0224426  << " Reco Charge: " << reco_tot <<" channel num " << ch << std::endl;
  } // Reco Hits Loop


  // Fit a linear graph of the MC charge to Reco Charge to check the the scale factor. 
  ADCGraph->Draw("P");
  ADCGraph->SetName("ADCGraph");
  ADCGraph->GetXaxis()->SetTitle("MC Charge (ADC)");
  ADCGraph->GetYaxis()->SetTitle("Reco Charge (ADC)");
  ADCGraph->SetTitle("ADC Graph");
  TF1 *fit = new TF1("fit","pol1");
  ADCGraph->Fit(fit,"+rob=0.75");
  std::cout << fit->GetParameter(0) << std::endl;
  ADCGraph->Write();
  ADCGraph->Delete();
  fit->Delete();

  PeakChargeHist->Draw("same");
  PeakChargeHist->SetName("Peak Charge");
  PeakChargeHist->GetXaxis()->SetTitle("(mc_peak - reco_peak)/mc_peak");
  PeakChargeHist->SetTitle("Peak Charge");
  PeakChargeHist->Write();
  PeakChargeHist->Delete();

  TotChargeHist->Draw("same");
  TotChargeHist->SetName("TotalCharge");
  TotChargeHist->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  TotChargeHist->SetTitle("Total Charge ");
  TotChargeHist->Write();
  TotChargeHist->Delete();


  TotChargeHist0->Draw("same");
  TotChargeHist0->SetName("TotalCharge 0");
  TotChargeHist0->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  TotChargeHist0->SetTitle("Total Charge ");
  TotChargeHist0->Write();
  TotChargeHist0->Delete();


  TotChargeHist1->Draw("same");
  TotChargeHist1->SetName("TotalCharge 1");
  TotChargeHist1->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  TotChargeHist1->SetTitle("Total Charge ");
  TotChargeHist1->Write();
  TotChargeHist1->Delete();


  TotChargeHist2->Draw("same");
  TotChargeHist2->SetName("TotalCharge 2");
  TotChargeHist2->GetXaxis()->SetTitle("(mc_tot - reco_tot)/mc_tot");
  TotChargeHist2->SetTitle("Total Charge ");
  TotChargeHist2->Write();
  TotChargeHist2->Delete();

 f->Close();
 f->Delete();
//####################################################################################################################################################
//### Find the MC Trajectory of the Particle and Calculate the wires it crosses. Maybe Neglects diffusion - Now obsolete useful comparision though ###
//####################################################################################################################################################


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
  const double PX = Muon->Px();

  double angle;
  if(fThetaXZinstead){angle = TMath::ATan(PX/PZ)*180/TMath::Pi();}
  else {angle = TMath::ATan(PY/PZ)*180/TMath::Pi();}
  
  if(angle>180){angle = 360- angle;}



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
    //Calculate the metrics 
    u = (((WireStart.Z()- StartZ_TPC)*(EndY_TPC-StartY_TPC)) - ((EndZ_TPC-StartZ_TPC)*(WireStart.Y()-StartY_TPC)))/rcrossx;
    t = (((WireStart.Z() - StartZ_TPC)*(WireEnd.Y()-WireStart.Y())) - ((WireEnd.Z()-WireStart.Z())*(WireStart.Y()-StartY_TPC)))/rcrossx;


    //    std::cout <<"WireStartY = " << WireStart.Y() << " WireEndY = " << WireEnd.Y() << "PlaneID = " << wire_id  << std::endl;
    //std::cout <<"WireStartZ = "<< WireStart.Z() << " WireEndZ = " << WireEnd.Z() << "PlaneID = " << wire_id  << std::endl ;

    
    if(u<= 1 && t<=1 && u>= 0 && t>=0){
      int wire_no = wire_id.Wire;
      geo::PlaneID planeID = wire_id.planeID(); 
      WiresCrossed_map[planeID].push_back(wire_no);
      ++Wirenum;
    }
  }

  int test_RecoHits = 0;
  //run over all planes
  for(geo::PlaneID plane_it: geom->IteratePlaneIDs()){

//#################################################################                                                                                                      
//### Compare MCHits to Reco Hits - No DeltaRay Wires involved  ###                                                                                                      
//#################################################################                         

    int wires_crossed=0;
    double  missed_hits_nodelta =0;
    int RemovedMCHits=0;
    double n0_MCHits_nodelta=0;

  //Loop over the MCHits and see if there is a recostructed that is associated with it.
  for(std::map<unsigned int,std::vector<MCHits> >::iterator iter=channel_MCHit_map.begin(); iter!=channel_MCHit_map.end(); ++iter){
 
    if(geom->ChannelToWire(iter->first).at(0).planeID() != plane_it){continue;}
    else{++wires_crossed;}

    //See if the channel holds a delta ray and ignore if so. 
    unsigned int ch = iter->first; 
    sort(iter->second.begin(), iter->second.end(),sortbyPeakTime);
    
    auto search =  DeltaMCChannels_Eitherside.find(ch);
    if(search !=  DeltaMCChannels_Eitherside.end()){++RemovedMCHits; continue;} 
    
   std::map<art::Ptr<recob::Hit>, double > used_hits;
 
    //Loop over the MCHits on the channel to see if there is a reco hit. 
    for(std::vector<MCHits>::iterator jter=iter->second.begin(); jter!=iter->second.end(); ++jter){
      if(jter->PeakCharge()< charge_cut){jter = iter->second.erase(jter);continue;}
      n0_MCHits_nodelta++;
      double abs_dt_min = 1e9;
      art::Ptr<recob::Hit> associted_hit;

      if(channel_RecoHit_map[ch].size() == 0){++missed_hits_nodelta; continue;}
      //Loop over the reco hits on the chanel to find the minimum peak time. 
      for(std::vector<art::Ptr<recob::Hit> >::iterator kter=channel_RecoHit_map[ch].begin(); kter!=channel_RecoHit_map[ch].end();++kter){
 
	art::Ptr<recob::Hit> reco_hit_prt = *kter;
	if(std::find(noise_hits.begin(), noise_hits.end(), reco_hit_prt) != noise_hits.end()){continue;} 
	if(used_hits.find(reco_hit_prt) != used_hits.end()){continue;}
	double dt =  jter->PeakTime() - (*kter)->PeakTime();
	double abs_dt = dt;
	if(abs_dt<0){abs_dt *= -1;}
	if(abs_dt < abs_dt_min) {
	  
	  abs_dt_min = abs_dt;
	  associted_hit = *kter;
	}
      }
     
      //	std::cout << " abs_dt_min: " <<  abs_dt_min << " Peak Charge: "  << jter->PeakCharge() << " TotalCharge: " << jter->TotalCharge() << " Track ID: " << jter->Trackid() << " Channel: " << ch << " Peak Time: " <<  jter->PeakTime() << std::endl;

       //Find a hit within a time threshold and make sure it hasn't already been used. 
       if(abs_dt_min == 1e9){   	
	++missed_hits_nodelta;
      }
       used_hits[associted_hit] =  abs_dt_min;
    }//Loop of MCHits on the channel
  }//Channel Loops (Channel-MCHitsMap)


  raw::ChannelID_t channel;
  double number_of_wires_with_morethan_one_hit=0;
  double number_of_wires_with_one_hit=0; 
  double RecoHitNum=0;
  double MCHitNum=0;
  std::map<int,int> channelmap;


  //###################################################                                                                                                             
  //### Get Number of MC Hits/ Reco Hits PerPlane  ###                                                                                                              
  //##################################################  

  //Find the Number of noise hits on the plane.                                                                                                                                        
  int noise_hits_plane=0;
  std::map<int,int> noise_map;

  //add up the noise hits in the plane - now obsolete 
  for(std::vector<art::Ptr<recob::Hit> >::iterator  noise_it = noise_hits.begin(); noise_it != noise_hits.end(); ++noise_it){
    art::Ptr<recob::Hit> hit = *noise_it;
    geo::WireID wireid = hit->WireID();
    geo::PlaneID  PlaneID = wireid.planeID();
    if(PlaneID != plane_it){continue;}
    ++noise_hits_plane;
  }



  //Find the Number of MCHits in the Plane
  for(std::vector<MCHits>::iterator MChitIt=MC_vec.begin(); MChitIt!=MC_vec.end(); ++MChitIt){
      if(geom->ChannelToWire(MChitIt->Channel()).at(0).planeID() != plane_it){continue;}
      ++MCHitNum;
  }

  //Find the number of hits in the plane in total and place in a channel map to look for wires with more than 2 hits.
  for(std::vector<art::Ptr<recob::Hit> >::iterator hitIt = hits.begin(); hitIt != hits.end(); ++hitIt){
    art::Ptr<recob::Hit> hit = *hitIt;
    geo::WireID wireid = hit->WireID();
    geo::PlaneID  PlaneID = wireid.planeID();
    if(PlaneID != plane_it){continue;}
    RecoHitNum++;
    channel = wireid.Wire;

    //    std::cout << "channel: " <<  geom->PlaneWireToChannel(wireid.Plane,
    //			     wireid.Wire,
    //			     wireid.TPC,
    //							  wireid.Cryostat) << " wire id: " << wireid << std::endl;

    if(std::find(noise_hits.begin(), noise_hits.end(), hit) != noise_hits.end()){--channelmap[channel];}
    channelmap[channel] = ++channelmap[channel];
   }


  for(std::map<int,int>::iterator channelmap_iter=channelmap.begin();channelmap_iter!=channelmap.end();++channelmap_iter){
    if(channelmap_iter->second==1){++number_of_wires_with_one_hit;}
  }

  std::map<int,int> mt1_channelmap;  
//Find the number of channels with more than one hit from all the hits.
for(std::map<int,int>::iterator channelIt = channelmap.begin(); channelIt != channelmap.end(); ++channelIt){
  if(channelIt->second > 1){
      mt1_channelmap[channelIt->first] = channelIt->second;
      if(channelIt->first != 0){mt1_channelmap[(channelIt->first) - 1] = channelIt->second;}
    int channel_map_size_m1 = channelmap.size() - 1;
      if(channelIt->first != channel_map_size_m1){mt1_channelmap[(channelIt->first) + 1] = channelIt->second;}
      ++number_of_wires_with_morethan_one_hit;
  } 
 }


//  double  number_of_wire_that_are_hit = channelmap.size(); 
  // double  wires_that_are_crossed = WiresCrossed_map[plane_it].size();
  
  // number_of_wires_with_one_truehit = number_of_wire_that_are_truehit - number_of_wires_with_morethan_one_truehit;
//  number_of_wires_with_one_hit = number_of_wire_that_are_hit - number_of_wires_with_morethan_one_hit; 


  //Efficiencies 

     //The number of wires which see one hit/ number of wires cross  - number of wires which see more than one hit.
     double Efficiency_wire = (number_of_wires_with_one_hit)/ (wires_crossed-number_of_wires_with_morethan_one_hit);
      

     //Excluding the wires with delta ray hits. The number of MC hits - number of MC hits not matched to a Reco hit/ number of MCHits
     double  Efficiency_nodelta        = (n0_MCHits_nodelta-missed_hits_nodelta)/n0_MCHits_nodelta;
    
     // The number of Reco hits - noise / number of MC hits per plane
     double  Efficiency_seperatedelta  = (RecoHitNum -  noise_hits_plane)/MCHitNum;

     if((wires_crossed-number_of_wires_with_morethan_one_hit) !=0){
       ((Efficiency_wire_map[plane_it])[threshold_pair])[angle].push_back(Efficiency_wire);
       Efficiency_total_wire_map[threshold_pair][angle].push_back(Efficiency_wire);
      }

     if(n0_MCHits_nodelta !=0){
       ((Efficiency_nodelta_map[plane_it])[threshold_pair])[angle].push_back(Efficiency_nodelta);
       Efficiency_total_nodelta_map[threshold_pair][angle].push_back(Efficiency_nodelta);
     }
     if(MCHitNum !=0){
       ((Efficiency_seperatedelta_map[plane_it])[threshold_pair])[angle].push_back(Efficiency_seperatedelta);
       Efficiency_total_seperatedelta_map[threshold_pair][angle].push_back(Efficiency_seperatedelta);
     }


    
  
     test_RecoHits= test_RecoHits+RecoHitNum;


 std::cout << " #############################################" << std::endl;
 std::cout << "WiresCrossed = " << WiresCrossed_map[plane_it].size() << std::endl;
 std::cout << "MC Wire Cross = " << wires_crossed << std::endl;
 std::cout << "Noise_hts per plane: " << noise_hits_plane << std::endl;
 std::cout << "Efficiency Wire = " <<  Efficiency_wire << std::endl; 
 std::cout << "Efficiency Nodelta = " << Efficiency_nodelta  << std::endl;
 std::cout << "Efficiency_seperatedelta = " << Efficiency_seperatedelta << std::endl; 
 std::cout << "Number of wires with one hit: " << number_of_wires_with_one_hit << " number_of_wires_with_morethan_one_hit: " << number_of_wires_with_morethan_one_hit << std::endl;
 std::cout << "MCHitNum: " << MCHitNum << " RecoHitNum: " << RecoHitNum << "  channelmap.size(): " <<   channelmap.size()  <<" n0_MCHits_nodelta.size() " <<  n0_MCHits_nodelta<<std::endl; 
 std::cout << "num_merged_hits: " << num_merged_hits << " num_merged_n_not : "<< num_merged_n_not << std::endl;
 std::cout << " missed_hits_nodelta: " << missed_hits_nodelta <<std::endl;
 std::cout << " #############################################" << std::endl;

  }//plane loop

  std::cout << "  channel_MCHit_map.size(): " << channel_MCHit_map.size() << std::endl;
  std::cout << " channels.size(): " << channels.size() << std::endl; 
  std::cout << " MuonMCHits.size(): " << MuonMCHits.size() <<std::endl;
  std::cout << "MC_vec.size() " << MC_vec.size() << std::endl;
  std::cout << "The total number of noise hits is: " << noise_hits.size() << std::endl;
  std::cout << "angle: " <<  angle <<std::endl;
  std::cout << " likelyTrackID.size: " <<  likelyTrackID.size() << " hitssize: " << hits.size() << " mchit num: " << mchit_num <<std::endl;
  std::cout << "testRecoHits size: " << test_RecoHits <<  "  hits.size(): " <<  hits.size() << std::endl; 
  
  }//dtThreshold Loop  
  }//chargThreshold Loop

return;
  }
 

void ana::HitEff::endJob() {


  
  std::string file_string;
  std::string Theta;
  std::stringstream sstm_file;
  sstm_file << "EfficiencyGraphs_";  
  if(fThetaXZinstead){sstm_file << "ThetaXZ_"; Theta="(XZ)";}
  else{sstm_file << "_ThetaYZ_"; Theta="(YZ)";}
  sstm_file << fADC << "ADC_" << fChargeCut.size() << "_" << fdtThreshold.size();
  file_string = sstm_file.str();
  const char* namefile = file_string.c_str();

  TFile *file = new TFile(namefile,"RECREATE");

  //###########################################                                                                                                                          
  //### Individual Plane Efficiency Graphs  ###                                                                                                                            
  //########################################### 

  std::vector<std::map<geo::PlaneID, std::map<std::pair<double,double>, std::map<double,std::vector<double> > > > > Efficiencies = {Efficiency_wire_map,Efficiency_nodelta_map,Efficiency_seperatedelta_map}; 

  
  std::vector<std::string> Efficiencies_string = {"Efficiency on one Hit Wires","Efficiency Exluding Wires with Delta Rays","Efficiency to Seperate Delta Rays"};

  for(unsigned int l=0; l<Efficiencies.size(); ++l){

    //Loop over the planes 
   for( std::map<geo::PlaneID, std::map<std::pair<double,double>, std::map<double,std::vector<double> > > >::iterator k = Efficiencies[0].begin(); k != Efficiencies[0].end(); ++k){

      //Create the canvas
     std::string string_canvas;
     std::stringstream sstm_canvas;
     sstm_canvas << Efficiencies_string[l] << " PlaneID:" << (k->first);
     string_canvas = sstm_canvas.str();
     const char* name_canvas = string_canvas.c_str();
     TCanvas *c1 = new TCanvas(name_canvas,name_canvas,900,600);
     c1->cd();

      //Create the graph.
      TMultiGraph *mg_Efficiency_wire = new TMultiGraph();
      std::string string_mg;
      std::stringstream sstm_mg;
      sstm_mg << Efficiencies_string[l] << "PlaneID:" << (k->first) <<";Angle"<< Theta << " deg;Efficiency %";
      string_mg = sstm_mg.str();
      const char* name_mg = string_mg.c_str();

      //Create the legend 
      TLegend *leg = new TLegend(0.65, 0.1, 0.9, 0.35);
      leg->SetFillColor(0);
      leg->SetHeader("MC ADC Threshold and dtThreshold");
      leg->SetBorderSize(1);
      leg->SetTextSize(0.025);
      
      //Define the colour for the graphs                                                                                                                                           
      int colour_int=0;

      //Loop over the threshold.
      for( std::map<std::pair<double,double>, std::map<double,std::vector<double> > >::iterator i = k->second.begin(); i != k->second.end(); ++i){

	//Create the Graph for angle over Efficiency.
	TGraphErrors* Efficiency_para_graph = new TGraphErrors(0);

	//Name the legend depending on the threshold.
	std::string string_legend;
	std::stringstream sstm1_legend;
	sstm1_legend << (i->first).first << " ADCs " << (i->first).second << " dt Threshold";
	string_legend = sstm1_legend.str();
	const char* name_legend = string_legend.c_str();

	//Loop over the Efficiencys at the angle.
	for(std::map<double,std::vector<double> >::iterator j = (i->second).begin();  j != (i->second).end(); ++j){
    
	  //Find the mean and standard error on the mean. 
	  double angle=j->first;
	  double samplesize = (j->second).size();
	  double bias;
	  if(samplesize==1){bias=1;}
	  else{bias=std::sqrt(1/(samplesize-1));}
	  double mean   = accumulate( (j->second).begin(), (j->second).end(), 0.0)/samplesize;
	  double sq_sum = std::inner_product((j->second).begin(), (j->second).end(), (j->second).begin(), 0.0);
	  double stdev  = std::sqrt(sq_sum /samplesize - mean * mean)*bias;
	  double stderrmean = stdev/std::sqrt((j->second).size());

	  //Add a point to the graph.
	  Efficiency_para_graph->SetPoint(Efficiency_para_graph->GetN(),angle, mean);
	  Efficiency_para_graph->SetPointError(Efficiency_para_graph->GetN()-1,0, stderrmean);
	  //      std::cout << "angle: " << angle << " mean: " << mean << " stdev: " << stdev << " N: " << (j->second).size() <<std::endl;
	}//loop over vector
  

	//if you wish to save every efficiency graph seperately 
	// std::stringstream sstm1;
	// sstm1 << "Efficiency on one Hit Wires, PlaneID:" << (k->first);
	//map_string = sstm1.str();
	//const char* name = map_string.c_str();
	//Efficiency_para_graph->SetName(name);
	//Efficiency_para_graph->SetTitle(name);
	//Efficiency_para_graph->Draw("A");
	//Efficiency_para_graph->GetXaxis()->SetTitle("Angle");
	//Efficiency_para_graph->GetYaxis()->SetTitle("Efficiency");
	//Efficiency_para_graph->Write();

	//Just in case they are fractions
	++colour_int;

	Efficiency_para_graph->SetMarkerColor(colour_int);
	Efficiency_para_graph->SetMarkerStyle(8);

	//Add graph to the mutligraph
	mg_Efficiency_wire->Add(Efficiency_para_graph);
	leg->AddEntry(Efficiency_para_graph, name_legend, "p");

      }//loop over thresholds
 
      mg_Efficiency_wire->SetName(name_mg);
      mg_Efficiency_wire->SetTitle(name_mg);
      mg_Efficiency_wire->Draw("AP");
      leg->Draw();
      c1->Update();
      c1->Write();
      mg_Efficiency_wire->Write();
      mg_Efficiency_wire->Delete();
      gSystem->ProcessEvents();
      delete c1;

    }//loop over plane

  }//loop over the efficiencies


  // //#################################                                                                                                                                               
  // //### Efficiency_NoDelta Graph  ###                                                                                                                                              
  // //#################################   


  // for( std::map<geo::PlaneID, std::map<int, std::map<double,std::vector<double> > > >::iterator k = Efficiency_nodelta_map.begin(); k != Efficiency_nodelta_map.end(); ++k){
  //   std::cout << "tets5.1 " << std::endl;

  //   std::string map_string_canvas;
  //   std::stringstream sstm1_canvas;
  //   sstm1_canvas << "Efficiency_nodelta, PlaneID:" << (k->first);
  //   map_string_canvas = sstm1_canvas.str();
  //   const char* name_canvas = map_string_canvas.c_str();
  //   TCanvas *c1 = new TCanvas(name_canvas,name_canvas,900,600);
  //   c1->cd();
  //   TMultiGraph *mg_Efficiency_nodelta = new TMultiGraph();
  //   TLegend *leg = new TLegend(0.7, 0.1, 0.9, 0.3);
  //   leg->SetFillColor(0);
  //   leg->SetHeader("MC ADC Threshold");

  //   //std::string map_string_canvas;
  //   //std::stringstream sstm1_canvas;
  //   //sstm1_canvas << "PlaneID:" << (k->first);
  //   //map_string_canvas = sstm1_canvas.str();
  //   //const char* name_canvas = map_string_canvas.c_str();
  //   //TCanvas *c1 = new TCanvas(name_canvas,name_canvas,900,600);
  //   //c1->cd();
    

  //   for( std::map<int, std::map<double,std::vector<double> > >::iterator i = k->second.begin(); i != k->second.end(); ++i){
  //     std::cout << "tets5.2 " << std::endl;

  //     std::string map_string_legend;
  //     std::stringstream sstm1_legend;
  //     sstm1_legend << (i->first) << " ADCs";
  //     map_string_legend = sstm1_legend.str();
  //     const char* name_legend = map_string_legend.c_str();
  //   TGraphErrors* Efficiency_nodelta_graph = new TGraphErrors(0);

  //   for(std::map<double,std::vector<double> >::iterator j = (i->second).begin();  j != (i->second).end(); ++j){
  //     double angle=j->first;
  //     double samplesize = (j->second).size();
  //     double bias;
  //     if(samplesize==1){bias=1;}
  //     else{bias=std::sqrt(1/(samplesize-1));}
  //     double mean   = accumulate( (j->second).begin(), (j->second).end(), 0.0)/ samplesize;
  //     double sq_sum = std::inner_product((j->second).begin(), (j->second).end(), (j->second).begin(), 0.0);
  //     double stdev  = std::sqrt(sq_sum /samplesize - mean * mean)*bias;
  //     double stderrmean = stdev/std::sqrt((j->second).size());
  //     std::cout << "angle: " << angle << " mean: " << mean << " stdev: " << stdev << " N: " << (j->second).size() <<std::endl;       
  //     Efficiency_nodelta_graph->SetPoint(Efficiency_nodelta_graph->GetN(),angle, mean);
  //     Efficiency_nodelta_graph->SetPointError(Efficiency_nodelta_graph->GetN()-1,0, stderrmean);
  //   }

  //   std::string map_string;
  //   std::stringstream sstm1;
  //   sstm1 << "Efficiency Exluding Wires with Delta Rays, PlaneID:" << (k->first);
  //   map_string = sstm1.str();
  //   const char* name = map_string.c_str();

  //   Efficiency_nodelta_graph->SetMarkerColor(i->first);
  //   Efficiency_nodelta_graph->SetMarkerStyle(8);
  //   Efficiency_nodelta_graph->SetName(name);
  //   Efficiency_nodelta_graph->SetTitle(name);
  //   Efficiency_nodelta_graph->Draw("A");
  //   Efficiency_nodelta_graph->GetXaxis()->SetTitle("Angle");
  //   Efficiency_nodelta_graph->GetYaxis()->SetTitle("Efficiency");
  //   Efficiency_nodelta_graph->Write();
  //   mg_Efficiency_nodelta->Add(Efficiency_nodelta_graph);
  //   //    Efficiency_nodelta_graph->Delete();
  //   leg->AddEntry(Efficiency_nodelta_graph, name_legend, "p");
  //   }
  
  //   std::string map_string_mg;
  //   std::stringstream sstm1_mg;
  //   sstm1_mg << "MultiGraph: Efficiency Exluding Wires with Delta Rays PlaneID:" << (k->first) << ";Angle;Efficiency" ;
  //   map_string_mg = sstm1_mg.str();
  //   const char* name_mg = map_string_mg.c_str();
  //   mg_Efficiency_nodelta->SetTitle(name_mg);
  //   mg_Efficiency_nodelta->Draw("P");
  //   mg_Efficiency_nodelta->SetName(name_mg);
  //   leg->Draw();
  //   c1->Update();
  //   gPad->Update();
  //   c1->Write();
  //   mg_Efficiency_nodelta->Write();
  //   mg_Efficiency_nodelta->Delete();
    
  // }
  
  // //#######################################                                                                                                             
  // //### Efficiency_seperatedelta Graph  ###                                                                                                            
  // //#######################################                                                                                                             
              
                  

  // for( std::map<geo::PlaneID, std::map<int, std::map<double,std::vector<double> > > >::iterator k = Efficiency_seperatedelta_map.begin(); k != Efficiency_seperatedelta_map.end(); ++k){

  //   std::string map_string_canvas;
  //   std::stringstream sstm1_canvas;
  //   sstm1_canvas << "seperatedelta PlaneID:" << (k->first);
  //   map_string_canvas = sstm1_canvas.str();
  //   const char* name_canvas = map_string_canvas.c_str();
  //   TCanvas *c1 = new TCanvas(name_canvas,name_canvas,900,600);
  //   c1->cd();
  //   TMultiGraph *mg_seperatedelta = new TMultiGraph();
  //   TLegend *leg = new TLegend(0.7, 0.1, 0.9, 0.3);
  //   leg->SetFillColor(0);
  //   leg->SetHeader("MC ADC Threshold");
  //   for( std::map<int, std::map<double,std::vector<double> > >::iterator i = k->second.begin(); i != k->second.end(); ++i){

  //   TGraphErrors* Efficiency_seperatedelta_graph = new TGraphErrors(0);
  //   std::string map_string_legend;
  //   std::stringstream sstm1_legend;
  //   sstm1_legend << (i->first) << " ADCs";
  //   map_string_legend = sstm1_legend.str();
  //   const char* name_legend = map_string_legend.c_str();

  //   for(std::map<double,std::vector<double> >::iterator j = (i->second).begin();  j != (i->second).end(); ++j){

  //     double angle=j->first;
  //     double samplesize = (j->second).size();
  //     double bias;
  //     if(samplesize==1){bias=1;}
  //     else{bias=std::sqrt(1/(samplesize-1));}
  //     double mean   = accumulate( (j->second).begin(), (j->second).end(), 0.0)/samplesize;
  //     double sq_sum = std::inner_product((j->second).begin(), (j->second).end(), (j->second).begin(), 0.0);
  //     double stdev  = std::sqrt((sq_sum /samplesize)  - mean * mean)*bias;
  //     double stderrmean = stdev/std::sqrt((j->second).size());
  //     //std::cout << "angle: " << angle << " mean: " << mean << " stdev: " << stdev << " N: " << (j->second).size() <<std::endl;
  //     //std::cout << "inner product: " << sq_sum << " sample size: " << samplesize << std::endl;;
  //     Efficiency_seperatedelta_graph->SetPoint(Efficiency_seperatedelta_graph->GetN(),angle, mean);
  //     Efficiency_seperatedelta_graph->SetPointError(Efficiency_seperatedelta_graph->GetN()-1,0, stderrmean);
  //   }

  //   std::string map_string;
  //   std::stringstream sstm1;
  //   sstm1 << "Efficiency to Seperate Delta Rays, PlaneID:" << (k->first);
  //   map_string = sstm1.str();
  //   const char* name = map_string.c_str();

  //   Efficiency_seperatedelta_graph->SetMarkerColor(i->first);
  //   Efficiency_seperatedelta_graph->SetMarkerStyle(8);
  //   Efficiency_seperatedelta_graph->Draw("AP");
  //   Efficiency_seperatedelta_graph->SetName(name);
  //   Efficiency_seperatedelta_graph->GetXaxis()->SetTitle("Angle");
  //   Efficiency_seperatedelta_graph->GetYaxis()->SetTitle("Efficiency");
  //   Efficiency_seperatedelta_graph->SetTitle(name);
  //   Efficiency_seperatedelta_graph->Write();
  //   mg_seperatedelta->Add(Efficiency_seperatedelta_graph);
  //   leg->AddEntry(Efficiency_seperatedelta_graph, name_legend, "p");
  //   //Efficiency_seperatedelta_graph->Delete();
  //   }

  //   std::string map_string_mg;
  //   std::string map_string_leg1;
  //   std::stringstream sstm1_mg;
  //   std::stringstream sstm1_leg1;
  //   sstm1_mg << "MultiGraph: Efficiency to Seperate Delta Rays, PlaneID:" << (k->first) << ";Angle;Efficiency";
  //   sstm1_leg1 << "Legend: Efficiency to Seperate Delta Rays, PlaneID:" << (k->first);
  //   map_string_mg = sstm1_mg.str();
  //   const char* name_mg = map_string_mg.c_str();
  //   map_string_leg1 = sstm1_leg1.str();
  //   const char* name_leg1 = map_string_leg1.c_str();    

  //   mg_seperatedelta->SetTitle(name_mg);
  //   mg_seperatedelta->Draw("AP");
  //   leg->Draw();
  //   mg_seperatedelta->SetName(name_mg);
  //   leg->SetName(name_leg1);
  //   //    mg_seperatedelta->GetXaxis()->SetTitle("Angle");
  //   // mg_seperatedelta->GetYaxis()->SetTitle("Efficiency");


  //   //c1->BuildLegend();
  //   c1->Update();
  //   gPad->Update();
    
  //   std::string map_string_c1;
  //   std::stringstream sstm1_c1;
  //     int plane = ((k->first)).Plane;
      
  //   //std::cout << "Plane: " << plane << std::endl;
  //   sstm1_c1 << "Eff3_Plane" << plane <<".png";
  //   map_string_c1 = sstm1_c1.str();
  //    const char* name_c1 = map_string_c1.c_str();
  //   c1->Print(name_c1);
  //   mg_seperatedelta->Write();
  //   leg->Write();
  //   c1->Write();
  //   mg_seperatedelta->Delete();
  // }


  //################################                                                                                                                           
  //### Total Efficiency Graphs ###                                                                                                                           
  //###############################  

  std::vector<std::map<std::pair<double,double>, std::map<double,std::vector<double> > > >  Efficiencies_Total =  {Efficiency_total_wire_map, Efficiency_total_nodelta_map, Efficiency_total_seperatedelta_map};
  //Wire 

  //Loop over all the defined Efficiencies.
  for(unsigned int l=0; l<Efficiencies_Total.size(); ++l){

    //Start naming the canvas depending on the Efficiency.                                                                                                              
    std::string string_canvas;
    std::stringstream sstm_canvas;
    sstm_canvas << Efficiencies_string[l];
    const char* name_canvas = string_canvas.c_str();
    TCanvas *c1 = new TCanvas(name_canvas,name_canvas,900,600);
    c1->cd();

    //Create graph.                                                                                                               
    std::string string_mg;
    std::stringstream sstm_mg;
    sstm_mg << "Total " << Efficiencies_string[l];
    string_mg = sstm_mg.str();
    const char* name_mg = string_mg.c_str();
    TMultiGraph *mg_TotalEfficiency_wire = new TMultiGraph();

    //Create the legend.                                                                                                                                                            
    TLegend *leg = new TLegend(0.7, 0.1, 0.9, 0.3);
    leg->SetFillColor(0);
    leg->SetHeader("MC ADC Threshold and dtThreshold");

    //define the colour integer for the legend.                                                                                                                                    
    int colour_int =0;

    for(std::map<std::pair<double,double>, std::map<double,std::vector<double> > >::iterator k = Efficiencies_Total[l].begin();  k != Efficiencies_Total[l].end(); ++k){

      //Create the The individual efficiency graphs. 
      TGraphErrors* Efficiency_total_wire_graph = new TGraphErrors(0);

      //Name the legend depending on the threshold.                                                                                                                                
      std::string string_legend;
      std::stringstream sstm_legend;
      sstm_legend << (k->first).first << " ADCs " << (k->first).second << " dt Threshold";
      string_legend = sstm_legend.str();
      const char* name_legend = string_legend.c_str();

      //Loop over the angles to create a Efficiency value at that point. 
      for(std::map<double,std::vector<double> >::iterator j = k->second.begin();  j != k->second.end(); ++j){

	//Caclulate the mean 
	double angle=j->first;
	double samplesize = (j->second).size();
	double bias;
	if(samplesize==1){bias=1;}
	else{bias=std::sqrt(1/(samplesize-1));}
	double mean   = accumulate( (j->second).begin(), (j->second).end(), 0.0)/samplesize;
	double sq_sum = std::inner_product((j->second).begin(), (j->second).end(), (j->second).begin(), 0.0);
	double stdev  = std::sqrt(sq_sum /samplesize - mean * mean)*bias;
	double stderrmean = stdev/std::sqrt((j->second).size());
	Efficiency_total_wire_graph->SetPoint(Efficiency_total_wire_graph->GetN(),angle, mean);
	Efficiency_total_wire_graph->SetPointError(Efficiency_total_wire_graph->GetN()-1,0, stderrmean);
      }

      ++colour_int;
      Efficiency_total_wire_graph->SetMarkerColor(colour_int);
      Efficiency_total_wire_graph->SetMarkerStyle(8);

      //Used if you want to save the graphs seperately 
      // Efficiency_total_wire_graph->Draw("AP");
      // Efficiency_total_wire_graph->SetName(name);
      // Efficiency_total_wire_graph->GetXaxis()->SetTitle("Angle");
      // Efficiency_total_wire_graph->GetYaxis()->SetTitle("Efficiency");
      // Efficiency_total_wire_graph->SetTitle(name);
      // Efficiency_total_wire_graph->Write();

      //Add the graph to the multigraph
      mg_TotalEfficiency_wire->Add(Efficiency_total_wire_graph);
      leg->AddEntry(Efficiency_total_wire_graph, name_legend, "p");

    }//Loop threshold 


    //Save the graph.
    mg_TotalEfficiency_wire->SetTitle(name_mg);
    mg_TotalEfficiency_wire->Draw("AP");
    leg->Draw();
    c1->Update();
    mg_TotalEfficiency_wire->SetName(name_mg);
    c1->Write();
    mg_TotalEfficiency_wire->Write();
    mg_TotalEfficiency_wire->Delete();
    gSystem->ProcessEvents();
    delete c1;
    
  }//Loop over Efficiencies

// //No Delta 
//   TMultiGraph *mg_TotalEfficiency_nodelta = new TMultiGraph();
//   for(std::map<int, std::map<double,std::vector<double> > >::iterator k = Efficiency_total_nodelta_map.begin();  k != Efficiency_total_nodelta_map.end(); ++k){
//     TGraphErrors* Efficiency_total_nodelta_graph = new TGraphErrors(0);
//     for(std::map<double,std::vector<double> >::iterator j = k->second.begin();  j != k->second.end(); ++j){


//   double angle=j->first;
//   double samplesize = (j->second).size();
//   double bias;
//   if(samplesize==1){bias=1;}
//   else{bias=std::sqrt(1/(samplesize-1));}
//   double mean   = accumulate( (j->second).begin(), (j->second).end(), 0.0)/samplesize;
//   double sq_sum = std::inner_product((j->second).begin(), (j->second).end(), (j->second).begin(), 0.0);
//   double stdev  = std::sqrt(sq_sum /samplesize - mean * mean)*bias;
//   double stderrmean = stdev/std::sqrt((j->second).size());
//   Efficiency_total_nodelta_graph->SetPoint(Efficiency_total_nodelta_graph->GetN(),angle, mean);
//   Efficiency_total_nodelta_graph->SetPointError(Efficiency_total_nodelta_graph->GetN()-1,0, stderrmean);
//  }

//     std::string map_string;
// std::stringstream sstm2;
// sstm2 << "Total Efficiency Exluding Wires with Delta Rays" ;
// map_string = sstm2.str();
// const char* name1 = map_string.c_str();

// Efficiency_total_nodelta_graph->SetMarkerColor(fdtThreshold[0]);
// Efficiency_total_nodelta_graph->SetMarkerStyle(8);
// Efficiency_total_nodelta_graph->Draw("AP");
// Efficiency_total_nodelta_graph->SetName(name1);
// Efficiency_total_nodelta_graph->GetXaxis()->SetTitle("Angle");
// Efficiency_total_nodelta_graph->GetYaxis()->SetTitle("Efficiency");
// Efficiency_total_nodelta_graph->SetTitle(name1);
// Efficiency_total_nodelta_graph->Write();
// //Efficiency_total_nodelta_graph->Delete();
// mg_TotalEfficiency_nodelta->Add(Efficiency_total_nodelta_graph);
//   }

//   mg_TotalEfficiency_nodelta->SetTitle("MultiGraph Total Efficiency Exluding Wires with Delta Rays");
//   mg_TotalEfficiency_nodelta->Draw("AP");
//   mg_TotalEfficiency_nodelta->SetName("MultiGraph Total Efficiency Exluding Wires with Delta Rays");
//   mg_TotalEfficiency_nodelta->GetXaxis()->SetTitle("Angle");
//   mg_TotalEfficiency_nodelta->GetYaxis()->SetTitle("Efficiency");
//   mg_TotalEfficiency_nodelta->Write();
//   mg_TotalEfficiency_nodelta->Delete();



// //Seperate Delta 
//   TMultiGraph *mg_TotalEfficiency_seperatedelta = new TMultiGraph();
//   TCanvas *c1 = new TCanvas("Totalseperatedelta","Totalseperatedelta",900,600);
//   c1->cd();
//   TLegend *leg = new TLegend(0.7, 0.1, 0.9, 0.3);
//   leg->SetFillColor(0);
//   leg->SetHeader("MC ADC Threshold");


//   for(std::map<int, std::map<double,std::vector<double> > >::iterator k = Efficiency_total_seperatedelta_map.begin();  k != Efficiency_total_seperatedelta_map.end(); ++k){
//     std::string map_string_legend;
//     std::stringstream sstm1_legend;
//     sstm1_legend << (k->first) << " ADCs";
//     map_string_legend = sstm1_legend.str();
//     const char* name_legend = map_string_legend.c_str();

//     TGraphErrors* Efficiency_total_seperatedelta_graph = new TGraphErrors(0);
//     for(std::map<double,std::vector<double> >::iterator j = k->second.begin();  j != k->second.end(); ++j){

//   double angle=j->first;
//   double samplesize = (j->second).size();
//   double bias;
//   if(samplesize==1){bias=1;}
//   else{bias=std::sqrt(1/(samplesize-1));}
//   double mean   = accumulate( (j->second).begin(), (j->second).end(), 0.0)/samplesize;
//   double sq_sum = std::inner_product((j->second).begin(), (j->second).end(), (j->second).begin(), 0.0);
//   double stdev  = std::sqrt(sq_sum /samplesize - mean * mean)*bias;
//   double stderrmean = stdev/std::sqrt((j->second).size());
//   Efficiency_total_seperatedelta_graph->SetPoint(Efficiency_total_seperatedelta_graph->GetN(),angle, mean);
//   Efficiency_total_seperatedelta_graph->SetPointError(Efficiency_total_seperatedelta_graph->GetN()-1,0, stderrmean);
//  }

// std::string map_string;
// std::stringstream sstm3;
// sstm3 << "Total Efficiency to Seperate Delta Rays";
// map_string = sstm3.str();
// const char* name2 = map_string.c_str();



// Efficiency_total_seperatedelta_graph->SetMarkerColor(k->first);
// Efficiency_total_seperatedelta_graph->SetMarkerStyle(8);
// Efficiency_total_seperatedelta_graph->Draw("AP");
// Efficiency_total_seperatedelta_graph->SetName(name2);
// Efficiency_total_seperatedelta_graph->GetXaxis()->SetTitle("Angle");
// Efficiency_total_seperatedelta_graph->GetYaxis()->SetTitle("Efficiency");
// Efficiency_total_seperatedelta_graph->SetTitle(name2);
// Efficiency_total_seperatedelta_graph->Write();
// // Efficiency_total_seperatedelta_graph->Delete();
//  mg_TotalEfficiency_seperatedelta->Add(Efficiency_total_seperatedelta_graph);
//  leg->AddEntry(Efficiency_total_seperatedelta_graph, name_legend, "p");
//  }


//   mg_TotalEfficiency_seperatedelta->SetTitle("MultiGraph Total Efficiency to Seperate Delta Rays");
//   mg_TotalEfficiency_seperatedelta->Draw("AP");
//   leg->Draw();
//   leg->SetName("Legend: Total Efficiency to Seperate Delta Rays");
//   c1->Update();
//   gPad->Update();
//   mg_TotalEfficiency_seperatedelta->SetName("MultiGraph Total Efficiency to Seperate Delta Rays");
//   mg_TotalEfficiency_seperatedelta->GetXaxis()->SetTitle("Angle");
//   mg_TotalEfficiency_seperatedelta->GetYaxis()->SetTitle("Efficiency");
//   mg_TotalEfficiency_seperatedelta->Write();
//   leg->Write();
//   c1->Write();
//   mg_TotalEfficiency_seperatedelta->Delete();
  
    

 delete file;


  return;  
}

DEFINE_ART_MODULE(ana::HitEff)


