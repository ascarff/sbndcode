////////////////////////////////////////////////////////////////////////
// Class:       PurityParallel
// Module Type: analyzer
// File:        PurityParallel_module.cc
//
// Modified from Roberto Acciarri's LArIAT code by Tom Brooks
// Uses through going muon tracks that are parallel to the wire plane to calculate electron lifetime
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
#include "TLegend.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TVirtualFitter.h"

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <utility>

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
  TVirtualFitter *fitter = TVirtualFitter::GetFitter();
  TMatrixD matrix(4,4, fitter->GetCovarianceMatrix());
  covmat[0] = fitter->GetCovarianceMatrixElement(0, 1);
  covmat[1] = fitter->GetCovarianceMatrixElement(0, 2);
  covmat[2] = fitter->GetCovarianceMatrixElement(1, 2);
  covmat[3] = fitter->GetCovarianceMatrixElement(0, 3);
  cout << "cov int " << covmat[3] << endl;
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

namespace sbnd
{
  class PurityParallel;
}

class sbnd::PurityParallel : public art::EDAnalyzer {
public:
  explicit PurityParallel(fhicl::ParameterSet const & p);
  virtual ~PurityParallel();

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

  // Declare all Data members
  int run; // run number
  int subrun; // subrun number
  int event; // event number
  int prevevent; // previous event number
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

  int tothit; // total number of hits
  int fpretrig;             // number of time ticks of pre trigger
  double chisqr;     // chi squared per ndof for linear fit to wire vs time of track
  double fchargecut;        // cut on hitcharge
  size_t fhitcutf;          // first cut on number of hits
  int fhitcuts;             // second cut on the number of hits 
  double flinfitcut;        // tolerance for recursive linear fit
  double fchisqcut;         // cut on the chi squares per ndof
  double fitvalue; // value from linear track fit
  double tau; // Electron lifetime
  double fsampling;   // TPC fsampling rate for time ticks to microseconds conversion

  std::pair<std::vector<double>,std::vector<double>> tothitvecpair; // record total hits from each track
  std::vector<double> time; // hit times that pass linear fit of time vs wire 
  std::vector<double> wire; // hit wires that pass linear fit of time vs wire
  std::vector<double> charge; // hit charges that pass lin fit
  std::vector<double> ftime; // hit times that pass hit charge cut
  std::vector<double> fwire; // hit wires that pass hit charge cut
  std::vector<double> fcharge; // hit charge that pass hit charge cut

  TH1D *hCharge; // hit charge/track pitch
  TH1D *hTimes; // hit times for all CRT pairs
  TH1D *hCharges; // hit charges for all CRT pairs

  std::vector<TH1D*> hTimesVec; // vector of hists of hit times for the first TPC
  std::vector<TH1D*> hChargesVec; // vector of hists of hit charges for the first TPC
  std::vector<TH1D*> hCutChargesVec; // vector of hists of hit chagres with Landau cut

  std::vector<std::string> thistnames;
  std::vector<std::string> chistnames;

  std::string fHitsModuleLabel;
  std::string fTrackModuleLabel;

  int filenum;

  const double scale = ((6.24150975*10e18)/10e15)* (1/39.3);//14.51;

  TCanvas *c1 = new TCanvas("c1","c1",1000,700);

  int fNevents;

}; // End of class Purity

sbnd::PurityParallel::PurityParallel(fhicl::ParameterSet const & pset) : EDAnalyzer(pset)
{
  this->reconfigure(pset);
} // End of constructor

sbnd::PurityParallel::~PurityParallel()
{
  
} //End of Destructor

void sbnd::PurityParallel::reconfigure(fhicl::ParameterSet const & pset)
{
  // Set some parameters from the FHiCL file
  fHitsModuleLabel             = pset.get< std::string >("HitsModuleLabel");
  fTrackModuleLabel            = pset.get< std::string >("TrackModuleLabel");

  fchargecut                   = pset.get< double               >("ChargeCut");
  fhitcutf                     = pset.get< size_t               >("FirstHitCut");
  fhitcuts                     = pset.get< int                  >("SecondHitCut");
  flinfitcut                   = pset.get< double               >("LinearFitCut");
  fchisqcut                    = pset.get< double               >("ChiSquaredCut");
  fsampling                    = pset.get< double               >("SamplingTime");
  fpretrig                     = pset.get< int                  >("PreSampling");
  fVerbose                     = pset.get< bool                 >("Verbose", false);

  fNevents                     = pset.get< int                  >("NumberEvents");

  return;
}

void sbnd::PurityParallel::analyze(art::Event const & evt)
{
  // Reset important variables like run number, etc
  ResetVars();

  // Get run number
  run = evt.run();
  // Get sub-run number
  subrun = evt.subRun();
  // Get event number
  event = evt.id().event();
  
  if (event == 1 && prevevent == 10){
    filenum += 1;
  }
  prevevent = event;

  // There are 9 different files for each TPC with parallel muons at 5, 30, 55, 80, 105, 130, 155, 180 and 195 cm
  // Many of the muons at 5cm pass into the wire plane so we will ignore this sample
  // There is a separate file for each TPC, TPC 2 80cm reco is missing but the first stage is there
  // Create array of distances and array of TPCs and use the fact that subrun goes from 20 to 1 between each file to change info

  std::cout << std::endl
            << "=========================================" << std::endl
            << "File = " << filenum << ", Run = " << run << ", SubRun = " << subrun << ", Evt = " << event << std::endl
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
    if(fVerbose) std::cout << "Track: " << i << std::endl;

    // ###########################################
    // ### Calculating the pitch for the track ###
    // ###########################################

    // === Looping over our three planes (0&1 == Induction, 2 == Collection) ===
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
      hit_charge[i] = raw_charge/(rms*TMath::Sqrt(2*M_PI));
    }

    if (fmtk.isValid())
    {
      if (fmtk.at(i).size()!=0) hit_trkkey[i] = fmtk.at(i)[0].key(); 
    }

  }

int isUsed = rand() % 10 + 1;

  //LOOP OVER TRACKS FROM HERE
  for (int trkn = 0; trkn<ntracks_reco; ++trkn){
  if (isUsed > fNevents) continue;
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

    for (int ij = 0; ij<nhits; ++ij)              // Loop over hits
    { 
      if (hit_plane[ij]==2&&hit_trkkey[ij]==trkn&&(hit_charge[ij]/trkpitch[trkn][2])<fchargecut)   // Selecting collection plane, first track of the plane
      {
        if (tpcno == -1) tpcno = hit_TPC[ij];
        hCharge->Fill((hit_charge[ij]/trkpitch[trkn][2])*scale);
        ftime.push_back((hit_peakT[ij]-fpretrig)*fsampling);
        fwire.push_back(hit_wire[ij]);
        fcharge.push_back((hit_charge[ij]/trkpitch[trkn][2]*scale));
        tothit += 1;
      }  // end selection collection plane and long tracks
    }     // loop over hits  

    double startw = 0.0;
    double endw = 0.0;
    double startt = 0.0;
    double endt = 0.0;

    // Calculate extent of track in wire number and time
    if(!ftime.empty()) {
      auto wresult = std::minmax_element(fwire.begin(), fwire.end());
      startw = fwire[(wresult.first-fwire.begin())];
      endw = fwire[(wresult.second-fwire.begin())];
      auto result = std::minmax_element(ftime.begin(), ftime.end());
      startt = ftime[(result.first-ftime.begin())];
      endt = ftime[(result.second-ftime.begin())];
    }

    // Make cut on time extent of track
    if (endt - startt > 63)
    {
      if(fVerbose) std::cout << "Not contained in a CRT pair, skipping..." << std::endl;
      continue;
    }

    // Make cut on wire number extent of track
    if (endw - startw < 500)
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

    ////////////////////////////////////////////////////////////////////////////////
    ///              Here I select the events for the minimization               ///
    ////////////////////////////////////////////////////////////////////////////////

    if (tothit>=fhitcuts) // Tracks must have a certain number of hits
    {  
      if(fVerbose) std::cout << "Track passed selection!" << std::endl;
      if (tpcno == 0) tothitvecpair.first.push_back(tothit);
      else if (tpcno == 1) tothitvecpair.second.push_back(tothit);

      // If the muons have not been generated at the same time, need to get t0 from CRT matching
      // Get the corresponding pair of CRTs that the muon has passed through

      std::sort(fcharge.begin(),fcharge.end());

      for (size_t hitn=0; hitn<fcharge.size(); ++hitn)
      {
        hTimesVec[filenum]->Fill(ftime[hitn]);
        hChargesVec[filenum]->Fill(fcharge[hitn]);
        // Fill histogram of hit times corresponding to the CRT pair
        hTimes->Fill(ftime[hitn]);
        // Fill histogram of hit charge corresponding to the CRT pair
        hCharges->Fill(fcharge[hitn]);
      }

    } // End Event selection 

  }// END OF LOOPING OVER TRACKS

} // End of analyze()

void sbnd::PurityParallel::beginJob()
{

  art::ServiceHandle<art::TFileService> tfs;

  hCharge = tfs->make<TH1D>("hCharge", "Hit Charge", 300, 0, 700*scale);  
  hTimes = tfs->make<TH1D>("hTimes", "Selected Hit Times", 130, 0, 1300);
  hCharges = tfs->make<TH1D>("hCharges", "Selected Hit Charges", 300, 0, 700*scale);
  filenum = 0;

  thistnames.clear();
  chistnames.clear();
  hTimesVec.clear();
  hChargesVec.clear();
  hCutChargesVec.clear();

  for (int nhists1 = 0; nhists1<18; ++nhists1){
    std::ostringstream timehistname1;
    std::ostringstream chargehistname1;
    timehistname1 << "hTimesDist=" << -185 + nhists1*10;
    chargehistname1 << "hChargeDist=" << -185 + nhists1*10;
    thistnames.push_back(timehistname1.str());
    chistnames.push_back(chargehistname1.str());
    TH1D *hTimesTemp = new TH1D(timehistname1.str().c_str(),timehistname1.str().c_str(),130,0,1300);
    TH1D *hChargesTemp = new TH1D(chargehistname1.str().c_str(),chargehistname1.str().c_str(),200,0,350*scale);
    hTimesVec.push_back(hTimesTemp);
    hChargesVec.push_back(hChargesTemp);
  }


  TFile *fitFile = new TFile("convolutionFits.root","RECREATE");
  fitFile->Close();

  tothitvecpair.first.clear(); tothitvecpair.second.clear();

} // End of beginJob()

void sbnd::PurityParallel::endJob()
{

  if(fVerbose)
  {
    std::cout << endl 
         << endl
         << "////////////////////////////////////////////////////////////" << endl
         << "/////                  SUMMARY                         /////" << endl
         << "////////////////////////////////////////////////////////////" << endl
         << "# Tracks selected in first TPC: " << tothitvecpair.first.size() << endl
         << "# Tracks selected in second TPC: " << tothitvecpair.second.size() << endl
         << endl 
         << endl;
  }

  double ftpchits = 0;
  for (size_t i = 0; i < 18; i++){
    ftpchits += hChargesVec[i]->GetEntries();
  }
  std::cout<<"Total number of hits = "<<ftpchits<<endl;

  std::vector<double> timevec1;
  std::vector<double> etimevec1;
  std::vector<double> mpcvec1;
  std::vector<double> empcvec1;

  TFile *tmpfile3 = new TFile("Histograms.root","RECREATE");

  for(size_t nthists1=0; nthists1<18; ++nthists1){
    if(hTimesVec[nthists1]->GetEntries() < 10) continue;
    hTimesVec[nthists1]->SetTitle("Hit Times; Time (#mus); Hits (/10#mus)");
    hTimesVec[nthists1]->GetYaxis()->SetTitleOffset(1.5);
    hTimesVec[nthists1]->SetLineWidth(2.0);

    hTimesVec[nthists1]->Write(thistnames[nthists1].c_str());
    timevec1.push_back(hTimesVec[nthists1]->GetMean());
    etimevec1.push_back(0);
  }

  for(size_t nchists1=0; nchists1<18; ++nchists1){
    if(hChargesVec[nchists1]->GetEntries() < 10) continue;
    hChargesVec[nchists1]->SetTitle("Hit Charges; dQ/dx (e/mm); Entries/10");
    hChargesVec[nchists1]->GetYaxis()->SetTitleOffset(1.5);
    hChargesVec[nchists1]->SetLineWidth(2.0);

    // starting parameters for the fit - TUNE ME -
    Double_t bfp[4];
    Double_t bfpe[4];
    Double_t bcov[4];        
    Int_t ii = hChargesVec[nchists1]->GetEntries();                                                                                                                                                 
    Double_t mm = hChargesVec[nchists1]->GetBinCenter(hChargesVec[nchists1]->GetMaximumBin());
    Double_t amp = ii*mm/100.0;
    Double_t rms = hChargesVec[nchists1]->GetRMS();
    cout << " rms = " << rms << endl;
    Double_t mw = mm/100.0;
    // starting parameter values
    bfp[0]=mw*10.0; bfp[1]=mm; bfp[2]=amp; bfp[3]=mw*5.0;
    // step size for fitter
    bfpe[0]=0.5*mw; bfpe[1]=0.08*mm; bfpe[2]=0.1*amp; bfpe[3]=0.5*mw;        
         
    Double_t lb=0.0;
    Double_t ub=0.0;
    int ibpeak = hChargesVec[nchists1]->GetMaximumBin();
    int iph = hChargesVec[nchists1]->GetBinContent(ibpeak);
    Double_t low = 0.01*iph;
    Double_t high = 0.1*iph;
    for (int ib=ibpeak;ib>0;ib--) {
      int itest = hChargesVec[nchists1]->GetBinContent(ib);
      if (itest<low) {lb=hChargesVec[nchists1]->GetBinCenter(ib); break;}
    }   
    for (int ib=ibpeak;ib<2*ibpeak;ib++) {
      int itest = hChargesVec[nchists1]->GetBinContent(ib);
      if (itest<high) {ub=hChargesVec[nchists1]->GetBinCenter(ib); break;}
    } 
    //  Fit to Landau-Gaussian convolution
    TF1 *fitres = LGfitter(hChargesVec[nchists1],lb,ub,bfp,bfpe,bcov,0);
    fitres->Draw();

    hChargesVec[nchists1]->Write(chistnames[nchists1].c_str());
    if(bfpe[1]>5){
      mpcvec1.push_back(bfp[1]);
      empcvec1.push_back(bfpe[1]);
    }
  }

  // Plot the mean value from the LGC  against the mean hit time
  // Fit an exponential to extract the lifetime
  TGraphErrors *gMPCvTime1 = new TGraphErrors(mpcvec1.size(),&timevec1[0],&mpcvec1[0],&etimevec1[0],&empcvec1[0]);
  TF1 *fexp1 = new TF1("fexp1","expo",0,1200);
  gMPCvTime1->Fit("fexp1","E");
  gMPCvTime1->Write("gMPCvTime1");
  double tau1;
  double etau1;
  tau1 = -1/(fexp1->GetParameter(1)-1.413e-5);
  etau1 = tau1*tau1*TMath::Sqrt(fexp1->GetParError(1)*fexp1->GetParError(1)+2.99e-6*2.99e-6);

  tmpfile3->Close();

  std::cout << endl << endl
  << "***************************************************************" << endl
  << "***************************************************************" << endl
  << "******              PARALLEL TRACKS METHOD               ******" << endl
  << "***************************************************************" << endl
  << "******                       TOTAL                       ******" << endl
  << " Tau= " << tau1  << " +/- " << etau1 << " mus" << endl
  << "***************************************************************" << endl
  << "***************************************************************" << endl;

} // End of endJob()

void sbnd::PurityParallel::ResetVars()
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

DEFINE_ART_MODULE(sbnd::PurityParallel)
