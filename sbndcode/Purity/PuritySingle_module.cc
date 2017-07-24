////////////////////////////////////////////////////////////////////////
// Class:       PuritySingle
// Module Type: analyzer
// File:        PuritySingle_module.cc
//
// Modified from Roberto Acciarri's LArIAT code by Tom Brooks
////////////////////////////////////////////////////////////////////////

// ##########################
// ### Framework includes ###
// ##########################
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Principal/Event.h" 
#include "fhiclcpp/ParameterSet.h" 
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Principal/Handle.h" 
#include "canvas/Persistency/Common/Ptr.h" 
#include "canvas/Persistency/Common/PtrVector.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h" 
#include "art/Framework/Services/Optional/TFileService.h" 
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "canvas/Persistency/Common/FindOneP.h" 
#include "canvas/Persistency/Common/FindManyP.h"
#include "messagefacility/MessageLogger/MessageLogger.h" 
//#include "cetlib/maybe_ref.h"

// ########################
// ### LArSoft includes ###
// ########################
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h" // Changed from larcoreobj to larcore (could cause problems)
#include "larcoreobj/SimpleTypesAndConstants/RawTypes.h" // raw::ChannelID_t //  Changed from larcoreobj to larcore (could cause problems)
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/CryostatGeo.h"
#include "larcorealg/Geometry/TPCGeo.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireGeo.h"
#include "lardataobj/RecoBase/Wire.h" // Changed from lardataobj to lardata
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/TrackHitMeta.h"
#include "lardataobj/RecoBase/Vertex.h"
#include "lardataobj/RecoBase/SpacePoint.h" // Up to here
#include "lardata/ArtDataHelper/TrackUtils.h"
#include "lardata/DetectorInfoServices/LArPropertiesService.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardata/Utilities/AssociationUtil.h"

//#include "RawData/ExternalTrigger.h"
#include "lardataobj/RawData/RawDigit.h" //remove obj (ro)
#include "lardataobj/RawData/raw.h" // ro
#include "larsim/MCCheater/BackTracker.h"
#include "lardataobj/Simulation/SimChannel.h" // ro
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "larevt/Filters/ChannelFilter.h"
#include "lardataobj/AnalysisBase/Calorimetry.h" // ro
#include "lardataobj/AnalysisBase/ParticleID.h" // ro
#include "larreco/RecoAlg/TrackMomentumCalculator.h"
#include "lardataobj/RecoBase/Shower.h" // ro
#include "lardataobj/RecoBase/EndPoint2D.h" // ro
#include "lardataobj/MCBase/MCShower.h" // ro
#include "lardataobj/MCBase/MCStep.h" // ro
#include "larreco/Calorimetry/CalorimetryAlg.h"

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

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <utility>

double likely(const double *par)
{

   // Numeric constants
   double mpshift  = -0.22278298;       // Landau maximum 
   int index = 0;
   const double tau=par[0];
   const double sigma=par[1];
   const double dqdxo=par[2];
   double mpc;             // Corrected most probable value
   double dqdx;            // charge after tau scaling
   double prob;            // Landau probability
   double lik;             // likelihood
   double aa;              // for getting times from text file
   double bb;              // for getting charges from text file

   vector<double> charge;
   vector<double> dtime;

   ifstream file ("tmpdontcareaboutthis.txt");

   // Read in relevant data from text file (can't add more arguments to function as that would stop it working with the minimization
   while (file.good() && !file.eof())
   {
      file >> aa >> bb;
      dtime.push_back(aa);
      charge.push_back(bb); 
   }

   //Make sure vectors are the same size
   index=dtime.size()-1;
   dtime.resize(index);
   charge.resize(index); 

   lik=0; // Initialize
   for (int ii = 0; ii<index; ++ii)              // Loop over hits
   { 
      // Calculate corrected charge per unit length
      dqdx=dqdxo*TMath::Exp(-(dtime.at(ii)/tau));
      // Calculate corrected most probable value
      mpc = dqdx-mpshift*sigma;
      // Probability that a value of charge is observed given that the charge has a Landau distribution
      prob = TMath::Landau(charge.at(ii),mpc,sigma,kTRUE);
      // Negative likelihood
      lik-=TMath::Log(prob);

   }     // loop over hits   

   file.close();
   dtime.clear();
   charge.clear();     

   return lik;
} // End of function likely()

namespace sbnd
{
  class PuritySingle;
}

class sbnd::PuritySingle : public art::EDAnalyzer {
public:
  explicit PuritySingle(fhicl::ParameterSet const & p);
  virtual ~PuritySingle();

  // Do the analysis
  void analyze(art::Event const & e) override;

  // Called at very start of analysis
  void beginJob();
  // Called at very end of analysis
  void endJob();
  // Set some parameters from input FHiCL file
  void reconfigure(fhicl::ParameterSet const & p);

private:
  // Reset some numbers
  void ResetVars();

  FILE *values_file;

  // Declare all Data members
  int run; // run number
  int subrun; // subrun number
  int event; // event number
  int tpcno; // tpc number
  bool   fVerbose;                /// Print - not print some output
  int    nhits; // number of hits
  int    ntracks_reco; // number of tracks
  int    hit_plane[20000]; // the wire plane associated to the hit // CHANGE THESE TO VECTORS AND MEASURE THE DIFFERENCE IN SPEED
  int    hit_wire[20000]; // the wire associated to the hit
  int    hit_trkkey[20000]; // the track number associated to the hit
  int    hit_TPC[20000]; // Which TPC the hit is in
  double hit_peakT[20000]; // the time associated to the hit
  double hit_charge[20000]; // the charge associated to the hit
  double trkpitch[1000][3]; // Pitch of tracks in two planes // SBND HAS THREE

  double startt; //start time of track
  double endt; // end time of track
  double startw; // start wire of track
  double endw; // end wire of track
  int tothit; // total number of hits
  int fpretrig;             // number of time ticks of pre trigger
  double avfittau;            // average tau from fit
  double eavfittau;            // error on average tau from fit
  double avtaufirst; double avtausecond;           // final average tau
  double errtaufirst; double errtausecond;           // error on final average tau
  double chisqr;     // chi squared per ndof for linear fit to wire vs time of track
  double fchargecut;        // cut on hitcharge
  size_t fhitcutf;          // first cut on number of hits
  int fhitcuts;             // second cut on the number of hits 
  double flinfitcut;        // tolerance for recursive linear fit
  double fchisqcut;         // cut on the chi squares per ndof
  double fwirenumcut;       // cut on the extent of the track in wire number
  double ftimecut;          // cut on the extent of the track in time
  double fitvalue; // value from linear track fit
  double lik; // Likelihood
  double tau; // Electron lifetime
  double mpc;             // Corrected most probable value
  double dqdx;            // dQ0/dx after tau scaling
  double prob; // probability of observing charge from Landau dist
  double sigminoserrlow[3]; // Minimization errors
  double sigminoserrhi[3]; // Minimization errors
  double lowtaulimit;     // lower limit on tau minimization
  double hitaulimit;      // upper limit on tau minimization
  double lowsigmalimit;   // lower limit on sigma minimization
  double hisigmalimit;    // upper limit on sigma minimization
  double lowdqdxolimit;   // lower limit on dqdxo minimization
  double hidqdxolimit;    // upper limit on dqdxo minimization
  double hitaufirst; double hitausecond; // high limit of lifetime
  double lowtaufirst; double lowtausecond; // low limit of lifetime 
  double expo0; // exp fit parameter
  double expo1; // exp fit parameter
  double landau1; // landau fit parameter
  double lowcut; // lower cut limit for hit charge
  double hicut; // high cut limit for hit charge
  double fsampling;   // TPC fsampling rate for time ticks to microseconds conversion

  std::vector<double> fvariable; // Minimization starting values
  std::vector<double> fstep; // Minimization step sizes
  std::pair<std::vector<double>,std::vector<double>> tothitvecpair; // record total hits from each track
  std::vector<double> time; // hit times that pass linear fit of time vs wire 
  std::vector<double> wire; // hit wires that pass linear fit of time vs wire
  std::vector<double> charge; // hit charges that pass lin fit
  std::vector<double> ftime; // hit times that pass hit charge cut
  std::vector<double> fwire; // hit wires that pass hit charge cut
  std::vector<double> fcharge; // hit charge that pass hit charge cut

  std::pair<std::vector<double>,std::vector<double>> minvecpair; // likelihood value at minimum
  std::pair<std::vector<double>,std::vector<double>> dqdxovecpair; // corrected charge from minimization 2TPC
  std::pair<std::vector<double>,std::vector<double>> edqdxovecpairlow; // low error on corrected charge from minimization 2TPC
  std::pair<std::vector<double>,std::vector<double>> edqdxovecpairhi; // high error on corrected charge 2TPC
  std::pair<std::vector<double>,std::vector<double>> tauvecpair; // electron lifetime from min 2TPC
  std::pair<std::vector<double>,std::vector<double>> etauvecpairlow; // low error on electron lifetime 2TPC
  std::pair<std::vector<double>,std::vector<double>> etauvecpairhi; // high error on electron lifetime 2TPC
  std::pair<std::vector<double>,std::vector<double>> sigmavecpair; // landau width from minimization 2TPC
  std::pair<std::vector<double>,std::vector<double>> esigmavecpairlow; // low error on landau width 2TPC
  std::pair<std::vector<double>,std::vector<double>> esigmavecpairhi; // high error on landau width 2TPC

  TH1D *hChiSq; // all chisq
  TH1D *hChiSqSelected; // selected chisq
  TH1D *hGrad; // all gradient
  TH1D *hGradSelected; // selected gradient
  TH1D *hCharge; // hit charge/track pitch

  std::string fHitsModuleLabel;
  std::string fTrackModuleLabel;

  ROOT::Math::Minimizer* min = 
     ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");

  int fNevents;

}; // End of class PuritySingle

sbnd::PuritySingle::PuritySingle(fhicl::ParameterSet const & pset) : EDAnalyzer(pset)
{
  this->reconfigure(pset);
} // End of constructor

sbnd::PuritySingle::~PuritySingle()
{
  // No dynamic memory to clean up
} //End of Destructor

void sbnd::PuritySingle::reconfigure(fhicl::ParameterSet const & pset)
{
  // Set some parameters from the FHiCL file
  fHitsModuleLabel             = pset.get< std::string >("HitsModuleLabel");
  fTrackModuleLabel            = pset.get< std::string >("TrackModuleLabel");

  fchargecut                   = pset.get< double               >("ChargeCut");
  fhitcutf                     = pset.get< size_t               >("FirstHitCut");
  fhitcuts                     = pset.get< int                  >("SecondHitCut");
  flinfitcut                   = pset.get< double               >("LinearFitCut");
  fchisqcut                    = pset.get< double               >("ChiSquaredCut");
  fwirenumcut                  = pset.get< double               >("WireNumberCut");
  ftimecut                     = pset.get< double               >("TimeCut");
  fsampling                    = pset.get< double               >("SamplingTime");
  fpretrig                     = pset.get< int                  >("PreSampling");
  fvariable                    = pset.get< std::vector<double>  >("Variable");
  fstep                        = pset.get< std::vector<double>  >("Step");
  fVerbose                     = pset.get< bool                 >("Verbose", false);

  fNevents                     = pset.get< int                  >("NumberEvents");

  return;
}

void sbnd::PuritySingle::analyze(art::Event const & evt)
{
  // Reset important variables like run number, etc
  ResetVars();

  // Create a multidimensional wrapper for the likelihood function so it can be minimized
  ROOT::Math::Functor f(&likely, 3);

  // Get run number
  run = evt.run();
  // Get sub-run number
  subrun = evt.subRun();
  // Get event number
  event = evt.id().event();

  std::cout << std::endl
            << "=========================================" << std::endl
            << "Run = " << run << ", SubRun = " << subrun << ", Evt = " << event << std::endl
            << "=========================================" << std::endl 
            << std::endl;

  // #####################################
  // ### Getting the Track Information ###
  // #####################################
  art::Handle< std::vector<recob::Track> > trackListHandle; //<---Define trackListHandle as a vector of recob::Track objects
  std::vector<art::Ptr<recob::Track> > tracklist; //<---Define tracklist as a pointer to recob::tracks

  // === Filling the tracklist from the tracklistHandle ===
  if (evt.getByLabel(fTrackModuleLabel,trackListHandle))
     {art::fill_ptr_vector(tracklist, trackListHandle);}

  // ###################################
  // ### Getting the Hit Information ###
  // ###################################
  art::Handle< std::vector<recob::Hit> > hitListHandle; //<---Define hitListHandle as a vector of recob::Track objects
  std::vector<art::Ptr<recob::Hit> > hitlist; //<---Define tracklist as a pointer to recob::tracks

  // === Filling the hitlist from the hitlistHandle ===
  if (evt.getByLabel(fHitsModuleLabel,hitListHandle))
     {art::fill_ptr_vector(hitlist, hitListHandle);}

  // ##########################################################
  // ### Grabbing associations for use later in the AnaTool ###
  // ##########################################################

  // === Associations between hits and raw digits ===
  art::FindOneP<raw::RawDigit>       ford(hitListHandle,   evt, fHitsModuleLabel);

  // === Association between Tracks and 2d Hits ===
  art::FindManyP<recob::Track>       fmtk(hitListHandle,   evt, fTrackModuleLabel);


  // ----------------------------------------------------------------------------------------------------------------------------
  // ----------------------------------------------------------------------------------------------------------------------------
  //                                                        FILLING THE EVENT INFORMATION
  // ----------------------------------------------------------------------------------------------------------------------------
  // ----------------------------------------------------------------------------------------------------------------------------
  if(fVerbose) std::cout << "Tracklist size " << tracklist.size() << std::endl;

  ntracks_reco=tracklist.size();

  for(size_t i=0; i<tracklist.size();++i)
  {
    if(fVerbose) std::cout << "Track: " << i <<std::endl;

    // ###########################################
    // ### Calculating the pitch for the track ###
    // ###########################################

    // === Looping over our two planes (0&1 == Induction, 2 == Collection) ===
    for (int j = 0; j<3; ++j)
    {
      // ### Putting in a protective try in case we can't set the pitch ###
      try
      {
        // ### If we are in the induction plane calculate the tracks pitch in that view ###
        if (j==0) trkpitch[i][j] = lar::util::TrackPitchInView(*tracklist[i], geo::kU);
        // ### If we are in the collection plane calculate the tracks pitch in that view ###
        else if (j==1) trkpitch[i][j] = lar::util::TrackPitchInView(*(tracklist[i]), geo::kV);
        else if (j==2) trkpitch[i][j] = lar::util::TrackPitchInView(*(tracklist[i]), geo::kZ);
      }//<---End Try statement
      catch( cet::exception &e)
      {
        mf::LogWarning("PuritySingleOnline")<<"caught exeption "<<e<<"\n setting pitch to 0";
        trkpitch[i][j] = 0;
      }//<---End catch statement
    }// <---End looping over planes (j)
  }// <---End looping over tracks   

  nhits = hitlist.size();
  if(fVerbose) std::cout << "Total number of hits: " << nhits << std::endl;

  for (size_t i = 0; i<hitlist.size() && int(i)< 20000; ++i)
  {
    //cet::maybe_ref<raw::RawDigit const> rdref(ford.at(i));
    geo::WireID wireid = hitlist[i]->WireID();
    hit_plane[i]   = wireid.Plane;
    hit_wire[i]    = wireid.Wire;
    hit_TPC[i]     = wireid.TPC;
    hit_peakT[i]   = hitlist[i]->PeakTime();

    double rms = hitlist[i]->RMS();
    if(rms<8) rms = 8;

    double noise_total(0);
    int noise_samples(0);
    double raw_charge(0);

    if(hit_plane[i]!=2){
      hit_charge[i] = hitlist[i]->Integral();
    }
    else {
      // Obtain the raw digit data associated with the hit wire
      const art::Ptr<raw::RawDigit> raw_data = ford.at(i);
      Int_t data_size = raw_data->Samples();
      // The pedestal is the extra gain on the wire that needs to be subtracted (slightly wrong)
      float pdstl = raw_data->GetPedestal();

      // Calculate the correction to the pedestal
      for(int ii=0; ii<data_size; ++ii){
        double raw_charge_element1 = raw_data->ADC(ii) - pdstl;
        // Loop over noise (everything outside of +/- 3sigma of the signal)
        // Could be other hits on the wire
        if(ii<(hit_peakT[i]-3*rms)||ii>(hit_peakT[i]+3*rms)){
          noise_total += raw_charge_element1;
          noise_samples += 1;
        }
      }
      double new_pdstl = noise_total/noise_samples;
      // Loop over the ROI (+/- 3sigma around the peak time)
      for(int ii=0; ii<6*rms; ++ii){
        try{raw_charge += raw_data->ADC(hit_peakT[i]-(3*rms)+ii) - pdstl - new_pdstl;}
        catch(...){raw_charge += 0;}
      }
      // The preamp makes the peak the raw charge so calculate that by looking at the area of the graph
      hit_charge[i] = ((6.24150975*10e18)/10e15)* (1/39.3)*raw_charge/(rms*TMath::Sqrt(2*M_PI));
    }

    if (fmtk.isValid())
    {
      if (fmtk.at(i).size()!=0) hit_trkkey[i] = fmtk.at(i)[0].key(); 
    }

  }

  int isUsed = rand() % 10 + 1;

  //LOOP OVER TRACKS FROM HERE
  for (int trkn = 0; trkn<ntracks_reco; ++trkn){
  for (int tpcnum = 0; tpcnum<2; ++tpcnum){

  if (isUsed > fNevents) continue;

  //double azimuth = tracklist[trkn]->AzimuthAngle();

    // ----------------------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------------------------------------------------
    //                                                        PURITY CODE
    // ----------------------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------------------------------------------------
    ftime.clear();
    fwire.clear();
    fcharge.clear();
    chisqr=0.0;
    lowcut = 0.0;
    hicut = 0.0;
    tpcno = -1;
    startt = 0;
    endt = 0;
    startw = 0;
    endw = 0;

    for (int ij = 0; ij<nhits; ++ij)              // Loop over hits
    { 
      if (hit_plane[ij]==2&&hit_trkkey[ij]==trkn&&(hit_charge[ij]/trkpitch[trkn][2])<fchargecut&&hit_TPC[ij]==tpcnum)   // Selecting collection plane, first track of the plane
      {
        if (tpcno == -1) tpcno = hit_TPC[ij];
        hCharge->Fill(hit_charge[ij]/trkpitch[trkn][2]);
        ftime.push_back((hit_peakT[ij]-fpretrig)*fsampling);
        fwire.push_back(hit_wire[ij]);
        fcharge.push_back(hit_charge[ij]/trkpitch[trkn][2]);
      }  // end selection collection plane and long tracks
    }     // loop over hits  

    // Calculate extent of track in wire number and time
    if(!ftime.empty()) {
      auto wresult = std::minmax_element(fwire.begin(), fwire.end());
      startw = fwire[(wresult.first-fwire.begin())];
      endw = fwire[(wresult.second-fwire.begin())];
      auto result = std::minmax_element(ftime.begin(), ftime.end());
      startt = ftime[(result.first-ftime.begin())];
      endt = ftime[(result.second-ftime.begin())];
    }
/*
    if ((azimuth>0.87&&azimuth<2.27)||(azimuth>4.01&&azimuth<5.41))
    {
      continue;
    }
*/
    // Make cut on time extent of track
    if (endt - startt < ftimecut)
    {
      if(fVerbose) std::cout << endt - startt << " Not a through going muon, skipping..." << std::endl;
      continue;
    }

    // Make cut on wire number extent of track
    if (endw - startw < fwirenumcut)
    {
      if(fVerbose) std::cout << "Not enough hit wires, skipping..." << std::endl;
      continue;
    }

    if(fVerbose) std::cout << "Hits in track " << trkn << ": " << ftime.size() << std::endl;

    if (ftime.size() < fhitcutf || fwire.size() < fhitcutf || fcharge.size() < fhitcutf)
    {
      if(fVerbose) std::cout << "Not enough hits, skipping..." << endl;
      continue;
    }
         
    // Make a fit of wire VS time to find whether track is straight or not 
    TGraph *wgr = new TGraph(ftime.size(),&ftime[0],&fwire[0]);
    TF1 *wfun = new TF1("wfun","pol1",0,400);
    wgr->Fit("wfun");

    for(int n=0; n<2; n++)  // make fit twice to make sure to pick up every hit
    {
      tothit=0;
      time.clear();
      wire.clear();
      charge.clear();

      for(size_t i=0; i< ftime.size(); i++)  //select only hits close to the fit
      {
        fitvalue=wfun->GetParameter(0)+(ftime[i]*wfun->GetParameter(1));

        if(TMath::Abs((fwire[i]-fitvalue)/fwire[i])<flinfitcut) //changed from 0.02 to scale with change in sampling time (flinfitcut)
        {
          tothit++;
          time.push_back(ftime[i]);
          wire.push_back(fwire[i]);
          charge.push_back(fcharge[i]);
        }
      }  //select only hits close to the fit

      TGraph *wgr2 = new TGraph(time.size(),&time[0],&wire[0]);
      wgr2->Fit("wfun","R");//,"N");

    } 

    std::cout<<"Hits after linear fit = "<<time.size()<<std::endl;

    chisqr = wfun->GetChisquare()/wfun->GetNDF();  // obtain chi^2 divided by number of degrees of freedom

    if(fVerbose) std::cout << "ChiSq/NDoF for track 1: " << chisqr << std::endl;

    if(chisqr!=chisqr) continue;                   // skip the event if the chi square is a nan value

    if(fVerbose) std::cout << "Track angle: " << wfun->GetParameter(1) << std::endl;

    hGrad->Fill(wfun->GetParameter(1)); 
    hChiSq->Fill(chisqr);     

    //double angle = wfun->GetParameter(1);
   
    ////////////////////////////////////////////////////////////////////////////////
    ///              Here I select the events for the minimization               ///
    ////////////////////////////////////////////////////////////////////////////////
    
    if (tothit>=fhitcuts && chisqr<fchisqcut && chisqr!=0.0 /*&& (angle>0.25 ||angle<-0.25)*/)    
    {  
      if(fVerbose) std::cout << "Track passed selection!" << std::endl;
      if (tpcno == 0) tothitvecpair.first.push_back(tothit);
      else if (tpcno == 1) tothitvecpair.second.push_back(tothit);
      hGradSelected->Fill(wfun->GetParameter(1)); 
      hChiSqSelected->Fill(chisqr);          

      // Make a fit of charge VS time to find charge at time=0, i.e., dqdxo 
      TGraph *gr = new TGraph(time.size(),&time[0],&charge[0]);
      TF1 *fun = new TF1("fun","expo",0,400);
      gr->Fit("fun"); //,"B");//,"N");

      expo0=fun->GetParameter(0);
      expo1=fun->GetParameter(1);

      TH1D *h1 = new TH1D("h1", "Charge distance", 200, -4000, +4000);

      for (size_t i = 0; i<charge.size(); ++i)  h1->Fill(charge[i]-TMath::Exp(expo0+(expo1*time[i])));

      TF1 *gfun = new TF1("gfun","landau",-2000,4000);
      h1->Fit("gfun");

      landau1=gfun->GetParameter(2);

      gr->Draw("AP");

      TFile *tmpf = new TFile("singleTrackAvT.root","RECREATE");
      gr->Write("AmpvTime");
      tmpf->Close();

      ////////////////////////////////////////////////////////////////////////////////
      ///  Now I have the cut, move on writing the file for the minimization       ///
      ////////////////////////////////////////////////////////////////////////////////

      if(expo1 < 0.0)  // I don't want an exponential fit of the charge going upward (i.e. more charge at the end of drift time rather than bottom)
      {
        values_file=fopen("tmpdontcareaboutthis.txt","w");

        for (size_t i = 0; i<charge.size(); i++)
        {
          if((4*landau1)<1200) lowcut=TMath::Exp(expo0+(expo1*time[i]))-(0.5*landau1);//change from 0.5 back to 4
          else lowcut=TMath::Exp(expo0+(expo1*time[i]))-1200;
          if((4*landau1)<1200) hicut=TMath::Exp(expo0+(expo1*time[i]))+(0.5*landau1);
          else hicut=TMath::Exp(expo0+(expo1*time[i]))+1200;
          if(charge[i] > lowcut && charge[i] < hicut && time[i] > 12.5 && time[i] < 1262.5) fprintf(values_file,"%6.2f %6.0f\n",time[i]-12.5,charge[i]);
        }
              
        fclose(values_file);
      }
      try{ 
      ////////////////////////////////////////////////////////////////////////////////
      ///////////////////     Minimization starts HERE       /////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      min->SetFunction(f);

      // Set the free variables to be minimized!
      min->SetVariable(0,"Tau",fvariable[0], fstep[0]);
      min->SetVariable(1,"Sigma",fvariable[1], fstep[1]);
      min->SetVariable(2,"dqdx0",fvariable[2], fstep[2]);

      lowtaulimit=10;     // lower limit on tau minimization
      hitaulimit=10000;      // upper limit on tau minimization
      lowsigmalimit=0;   // lower limit on sigma minimization
      hisigmalimit=6000;    // upper limit on sigma minimization
      lowdqdxolimit=100;   // lower limit on dqdxo minimization
      hidqdxolimit=40000;    // upper limit on dqdxo minimization

      min->SetVariableLimits(0,lowtaulimit,hitaulimit);
      min->SetVariableLimits(1,lowsigmalimit,hisigmalimit);
      min->SetVariableLimits(2,lowdqdxolimit,hidqdxolimit);
      min->SetVariableValue(2,TMath::Exp(fun->GetParameter(0)));

      // do the minimization
      min->Minimize();

      const double *xs = min->X();
      if( xs[0]!=xs[0] || xs[1]!=xs[1] || xs[2]!=xs[2] ) 
      {
        if(fVerbose) std::cout << "MINIMIZATION GAVE NAN VALUE!!!! Skipping the event" << endl; 
        continue;
      }

      // Get Errors from MiNOS
      min->SetErrorDef(0.5); // If you want the n-sigma level, you have to put here n*n/2. So for 1 sigma, you put 0.5. If you have likelihood. If you have chi2 fit, then you remove the /2. See also https://root.cern.ch/root/html/ROOT__Minuit2__FCNBase.html

      for (int l=0; l<3; l++)
      {
        sigminoserrlow[l] = 0;
        sigminoserrhi[l] = 0;
      }

      min->GetMinosError(0, sigminoserrlow[0], sigminoserrhi[0]); // Get MINOS errors accordingly.
      min->GetMinosError(1, sigminoserrlow[1], sigminoserrhi[1]); // Get MINOS errors accordingly.
      min->GetMinosError(2, sigminoserrlow[2], sigminoserrhi[2]); // Get MINOS errors accordingly.

      if (xs[0]>lowtaulimit+1   && xs[0]<hitaulimit-1   &&        // Fill vectors only if values are not at limit
          xs[1]>lowsigmalimit+1 && xs[1]<hisigmalimit-1 &&
          xs[2]>lowdqdxolimit+1 && xs[2]<hidqdxolimit-1) 
      {
        if (tpcno == 0){
          tauvecpair.first.push_back(1/((1/xs[0])+1.413e-5));
          etauvecpairlow.first.push_back(TMath::Sqrt(sigminoserrlow[0]*sigminoserrlow[0]+xs[0]*xs[0]*xs[0]*xs[0]*2.99e-6*2.99e-6));
          etauvecpairhi.first.push_back(TMath::Sqrt(sigminoserrhi[0]*sigminoserrhi[0]+xs[0]*xs[0]*xs[0]*xs[0]*2.99e-6*2.99e-6));
          sigmavecpair.first.push_back(xs[1]);
          esigmavecpairlow.first.push_back(-sigminoserrlow[1]);
          esigmavecpairhi.first.push_back(sigminoserrhi[1]);
          dqdxovecpair.first.push_back(xs[2]);
          edqdxovecpairlow.first.push_back(-sigminoserrlow[2]);
          edqdxovecpairhi.first.push_back(sigminoserrhi[2]);
          minvecpair.first.push_back(min->MinValue());
        }
        else if (tpcno == 1){
          tauvecpair.second.push_back(1/((1/xs[0])+1.413e-5));
          etauvecpairlow.second.push_back(TMath::Sqrt(sigminoserrlow[0]*sigminoserrlow[0]+xs[0]*xs[0]*xs[0]*xs[0]*2.99e-6*2.99e-6));
          etauvecpairhi.second.push_back(TMath::Sqrt(sigminoserrhi[0]*sigminoserrhi[0]+xs[0]*xs[0]*xs[0]*xs[0]*2.99e-6*2.99e-6));
          sigmavecpair.second.push_back(xs[1]);
          esigmavecpairlow.second.push_back(-sigminoserrlow[1]);
          esigmavecpairhi.second.push_back(sigminoserrhi[1]);
          dqdxovecpair.second.push_back(xs[2]);
          edqdxovecpairlow.second.push_back(-sigminoserrlow[2]);
          edqdxovecpairhi.second.push_back(sigminoserrhi[2]);
          minvecpair.second.push_back(min->MinValue());
        }
        else std::cout << "Someting went wrong with TPC labelling" << std::endl;
      }   // End fill vectors only if values are not at limit
      } catch(...){continue;}

      min->Clear();
    } // End Event selection 
    else continue;

  } // End of looping over TPCs
  }// END OF LOOPING OVER TRACKS

} // End of analyze()

void sbnd::PuritySingle::beginJob()
{
  art::ServiceHandle<art::TFileService> tfs;

  hChiSq = tfs->make<TH1D>("hChiSq", "Chi Square", 100, 0, 10); //Check these limits are appropriate
  hChiSqSelected = tfs->make<TH1D>("hChiSqSelected", "Chi Square", 100, 0, 10);
  hGrad = tfs->make<TH1D>("hGrad", "Track Angle", 100, -3, 3);
  hGradSelected = tfs->make<TH1D>("hGradSelected", "Track Angle", 100, -3, 3);
  hCharge = tfs->make<TH1D>("hCharge", "Hit Charge", 600, 0, 6000);  // 10 was 6000

  hChiSqSelected->SetLineColor(2);
  hGradSelected->SetLineColor(2);

  tothitvecpair.first.clear(); tothitvecpair.second.clear();
  minvecpair.first.clear(); minvecpair.second.clear();
  dqdxovecpair.first.clear(); dqdxovecpair.second.clear();
  edqdxovecpairlow.first.clear(); edqdxovecpairlow.second.clear();
  edqdxovecpairhi.first.clear(); edqdxovecpairhi.second.clear();
  tauvecpair.first.clear(); tauvecpair.second.clear();
  etauvecpairlow.first.clear(); etauvecpairlow.second.clear();
  etauvecpairhi.first.clear(); etauvecpairhi.second.clear();
  sigmavecpair.first.clear(); sigmavecpair.second.clear();
  esigmavecpairlow.first.clear(); esigmavecpairlow.second.clear();
  esigmavecpairhi.first.clear(); esigmavecpairhi.second.clear();

  // set tolerance , etc...
  min->SetMaxFunctionCalls(10000000); // for Minuit/Minuit2 
  min->SetMaxIterations(10000);  // for GSL 
  min->SetTolerance(0.0001);
  min->SetPrintLevel(1);

} // End of beginJob()

void sbnd::PuritySingle::endJob()
{
  gSystem->Exec("rm tmpdontcareaboutthis.txt");

  if(fVerbose)
  {
    std::cout << endl 
         << endl
         << "////////////////////////////////////////////////////////////" << endl
         << "/////                  SUMMARY                         /////" << endl
         << "////////////////////////////////////////////////////////////" << endl
         << "# Tracks selected in first TPC: " << tauvecpair.first.size() << endl
         << "# Tracks selected in second TPC: " << tauvecpair.second.size() << endl
         << endl 
         << endl;
  }

  double avtau = 0.0;
  double hitau=0.0;
  double lowtau=0.0;
  double tothits=0.0;

  for(size_t s=0; s<tauvecpair.first.size(); s++)
  {

    avtau+=1/((1/tauvecpair.first[s]));
    hitau+=tauvecpair.first[s]+etauvecpairhi.first[s];
    lowtau+=tauvecpair.first[s]-etauvecpairlow.first[s];
    tothits+=tothitvecpair.first[s];

    if(fVerbose)
    {
      std::cout << " First TPC:" << std::endl << " Number of hits passing selection: " << tothitvecpair.first[s] << endl
                << " Tau: " << tauvecpair.first[s] << " + " << etauvecpairhi.first[s] << " - " << etauvecpairlow.first[s] << endl
                << " Sigma: " << sigmavecpair.first[s] << " + " << esigmavecpairhi.first[s] << " - " << esigmavecpairlow.first[s] << endl
                << " dQ0/dx: " << dqdxovecpair.first[s] << " + " << edqdxovecpairhi.first[s] << " - " << edqdxovecpairlow.first[s] << endl
                << " Likelihood value @ minimum: " << minvecpair.first[s] << endl
                << "  " << endl;
    }
  } // loop on selected tracks

  for(size_t s=0; s<tauvecpair.second.size(); s++)
  {

    avtau+=tauvecpair.second[s];
    hitau+=tauvecpair.second[s]+etauvecpairhi.second[s];
    lowtau+=tauvecpair.second[s]-etauvecpairlow.second[s];
    tothits+=tothitvecpair.second[s];

    if(fVerbose)
    {
      std::cout << " Second TPC:" << std::endl << " Number of hits passing selection: " << tothitvecpair.second[s] << endl
                << " Tau: " << tauvecpair.second[s] << " + " << etauvecpairhi.second[s] << " - " << etauvecpairlow.second[s] << endl
                << " Sigma: " << sigmavecpair.second[s] << " + " << esigmavecpairhi.second[s] << " - " << esigmavecpairlow.second[s] << endl
                << " dQ0/dx: " << dqdxovecpair.second[s] << " + " << edqdxovecpairhi.second[s] << " - " << edqdxovecpairlow.second[s] << endl
                << " Likelihood value @ minimum: " << minvecpair.second[s] << endl
                << "  " << endl;
    }
  } // loop on selected tracks

  avtau/=(tauvecpair.first.size()+tauvecpair.second.size());
  hitau/=(tauvecpair.first.size()+tauvecpair.second.size());
  lowtau/=(tauvecpair.first.size()+tauvecpair.second.size());

  std::cout << endl << endl
  << "***************************************************************" << endl
  << "***************************************************************" << endl
  << "******                SINGLE TRACK METHOD                ******" << endl
  << "***************************************************************" << endl
  << "***************************************************************" << endl
  << "******                     TOTAL                        *******" << endl
  << " Average value of tau over " <<  tauvecpair.first.size()+tauvecpair.second.size() << " tracks: " << endl
  << " Tau= " << avtau  << " + " << hitau-avtau << " - " << avtau - lowtau << " mus" << endl
  << tothits << " Hits" << endl
  << "***************************************************************" << endl
  << "***************************************************************" << endl;

} // End of endJob()

void sbnd::PuritySingle::ResetVars()
{

  run = -99999;
  subrun = -99999;
  event = -99999;
  ntracks_reco = -99999;
  nhits = -99999;

  for (int i = 0; i < 1000; ++i) 
  {
    for (int j = 0; j<3; ++j) trkpitch[i][j] = -99999;
  }

  for (int i = 0; i<20000; ++i)
  {
    hit_plane[i] = -99999;
    hit_wire[i] = -99999;
    hit_peakT[i] = -99999;
    hit_charge[i] = -99999;
    hit_trkkey[i] = -99999;
    hit_TPC[i] = -99999;
  }

} // End of ResetVars()

DEFINE_ART_MODULE(sbnd::PuritySingle)
