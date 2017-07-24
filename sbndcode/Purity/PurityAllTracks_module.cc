////////////////////////////////////////////////////////////////////////
// Class:       PurityAllTracks
// Module Type: analyzer
// File:        PurityAllTracks_module.cc
//
// Adapted from Roberto Acciarri's LArIAT code to use the 
// ArgoNeuT/ICARUS method by Tom Brooks
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
#include "lardataobj/RawData/RawDigit.h" //remove obj (ro)
#include "lardataobj/RawData/raw.h" // ro
#include "larsim/MCCheater/BackTracker.h"
#include "lardataobj/Simulation/SimChannel.h" // ro
#include "nusimdata/SimulationBase/MCTruth.h"
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
#include "TGaxis.h"
#include "TVirtualFitter.h"

// ####################
// ### c++ includes ###
// ####################
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <utility>
#include <sstream>

Double_t langaufun(Double_t *x, Double_t *par) {                                                                                                                                                          
  //Fit parameters:
  //par[0]=Width (scale) parameter of Landau density
  //par[1]=Most Probable (MP, location) parameter of Landau density
  //par[2]=Total area (integral -inf to inf, normalization constant)
  //par[3]=Width (sigma) of convoluted Gaussian function
  //
  //In the Landau distribution (represented by the CERNLIB approximation), 
  //the maximum is located at x=-0.22278298 with the location parameter=0.
  //This shift is corrected within this function, so that the actual
  //maximum is identical to the MP parameter.

  // Numeric constants
  Double_t invsq2pi = 0.3989422804014;   // (2 pi)^(-1/2)
  Double_t mpshift  = -0.22278298;       // Landau maximum location

  // Control constants - need to be tuned for each specific application
  Double_t np = 5000.0;      // number of convolution steps
  Double_t sc =   8.0;      // convolution extends to +-sc Gaussian sigmas

  // Variables
  Double_t xx;
  Double_t mpc;
  Double_t fland;
  Double_t sum = 0.0;
  Double_t xlow,xupp;
  Double_t step;
  Double_t i;
  // MP shift correction
  mpc = par[1] - mpshift * par[0]; 
  // Range of convolution integral
  xlow = x[0] - sc * par[3];
  xupp = x[0] + sc * par[3];
  step = (xupp-xlow) / np;

  if (par[0]==0) sum=0;
  else {
  // Convolution integral of Landau and Gaussian by sum
  for(i=1.0; i<=np/2; i++) {
    xx = xlow + (i-.5) * step;
    fland = TMath::Landau(xx,mpc,par[0]) / par[0];
    sum += fland * TMath::Gaus(x[0],xx,par[3]);

    xx = xupp - (i-.5) * step;
    fland = TMath::Landau(xx,mpc,par[0]) / par[0];
    sum += fland * TMath::Gaus(x[0],xx,par[3]);
  }
  }

  return (par[2] * step * sum * invsq2pi / par[3]);
}


TF1 *LGfitter(TH1 *h,  Double_t lbound, Double_t rbound, Double_t *fitparams, Double_t *fiterrors, Double_t *covmat, Int_t ief)
{

  // Fit the histogram h to a Landau function and pass back the parameters
  //    and their errors.  
  //  Use TMath::Landau, but correct for MPV offset
  //  Initializes the fit parameters to reasonable values
  //  Pass back the fit parameters and their errors
  //  Return the fit function

  //  Note: to get this to work correctly, you need to tune the 
  //    two constants "control parameters" in langaufun
  //    to match your application (sc to match the gaus width and 
  //    np to accomodate the histogram binning)!!

  //gStyle->SetOptFit(12);

  //  Fit histogram to Landau/Gaussian conv
  Char_t FunName[100]; sprintf(FunName,"Fitfcn_%s",h->GetName());
  TF1 *ffit = new TF1(FunName,langaufun,lbound,rbound,4);
  cout << "LW error" << fiterrors[0] << endl;
  ffit->SetParameters(fitparams[0],fitparams[1],fitparams[2],fitparams[3]);
  ffit->SetParError(0,fiterrors[0]);
  if (fiterrors[0]==0) ffit->FixParameter(0,fitparams[0]);
  ffit->SetParError(1,fiterrors[1]);
  ffit->SetParError(2,fiterrors[2]);
  ffit->SetParError(3,fiterrors[3]);
  ffit->SetParNames("Width","MP","Amp","Sigma");  
  // If the bins are large w.r.t. to the rising slope, you may 
  //     need to use the I option when fitting.  i.e. "IMLEVR"
  TFitResultPtr r;
  // removed M,E options as a test
  if (ief)   r = h->Fit(FunName,"LVRE");
  else  r = h->Fit(FunName,"LQRE");

  // Check fit status 
  //TString test =  gMinuit->fCstatu.Data();
  //Double_t a = ffit->GetParameter(0);
  fiterrors[0] = -1000.0;
  fiterrors[1] = -1000.0;
  fiterrors[2] = -1000.0;
  fiterrors[3] = -1000.0;
  fitparams[0] = -1000.0;
  fitparams[1] = -1000.0;
  fitparams[2] = -1000.0;
  fitparams[3] = -1000.0;
  covmat[0] = -1000.0;
  covmat[1] = -1000.0;
  covmat[2] = -1000.0;  
  covmat[3] = -1000.0;  
  //  if (ii<20) return(ffit);
  //cout << "here  " << test << endl;
  //if (test.BeginsWith("SUCC")) {   //successful fit 

  // Get Fit Parameters, their errors and cov matrix
  fitparams[0] = h->GetFunction(FunName)->GetParameter(0);
  fitparams[1] = h->GetFunction(FunName)->GetParameter(1);
  fitparams[2] = h->GetFunction(FunName)->GetParameter(2);
  fitparams[3] = h->GetFunction(FunName)->GetParameter(3);
  fiterrors[0] = h->GetFunction(FunName)->GetParError(0);
  fiterrors[1] = h->GetFunction(FunName)->GetParError(1);
  fiterrors[2] = h->GetFunction(FunName)->GetParError(2);
  fiterrors[3] = h->GetFunction(FunName)->GetParError(3);
  /*TVirtualFitter *fitter = TVirtualFitter::GetFitter();
  TMatrixD matrix(4,4, fitter->GetCovarianceMatrix());
  covmat[0] = fitter->GetCovarianceMatrixElement(0, 1);
  covmat[1] = fitter->GetCovarianceMatrixElement(0, 2);
  covmat[2] = fitter->GetCovarianceMatrixElement(1, 2);
  covmat[3] = fitter->GetCovarianceMatrixElement(0, 3);
  cout << "cov int " << covmat[3] << endl;*/
  //missing covariance terms here !
  //}

  return(ffit);

}

Double_t LandFun(Double_t *x, Double_t *par) {

  //Fit parameters:
  //par[0]=Width (scale) parameter of Landau density (sigma) 
  //par[1]=Most Probable (MP, location) parameter of Landau density
  //par[2]=Landau Amplitude
  //
  //In the Landau distribution (represented by the CERNLIB approximation), 
  //the maximum is located at x=-0.22278298 with the location parameter=0.
  //This shift is corrected within this function, so that the actual
   //maximum is identical to the MP parameter
  
  // Numeric constants
  Double_t mpshift  = -0.22278298;       // Landau maximum location


  Double_t mpc;
  mpc=par[1]-mpshift*par[0];

  Double_t temp;
  temp = par[2]*(TMath::Landau(x[0],mpc,par[0]));

  return(temp);
}

// Function to round up hit times to nearest multiple of time slices
int roundUp(int numToRound, int multiple)  
{  
 if(multiple == 0)  
 {  
  return numToRound;  
 }  

 int remainder = numToRound % multiple; 
 if (remainder == 0)
  {
    return numToRound; 
  }

 return numToRound + multiple - remainder; 
}

namespace sbnd
{
  class PurityAllTracks;
}

class sbnd::PurityAllTracks : public art::EDAnalyzer {
public:
  explicit PurityAllTracks(fhicl::ParameterSet const & p);
  virtual ~PurityAllTracks();

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

  double startt; // start time of track
  double endt; // end time of track
  double startw; // start wire of track
  double endw; // end wire of track
  int tothit; // total number of hits
  int fpretrig;             // number of time ticks of pre trigger
  double chisqr;     // chi squared per ndof for linear fit to wire vs time of track
  double fchargecut;        // cut on hitcharge
  size_t fhitcutf;          // pre lin fit cut on number of hits
  int    fhitcuts;          // post lin fit cut on number of hits
  double flinfitcut;        // tolerance for linear fit
  double fchisqcut;         // cut on chi squared per Ndof
  double fwirenumcut;       // cut on extent of track in wire number
  double ftimecut;          // cut on extent of track in time
  double fitvalue; // value from linear track fit
  double expo1; // exp fit parameter

  double fsampling;   // TPC fsampling rate for time ticks to microseconds conversion

  std::pair<std::vector<double>,std::vector<double>> tothitvecpair; // record total hits from each track
  std::vector<double> time; // hit times that pass linear fit of time vs wire 
  std::vector<double> wire; // hit wires that pass linear fit of time vs wire
  std::vector<double> charge; // hit charges that pass lin fit
  std::vector<double> ftime; // hit times that pass hit charge cut
  std::vector<double> fwire; // hit wires that pass hit charge cut
  std::vector<double> fcharge; // hit charge that pass hit charge cut

  std::vector<std::pair<double,double>> timechargevec_ftpc; // Vector of time and charge of hits that pass all selection cuts for first tpc
  std::vector<std::pair<double,double>> timechargevec_stpc; // Vector of time and charge of hits that pass all selection cuts for second tpc

  TH1D *hChiSq; // all chisq
  TH1D *hChiSqSelected; // selected chisq
  TH1D *hGrad; // all gradient
  TH1D *hGradSelected; // selected gradient
  TH1D *hCharge; // hit charge/track pitch

  int fslices;
  int fNevents;

  const double scale = ((6.24150975*10e18)/10e15)* (1/39.3);//14.51; //Convert from ADC to fC or electrons

  std::string fHitsModuleLabel;
  std::string fTrackModuleLabel;
  std::string fClusterModuleLabel;

}; // End of class PurityAllTracks

sbnd::PurityAllTracks::PurityAllTracks(fhicl::ParameterSet const & pset) : EDAnalyzer(pset)
{
  this->reconfigure(pset);
} // End of constructor

sbnd::PurityAllTracks::~PurityAllTracks()
{
  // No dynamic memory to clean up
} //End of Destructor

void sbnd::PurityAllTracks::reconfigure(fhicl::ParameterSet const & pset)
{
  // Set some parameters from the FHiCL file
  fHitsModuleLabel             = pset.get< std::string          >("HitsModuleLabel");
  fTrackModuleLabel            = pset.get< std::string          >("TrackModuleLabel");

  fchargecut                   = pset.get< double               >("ChargeCut");
  fhitcutf                     = pset.get< size_t               >("FirstHitCut");
  fhitcuts                     = pset.get< int                  >("SecondHitCut");
  flinfitcut                   = pset.get< double               >("LinearFitCut");
  fchisqcut                    = pset.get< double               >("ChiSquaredCut");
  fwirenumcut                  = pset.get< double               >("WireNumberCut");
  ftimecut                     = pset.get< double               >("TimeCut");
  fsampling                    = pset.get< double               >("SamplingTime");
  fpretrig                     = pset.get< int                  >("PreSampling");
  fVerbose                     = pset.get< bool                 >("Verbose", false);
  fslices                      = pset.get< int                  >("TimeSlices");

  fNevents                     = pset.get< int                  >("NumberEvents");

  return;
}

void sbnd::PurityAllTracks::analyze(art::Event const & evt)
{
  // Reset important variables like run number, etc
  ResetVars();

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

  int isUsed = rand() % 10 + 1;

  for(size_t i=0; i<tracklist.size();++i)
  {
    // ###########################################
    // ### Calculating the pitch for the track ###
    // ###########################################

    // === Looping over our two planes (0&1 == Induction, 2 == Collection) ===
    for (int j = 0; j<3; ++j)
    {
      // ### Putting in a protective try in case we can't set the pitch ###
      try
      {
        size_t traj_point = 0;
        // ### If we are in the induction plane calculate the tracks pitch in that view ###
        if (j==0) trkpitch[i][j] = lar::util::TrackPitchInView(*tracklist[i], geo::kU, traj_point);
        // ### If we are in the collection plane calculate the tracks pitch in that view ###
        else if (j==1) trkpitch[i][j] = lar::util::TrackPitchInView(*(tracklist[i]), geo::kV, traj_point);
        else if (j==2) trkpitch[i][j] = lar::util::TrackPitchInView(*(tracklist[i]), geo::kW, traj_point);
      }//<---End Try statement
      catch( cet::exception &e)
      {
        mf::LogWarning("PurityOnline")<<"caught exeption "<<e<<"\n setting pitch to 0";
        trkpitch[i][j] = 0;
      }//<---End catch statement
    }// <---End looping over planes (j)
  }// <---End looping over tracks   

  nhits = hitlist.size();
  if(fVerbose) std::cout << "Total number of hits: " << nhits << std::endl;

  for (size_t i = 0; i<hitlist.size() && int(i)< 20000; ++i)
  {
    geo::WireID wireid = hitlist[i]->WireID();
    hit_plane[i]     = wireid.Plane;
    hit_wire[i]      = wireid.Wire;
    hit_TPC[i]       = wireid.TPC;
    hit_peakT[i]     = hitlist[i]->PeakTime();

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
      hit_charge[i] = raw_charge/(rms*TMath::Sqrt(2*M_PI));
    }

    if (fmtk.isValid())
    {
      if (fmtk.at(i).size()!=0) hit_trkkey[i] = fmtk.at(i)[0].key(); 
    }

  }

  //LOOP OVER TRACKS FROM HERE
  for (int trkn = 0; trkn<ntracks_reco; ++trkn){

  if (isUsed > fNevents) continue;

  double azimuth = tracklist[trkn]->AzimuthAngle();

  for (int tpcnum = 0; tpcnum<2; ++tpcnum){

    // ----------------------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------------------------------------------------
    //                                                        PURITY CODE
    // ----------------------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------------------------------------------------
    ftime.clear();
    fwire.clear();
    fcharge.clear();
    chisqr=0.0;
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
        hCharge->Fill((hit_charge[ij]/trkpitch[trkn][2])*scale);
        ftime.push_back((hit_peakT[ij]-fpretrig)*fsampling);
        fwire.push_back(hit_wire[ij]);
        fcharge.push_back((hit_charge[ij]/trkpitch[trkn][2])*scale);
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
    
    if(fVerbose) std::cout<<"Track: "<<trkn<<" (TPC:"<<tpcnum<<") time = "<<endt-startt<<" wire = "<<endw-startw<<" Hits = "<<ftime.size()<<std::endl
                          <<"Pitch = "<<trkpitch[trkn][2]<<" Azimuth = "<<azimuth<<std::endl;
/*
    // Make cut on azimuthal angle to deal with angular dependence of result
    if ((azimuth>0.87&&azimuth<2.27)||(azimuth>4.01&&azimuth<5.41))
    {
      continue;
    }
*/
    // Make cut on time extent of track
    if (endt - startt < ftimecut)
    {
      if(fVerbose) std::cout << "Not a through going muon, skipping..." << std::endl;
      continue;
    }

    // Make cut on wire number extent of track
    if (endw - startw < fwirenumcut)
    {
      if(fVerbose) std::cout << "Not enough hit wires, skipping..." << std::endl;
      continue;
    }

    // Make cut on number of hits
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

        if(TMath::Abs((fwire[i]-fitvalue)/fwire[i])<flinfitcut) // changed from 0.02
        {
          tothit++;
          time.push_back(ftime[i]);
          wire.push_back(fwire[i]);
          charge.push_back(fcharge[i]);
        }
      }  //select only hits close to the fit
      
      TGraph *wgr2 = new TGraph(time.size(),&time[0],&wire[0]);
      wgr2->Fit("wfun","R");
    } 

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
    if (tothit>=fhitcuts && chisqr<fchisqcut && chisqr!=0.0/*&& (angle>0.25 ||angle<-0.25)*/)
    {  
      if(fVerbose) std::cout << "Track passed selection!" << std::endl;
      if (tpcno == 0) tothitvecpair.first.push_back(tothit);
      else if (tpcno == 1) tothitvecpair.second.push_back(tothit);
      hGradSelected->Fill(wfun->GetParameter(1)); 
      hChiSqSelected->Fill(chisqr);         

      // Make a fit of charge VS time to find charge at time=0, i.e., dqdxo 
      TGraph *gr = new TGraph(time.size(),&time[0],&charge[0]);
      TF1 *fun = new TF1("fun","expo",0,400);
      gr->Fit("fun");

      expo1=fun->GetParameter(1);

      ////////////////////////////////////////////////////////////////////////////////
      ///  Now I have the cut, move on writing the file for the minimization       ///
      ////////////////////////////////////////////////////////////////////////////////

      if(expo1 < 0.0)  // I don't want an exponential fit of the charge going upward (i.e. more charge at the end of drift time rather than bottom)
      {
        
        for (size_t i = 0; i<charge.size(); i++)
        {
          if(time[i]>12.5&&time[i]<1262.5)
          {
            if (tpcno == 0) timechargevec_ftpc.push_back(std::make_pair(time[i]-12.5,charge[i]));
            else if (tpcno == 1) timechargevec_stpc.push_back(std::make_pair(time[i]-12.5,charge[i]));
          }
        }      
      }

    } // End Event selection 

  } // End of looping over TPCs
  }// END OF LOOPING OVER TRACKS

} // End of analyze()

void sbnd::PurityAllTracks::beginJob()
{
  art::ServiceHandle<art::TFileService> tfs;

  hChiSq = tfs->make<TH1D>("hChiSq", "Chi Square", 100, 0, 10); //Check these limits are appropriate
  hChiSqSelected = tfs->make<TH1D>("hChiSqSelected", "Chi Square", 100, 0, 10);
  hGrad = tfs->make<TH1D>("hGrad", "Track Angle", 100, -3, 3);
  hGradSelected = tfs->make<TH1D>("hGradSelected", "Track Angle", 100, -3, 3);
  hCharge = tfs->make<TH1D>("hCharge", "Hit Charge", 600, 0, 700*scale);  // changed title from Hit Charge. lower lim from 0 and upper lim from 6000

  hChiSqSelected->SetLineColor(2);
  hGradSelected->SetLineColor(2);

  tothitvecpair.first.clear();
  tothitvecpair.second.clear();
  timechargevec_ftpc.clear();
  timechargevec_stpc.clear();

  TFile *fnew = new TFile("convolutionFits.root","RECREATE");
  fnew->Close();

} // End of beginJob()

void sbnd::PurityAllTracks::endJob()
{

  if(fVerbose)
  {
    std::cout << endl 
         << endl
         << "////////////////////////////////////////////////////////////" << endl
         << "/////                  SUMMARY                         /////" << endl
         << "////////////////////////////////////////////////////////////" << endl
         << "# Hits selected in first TPC: " << timechargevec_ftpc.size() << " from " << tothitvecpair.first.size() << " tracks" << endl
         << "# Hits selected in second TPC: " << timechargevec_stpc.size() << " from " << tothitvecpair.second.size() << " tracks" << endl
         << endl 
         << endl;
  }

  // Break the scatter plot into time slices 

  // Vectors of histograms
  std::vector<TH1D*> hTimeSlicesAll;
  std::vector<std::string> histnamesall;

  std::vector<TH1D*> hTimes;
  std::vector<std::string> thistnames;

  TH2D *hAmpvTime = new TH2D("hAmpvTime","hAmpvTime",100,0,1300,100,0,350*scale);

  // Loop over the number of dirrerent time slices
  for (int nslices=0; nslices<fslices; ++nslices)
  {
    // Give each histogram an individual name
    std::ostringstream histnameall;
    std::ostringstream thistname;
    histnameall << "hTimeSlice" << nslices;
    thistname << "hTimes" << nslices;
    histnamesall.push_back(histnameall.str());
    thistnames.push_back(thistname.str());
    // Create histograms and push on to vectors
    TH1D *hTimeSlice = new TH1D(histnameall.str().c_str(),histnameall.str().c_str(),200,0,350*scale);
    TH1D *hTime = new TH1D(thistname.str().c_str(),thistname.str().c_str(),130,0,1300);
    hTimeSlicesAll.push_back(hTimeSlice);
    hTimes.push_back(hTime);
  }

  // Vectors of times and charges for putting in to graphs
  std::vector<double> time_ftpc;
  std::vector<double> charge_ftpc;
  std::vector<double> time_stpc;
  std::vector<double> charge_stpc;

  // Plot the charge vs drift time
  TFile *f1 = new TFile("AmpVsTime.root","RECREATE");
  // Loop over the selected hits from the first TPC ignoring the bottom 1% and top 30%
  for (size_t entry1=0; entry1<timechargevec_ftpc.size(); ++entry1)
  {
    // Fill vectors
    time_ftpc.push_back(timechargevec_ftpc[entry1].first);
    charge_ftpc.push_back(timechargevec_ftpc[entry1].second);
    hAmpvTime->Fill(timechargevec_ftpc[entry1].first,timechargevec_ftpc[entry1].second);
    // Round the time up to the nearest upper time slice limit
    int rounded1 = roundUp(ceil(timechargevec_ftpc[entry1].first),ceil(1250.0/fslices));//-12.5),ceil(1262.5/fslices));
    // Fill the histogram which corresponds to this time slice
    int histno1 = (rounded1/(ceil(1250.0/fslices))) - 1;
    hTimeSlicesAll[histno1]->Fill(timechargevec_ftpc[entry1].second);
    hTimes[histno1]->Fill(timechargevec_ftpc[entry1].first);
  }

  // Loop over all the selected hits from the second TPC
  for (size_t entry2=0; entry2<timechargevec_stpc.size(); ++entry2)
  {
    // Fill vectors
    time_stpc.push_back(timechargevec_stpc[entry2].first);
    charge_stpc.push_back(timechargevec_stpc[entry2].second);
    hAmpvTime->Fill(timechargevec_stpc[entry2].first,timechargevec_stpc[entry2].second);
    // Round the time up to the nearest upper time slice limit
    int rounded2 = roundUp(ceil(timechargevec_stpc[entry2].first),ceil(1250.0/fslices));
    // Fill the histogram which corresponds to this time slice
    int histno2 = (rounded2/(ceil(1250.0/fslices))) - 1;
    hTimeSlicesAll[histno2]->Fill(timechargevec_stpc[entry2].second);
    hTimes[histno2]->Fill(timechargevec_stpc[entry2].first);
  }

  // Check the hit finding efficiency for each time slice
  std::vector<double> vNHits;
  std::vector<double> vSlice;
  double maxHits = 0;
  for (size_t x=0; x<hTimeSlicesAll.size(); ++x)
  {
    vNHits.push_back(hTimeSlicesAll[x]->GetEntries());
    vSlice.push_back(x);
    if(hTimeSlicesAll[x]->GetEntries()>maxHits) maxHits = hTimeSlicesAll[x]->GetEntries();
  }
  TGraph *gEff = new TGraph(vNHits.size(), &(vSlice[0]), &(vNHits[0]));
  gEff->GetXaxis()->SetTitle("Slice");
  gEff->GetYaxis()->SetTitle("Number of hits");
  gEff->Write("gEff");

  // Plot charge vs time
  hAmpvTime->Write("hAmpvTime");

  f1->Close();

  std::vector<double> mpcvec;
  std::vector<double> errvec;

  TFile *tmpfile3 = new TFile("GaussFits.root","RECREATE");
  tmpfile3->Close();

  std::vector<double> timeslice;
  std::vector<double> etimeslice;

  // Fit each histogram with a convoluted Landau and Gaussian and get MPC
  for (int nfits=0; nfits<fslices; ++nfits)
  {
    try{
    TFile *tmpfile3 = new TFile("GaussFits.root","UPDATE");
    // starting parameters for the fit - TUNE ME -
    Double_t bfp[4];
    Double_t bfpe[4];
    Double_t bcov[4];        
    Int_t ii = hTimeSlicesAll[nfits]->GetEntries();  
    Double_t mm = hTimeSlicesAll[nfits]->GetBinCenter(hTimeSlicesAll[nfits]->GetMaximumBin());
    Double_t amp = ii*mm/100.0;
    Double_t rms = hTimeSlicesAll[nfits]->GetRMS();
    cout << " rms = " << rms << endl;
    Double_t mw = mm/100.0;
    // starting parameter values
    bfp[0]=mw*10.0; bfp[1]=mm; bfp[2]=amp; bfp[3]=mw*5.0;
    // step size for fitter
    bfpe[0]=0.5*mw; bfpe[1]=0.08*mm; bfpe[2]=0.1*amp; bfpe[3]=0.5*mw;        
         
    Double_t lb=0.0;
    Double_t ub=0.0;
    int ibpeak = hTimeSlicesAll[nfits]->GetMaximumBin();
    int iph = hTimeSlicesAll[nfits]->GetBinContent(ibpeak);
    Double_t low = 0.3*iph;
    Double_t high = 0.1*iph;
    for (int ib=ibpeak;ib>0;ib--) {
      int itest = hTimeSlicesAll[nfits]->GetBinContent(ib);
      if (itest<low) {lb=hTimeSlicesAll[nfits]->GetBinCenter(ib); break;}
    }   
    for (int ib=ibpeak;ib<2*ibpeak;ib++) {
      int itest = hTimeSlicesAll[nfits]->GetBinContent(ib);
      if (itest<high) {ub=hTimeSlicesAll[nfits]->GetBinCenter(ib); break;}
    }
    //  Fit to Landau-Gaussian convolution
    TF1 *fitres = LGfitter(hTimeSlicesAll[nfits],lb,ub,bfp,bfpe,bcov,0);
    fitres->Draw();
    hTimeSlicesAll[nfits]->Write(histnamesall[nfits].c_str());
    if(bfpe[1]>5/*&&vNHits[nfits]>(maxHits-0.1*maxHits)*/){
      mpcvec.push_back(bfp[1]);
      errvec.push_back(bfpe[1]);
      timeslice.push_back(hTimes[nfits]->GetMean());
      etimeslice.push_back(0);
    }
    tmpfile3->Close();
    }catch(...){}
  }

  // Plot MPC vs drift time and fit with an exponential to get electron lifetime
  TFile *f2 = new TFile("AmpVsTime.root","UPDATE");

  TGraph *gMPCvTime = new TGraphErrors(mpcvec.size(),&timeslice[0],&mpcvec[0],&etimeslice[0],&errvec[0]);
  gMPCvTime->GetYaxis()->SetTitle("MP dQ/dx (e/mm)");
  gMPCvTime->GetXaxis()->SetTitle("Time (#mus)");
  gMPCvTime->Fit("expo","E");
  gMPCvTime->Write("MPCvTime");

  f2->Close();

  TF1 *fitres = gMPCvTime->GetFunction("expo");
  double tau = -1/(fitres->GetParameter(1)-1.413e-5);
  double taue = tau*tau*TMath::Sqrt(fitres->GetParError(1)*fitres->GetParError(1)+2.99e-6*2.99e-6);

  std::cout << endl << endl
  << "***************************************************************" << endl
  << "***************************************************************" << endl
  << "******                  ALL TRACK METHOD                 ******" << endl
  << "***************************************************************" << endl
  << "***************************************************************" << endl
  << "******                        TOTAL                      ******" << endl
  << " Average value of tau over " <<  tothitvecpair.first.size() + tothitvecpair.second.size() << " tracks: " << endl
  << " Tau= " << tau  << " +/- " << taue << " mus" << endl
  << "***************************************************************" << endl
  << "***************************************************************" << endl;

} // End of endJob()

void sbnd::PurityAllTracks::ResetVars()
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

DEFINE_ART_MODULE(sbnd::PurityAllTracks)
