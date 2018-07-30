////////////////////////////////////////////////////////////////////////
// Class:       CRTRecoAna
// Module Type: analyzer
// File:        CRTRecoAna_module.cc
//
// Analysis module for evaluating CRT reconstruction on through going
// muons.
//
// Tom Brooks (tbrooks@fnal.gov)
////////////////////////////////////////////////////////////////////////

// sbndcode includes
#include "sbndcode/RecoUtils/RecoUtils.h"
#include "sbndcode/CRT/CRTProducts/CRTData.hh"

// LArSoft includes
#include "lardataobj/Simulation/SimChannel.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "larcore/Geometry/Geometry.h"
#include "larcore/Geometry/AuxDetGeometry.h"
#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larsim/Simulation/LArG4Parameters.h"

// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Utilities/Exception.h"
#include "larsim/MCCheater/BackTrackerService.h"


// Utility libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Atom.h"
#include "cetlib/pow.h" // cet::sum_of_squares()

// ROOT includes. Note: To look up the properties of the ROOT classes,
// use the ROOT web site; e.g.,
// <https://root.cern.ch/doc/master/annotated.html>
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector3.h"
#include "Math/Minimizer.h"
#include "Math/Functor.h"
#include "Math/Factory.h"
#include "TPolyMarker3D.h"
#include "TPolyLine3D.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TGraphErrors.h"
#include "TLine.h"
#include "TMarker.h"
#include "TGraphAsymmErrors.h"
#include "TLegend.h"
#include "TBox.h"
#include "TPad.h"
#include "TString.h"
#include "TGeoManager.h"

// C++ includes
#include <map>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

namespace {
  // Local namespace for local functions
  // Declare here, define later

  int MostCommonID(std::vector<int> ids);

}

namespace sbnd {

  struct CRTStrip {
    double t0; 
    uint32_t channel; 
    double x; 
    double ex; 
    int id1; 
    int id2;
    std::pair<std::string,unsigned> tagger;
  };

  struct CRTHit {
    std::string tagger;
    double xpos;
    double ypos;
    double zpos;
    double xerr;
    double yerr;
    double zerr;
    double ts0_s;
    double ts0_ns;
    double ts1_ns;
    std::vector<int> ids;
  }; // CRTHit

  struct CRTavehit{
    uint32_t ts0_ns;
    uint16_t ts0_ns_err;
    int32_t ts1_ns; 
    uint16_t ts1_ns_err;                                                        
    
    float x_pos;
    float x_err;
    float y_pos;
    float y_err;
    float z_pos;
    float z_err;
    float pe;
    std::string tagger;
    int id;
  } tempah;

  struct CRTTrack {
    double xstart;
    double ystart;
    double zstart;
    double xend;
    double yend;
    double zend;
    double time;
    int id;
  };

  struct RecoTruth{
    std::vector<std::pair<std::pair<std::string,unsigned>,art::Ptr<crt::CRTData>>> sipmHits;
    std::vector<CRTStrip> stripHits;
    std::vector<CRTHit> crtHits;
    std::vector<CRTavehit> crtAveHits;
    std::vector<CRTTrack> crtTracks;
  };

  class CRTRecoAna : public art::EDAnalyzer {
  public:

    // Describes configuration parameters of the module
    struct Config {
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;
 
      // One Atom for each parameter
      fhicl::Atom<art::InputTag> SimModuleLabel {
        Name("SimModuleLabel"),
        Comment("tag of detector simulation data product")
      };

      fhicl::Atom<art::InputTag> CRTModuleLabel {
        Name("CRTModuleLabel"),
        Comment("tag of CRT simulation data product")
      };

      fhicl::Atom<bool> Verbose {
        Name("Verbose"),
        Comment("Print information about what's going on")
      };
      
    }; // Config
 
    using Parameters = art::EDAnalyzer::Table<Config>;
 
    // Constructor: configures module
    explicit CRTRecoAna(Parameters const& config);
 
    // Called once, at start of the job
    virtual void beginJob() override;
 
    // Called once per event
    virtual void analyze(const art::Event& event) override;

    // Called once, at end of the job
    virtual void endJob() override;

    std::vector<double> ChannelToLimits(CRTStrip strip);

    std::vector<double> CrtOverlap(std::vector<double> strip1, std::vector<double> strip2);
  
    bool CrossesTagger(const simb::MCParticle& particle, int tag_i);

    TVector3 TaggerCrossPoint(const simb::MCParticle& particle, int tag_i);

    bool IsThroughGoing(const simb::MCParticle& particle);

    void vmanip(std::vector<float> v, float* ave, float* rms);

    CRTavehit fillme(uint32_t i,uint16_t j,int32_t k,uint16_t l,float a,float b, float c,float d, float e, float f, float g, std::string p, int id);

    void DrawTrueTracks(const std::vector<simb::MCParticle>& particle, std::map<int, RecoTruth> truthMatch, bool truth, bool strips, bool hits, bool tracks);

    void DrawCube(TCanvas *c1, double *rmin, double *rmax, int col);

    CRTavehit copyme(CRTHit myhit);

    std::pair<std::string,unsigned> ChannelToTagger(uint32_t channel);

    bool CheckModuleOverlap(uint32_t channel);

  private:

    // fcl file parameters
    art::InputTag fSimModuleLabel;   ///< name of detsim producer
    art::InputTag fCRTModuleLabel;   ///< name of CRT producer
    bool          fVerbose;          ///< print information about what's going on

    // n-tuples
    TH2D* fTagXYZResolution[7];
    TH1D* fTagXResolution[7];
    TH1D* fTagYResolution[7];

    TH1D* fSipmHitsPerTrack;
    TH1D* fStripHitsPerTrack;
    TH1D* fCrtHitsPerTrack;
    TH1D* fAveHitsPerTrack;
    TH1D* fTracksPerTrack;

    TH1D* fXResidual;
    TH1D* fYResidual;
    TH1D* fZResidual;

    // Other variables shared between different methods.
    geo::GeometryCore const* fGeometryService;                 ///< pointer to Geometry provider
    detinfo::DetectorProperties const* fDetectorProperties;    ///< pointer to detector properties provider
    art::ServiceHandle<geo::AuxDetGeometry> fAuxDetGeoService;
    const geo::AuxDetGeometry* fAuxDetGeo;
    const geo::AuxDetGeometryCore* fAuxDetGeoCore;

    // Positions of the CRT planes
    std::vector<double> crtPlanes = {-359.1, -357.3, 357.3, 359.1, -358.9, -357.1, 661.52, 663.32, 865.52, 867.32, -240.65, -238.85, 655.35, 657.15};
    std::vector<int> fixCoord   = {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2}; // Fixed coordinate for each plane
    std::vector<int> widthCoord = {2, 1, 2, 1, 0, 2, 2, 0, 2, 0, 1, 0, 1, 0}; // Width direction for each plane
    std::vector<int> lenCoord   = {1, 2, 1, 2, 2, 0, 0, 2, 0, 2, 0, 1, 0, 1}; // Length direction for each plane

    const size_t nTaggers = 7;

    // Performance Counters
    int nSipms = 0;
    int nStrips = 0;
    int nHits = 0;

    int nMissedHits = 0;
    int nMatchingHits = 0;
    int nNoTruth = 0;
    int nMissTag[7] = {0,0,0,0,0,0,0};
    int nMatchTag[7] = {0,0,0,0,0,0,0};
    int nNoTruthTag[7] = {0,0,0,0,0,0,0};

    std::map<std::string, int> nameToInd;
    std::map<int, std::string> indToName;

  }; // class CRTRecoAna

  // Constructor
  CRTRecoAna::CRTRecoAna(Parameters const& config)
    : EDAnalyzer(config)
    , fSimModuleLabel  (config().SimModuleLabel())
    , fCRTModuleLabel  (config().CRTModuleLabel())
    , fVerbose         (config().Verbose())
  {
    // Get a pointer to the fGeometryServiceetry service provider
    fGeometryService = lar::providerFrom<geo::Geometry>();
    fDetectorProperties = lar::providerFrom<detinfo::DetectorPropertiesService>(); 
    fAuxDetGeo = &(*fAuxDetGeoService);
    fAuxDetGeoCore = fAuxDetGeo->GetProviderPtr();
  }

  void CRTRecoAna::beginJob()
  {
    // Access tfileservice to handle creating and writing histograms
    art::ServiceHandle<art::TFileService> tfs;
    // Define n-tuple
    fTagXYZResolution[0]    = tfs->make<TH2D>("tag0XYZresolution",  ";True Z - reco Z (cm);True Y - reco Y (cm)",  50, -25, 25,  50, -25, 25);
    fTagXYZResolution[1]    = tfs->make<TH2D>("tag1XYZresolution",  ";True Z - reco Z (cm);True Y - reco Y (cm)",  50, -25, 25,  50, -25, 25);
    fTagXYZResolution[2]    = tfs->make<TH2D>("tag2XYZresolution",  ";True X - reco X (cm);True Z - reco Z (cm)",  50, -25, 25,  50, -25, 25);
    fTagXYZResolution[3]    = tfs->make<TH2D>("tag3XYZresolution",  ";True Z - reco Z (cm);True X - reco X (cm)",  50, -25, 25,  50, -25, 25);
    fTagXYZResolution[4]    = tfs->make<TH2D>("tag4XYZresolution",  ";True Z - reco Z (cm);True X - reco X (cm)",  50, -25, 25,  50, -25, 25);
    fTagXYZResolution[5]    = tfs->make<TH2D>("tag5XYZresolution",  ";True Y - reco Y (cm);True X - reco X (cm)",  50, -25, 25,  50, -25, 25);
    fTagXYZResolution[6]    = tfs->make<TH2D>("tag6XYZresolution",  ";True Y - reco Y (cm);True X - reco X (cm)",  50, -25, 25,  50, -25, 25);

    fTagXResolution[0]    = tfs->make<TH1D>("tag0Xresolution",  ";True Z - reco Z (cm);",  50, -25, 25);
    fTagXResolution[1]    = tfs->make<TH1D>("tag1Xresolution",  ";True Z - reco Z (cm);",  50, -25, 25);
    fTagXResolution[2]    = tfs->make<TH1D>("tag2Xresolution",  ";True X - reco X (cm);",  50, -25, 25);
    fTagXResolution[3]    = tfs->make<TH1D>("tag3Xresolution",  ";True Z - reco Z (cm);",  50, -25, 25);
    fTagXResolution[4]    = tfs->make<TH1D>("tag4Xresolution",  ";True Z - reco Z (cm);",  50, -25, 25);
    fTagXResolution[5]    = tfs->make<TH1D>("tag5Xresolution",  ";True Y - reco Y (cm);",  50, -25, 25);
    fTagXResolution[6]    = tfs->make<TH1D>("tag6Xresolution",  ";True Y - reco Y (cm);",  50, -25, 25);

    fTagYResolution[0]    = tfs->make<TH1D>("tag0Yresolution",  ";True Y - reco Y (cm);",  50, -25, 25);
    fTagYResolution[1]    = tfs->make<TH1D>("tag1Yresolution",  ";True Y - reco Y (cm);",  50, -25, 25);
    fTagYResolution[2]    = tfs->make<TH1D>("tag2Yresolution",  ";True Z - reco Z (cm);",  50, -25, 25);
    fTagYResolution[3]    = tfs->make<TH1D>("tag3Yresolution",  ";True X - reco X (cm);",  50, -25, 25);
    fTagYResolution[4]    = tfs->make<TH1D>("tag4Yresolution",  ";True X - reco X (cm);",  50, -25, 25);
    fTagYResolution[5]    = tfs->make<TH1D>("tag5Yresolution",  ";True X - reco X (cm);",  50, -25, 25);
    fTagYResolution[6]    = tfs->make<TH1D>("tag6Yresolution",  ";True X - reco X (cm);",  50, -25, 25);

    fSipmHitsPerTrack  = tfs->make<TH1D>("sipmhitspertrack",   ";SiPM Hits;N Tracks",  20, 0, 20);
    fStripHitsPerTrack = tfs->make<TH1D>("striphitspertrack",  ";Strip Hits;N Tracks",  10, 0, 10);
    fCrtHitsPerTrack   = tfs->make<TH1D>("crthitspertrack",    ";CRT Hits;N Tracks",  10, 0, 10);
    fAveHitsPerTrack   = tfs->make<TH1D>("avehitspertrack",    ";Average Hits;N Tracks",  10, 0, 10);
    fTracksPerTrack    = tfs->make<TH1D>("trackshitspertrack", ";CRT Tracks;N Tracks",  10, 0, 10);

    fXResidual    = tfs->make<TH1D>("xresidual", ";X Residual (cm);N Tracks",  50, -1000, 1000);
    fYResidual    = tfs->make<TH1D>("yresidual", ";Y Residual (cm);N Tracks",  50, -1000, 1000);
    fZResidual    = tfs->make<TH1D>("zresidual", ";Z Residual (cm);N Tracks",  50, -1000, 1000);
    // Initial output
    std::cout<<"----------------- CRT Hit Reco Module -------------------"<<std::endl;

    // Take a position that is known to be the center of a strip

    nameToInd["volTaggerSideRight_0"] = 0;
    nameToInd["volTaggerSideLeft_0"] = 1;
    nameToInd["volTaggerBot_0"] = 2;
    nameToInd["volTaggerTopLow_0"] = 3;
    nameToInd["volTaggerTopHigh_0"] = 4;
    nameToInd["volTaggerFaceFront_0"] = 5;
    nameToInd["volTaggerFaceBack_0"] = 6;

    indToName[0] = "volTaggerSideRight_0";
    indToName[1] = "volTaggerSideLeft_0";
    indToName[2] = "volTaggerBot_0";
    indToName[3] = "volTaggerTopLow_0";
    indToName[4] = "volTaggerTopHigh_0";
    indToName[5] = "volTaggerFaceFront_0";
    indToName[6] = "volTaggerFaceBack_0";

  } // CRTRecoAna::beginJob()

  void CRTRecoAna::analyze(const art::Event& event)
  {

    // Fetch basic event info
    if(fVerbose){
      std::cout<<"============================================"<<std::endl
               <<"Run = "<<event.run()<<", SubRun = "<<event.subRun()<<", Event = "<<event.id().event()<<std::endl
               <<"============================================"<<std::endl;
    }

    // Detector properties
    double readoutWindow  = (double)fDetectorProperties->ReadOutWindowSize();
    double driftTimeTicks = 2.0*(2.*fGeometryService->DetHalfWidth()+3.)/fDetectorProperties->DriftVelocity();

    // Store the true x (CRT width direction) and y (CRT length direction) crossing points
    std::vector<TVector3> *taggerXYZ = new std::vector<TVector3>[nTaggers];
    std::map<int,TVector3> *partXYZ = new std::map<int,TVector3>[nTaggers];
    std::map<int,TVector3> *usedXYZ = new std::map<int,TVector3>[nTaggers];

    // Retrieve all the truth info in the events
    auto particleHandle = event.getValidHandle<std::vector<simb::MCParticle>>(fSimModuleLabel);
    // Fill a map of true particles
    std::vector<simb::MCParticle> particles;
    for (auto const& particle: (*particleHandle)){
      int partId = particle.TrackId();
      double pt = 2.*(particle.T()*10e-9)/(10e-6);
      // Check if the particle is in the reconstructible time window
      if(pt < -driftTimeTicks || pt > readoutWindow) continue;
      // Check particle is a muon
      if(std::abs(particle.PdgCode()) != 13) continue;
      // Check particle is through-going
      if(!IsThroughGoing(particle)) continue;
      // Loop over number of taggers
      for (size_t tag_i = 0; tag_i < nTaggers; tag_i++){
        if(!CrossesTagger(particle, tag_i)) continue;
        TVector3 crossPoint = TaggerCrossPoint(particle, tag_i);
        taggerXYZ[tag_i].push_back(crossPoint);
        partXYZ[tag_i][partId] = crossPoint;
      }
      //std::cout<<"PartID = "<<partId<<" Length = "<<particle.Trajectory().TotalLength()<<" time = "<<pt<<std::endl;
      particles.push_back(particle);
    }

    if(fVerbose) std::cout<<"Number of true particles = "<<particles.size()<<std::endl;

    // Retrieve list of CRT hits
    art::Handle< std::vector<crt::CRTData>> crtListHandle;
    std::vector<art::Ptr<crt::CRTData> > crtList;
    if (event.getByLabel(fCRTModuleLabel,crtListHandle))
      art::fill_ptr_vector(crtList, crtListHandle); 

    if(fVerbose) std::cout<<"Number of SiPM hits = "<<crtList.size()<<std::endl;

    // Fill a vector of pairs of time and width direction for each CRT plane
    // The y crossing point of z planes and z crossing point of y planes would be constant
    std::map<int, RecoTruth> truthMatch;
    std::map<std::pair<std::string,unsigned>, std::vector<CRTStrip>> taggerStrips;
    
    // Loop over all the SiPM hits in 2 (should be in pairs due to trigger)
    for (size_t i = 0; i < crtList.size(); i+=2){
      // Get the time, channel, center and width
      double t1 = (double)(int)crtList[i]->T0()/8.;
      uint32_t channel = crtList[i]->Channel();
      int strip = (channel >> 1) & 15;
      int module = (channel >> 5);
      std::string name = fGeometryService->AuxDet(module).TotalVolume()->GetName();
      TVector3 center = fAuxDetGeoCore->AuxDetChannelToPosition(2*strip, name);
      const geo::AuxDetSensitiveGeo stripGeo = fAuxDetGeoCore->ChannelToAuxDetSensitive(name, 2*strip);
      double width = 2*stripGeo.HalfWidth1();

      std::pair<std::string,unsigned> tagger = ChannelToTagger(channel);
      //std::cout<<"Tagger = "<<tagger.first<<" plane = "<<tagger.second<<" channel = "<<channel<<std::endl;

      // Check hit is inside the reconstructible window
      if (!(t1 >= -driftTimeTicks && t1 <= readoutWindow)) continue;
      nSipms += 2;

      // Sort the hit SiPMs by what plane they're in
      nStrips++;

      int id1 = crtList[i]->TrackID();
      int id2 = crtList[i+1]->TrackID(); 
      truthMatch[id1].sipmHits.push_back(std::make_pair(tagger, crtList[i]));
      truthMatch[id2].sipmHits.push_back(std::make_pair(tagger, crtList[i+1]));

      // Get the time of hit on the second SiPM
      double t2 = (double)(int)crtList[i+1]->T0()/8.;
      // Calculate the number of photoelectrons at each SiPM
      double npe1 = ((double)crtList[i]->ADC() - 63.6)/131.9;
      double npe2 = ((double)crtList[i+1]->ADC() - 63.6)/131.9;
      // Calculate the distance between the SiPMs
      double x = (width/2.)*atan(log(1.*npe2/npe1)) + (width/2.);

      // Calculate the error
      double normx = x + 0.344677*x - 1.92045;
      double ex = 1.92380e+00+1.47186e-02*normx-5.29446e-03*normx*normx;
      double time = (t1 + t2)/2.;

      CRTStrip stripHit = {time, channel, x, ex, id1, id2, tagger};
      taggerStrips[tagger].push_back(stripHit);

      truthMatch[id1].stripHits.push_back(stripHit);
      if(id1!=id2) truthMatch[id2].stripHits.push_back(stripHit);

    }

    // Remove any duplicate (same channel and time) hit strips
    for(auto &tagStrip : taggerStrips){
      std::sort(tagStrip.second.begin(), tagStrip.second.end(),
                [](const CRTStrip & a, const CRTStrip & b) -> bool{
                  return (a.t0 < b.t0) || 
                         ((a.t0 == b.t0) && (a.channel < b.channel));
                });
      // Remove hits with the same time and channel
      tagStrip.second.erase(std::unique(tagStrip.second.begin(), tagStrip.second.end(),
                                           [](const CRTStrip & a, const CRTStrip & b) -> bool{
                                             return a.t0 == b.t0 && a.channel == b.channel;
                                            }), tagStrip.second.end());
    }

    std::vector<CRTHit> crtHits;
    std::vector<std::string> usedTaggers;

    for (auto &tagStrip : taggerStrips){
      if (std::find(usedTaggers.begin(),usedTaggers.end(),tagStrip.first.first)!=usedTaggers.end()) continue;
      usedTaggers.push_back(tagStrip.first.first);
      unsigned planeID = 0;
      if(tagStrip.first.second==0) planeID = 1;
      std::pair<std::string,unsigned> otherPlane = std::make_pair(tagStrip.first.first, planeID);
      for (size_t hit_i = 0; hit_i < tagStrip.second.size(); hit_i++){
        // Get the position (in real space) of the 4 corners of the hit, taking charge sharing into account
        std::vector<double> limits1 =  ChannelToLimits(tagStrip.second[hit_i]);
        // Check for overlaps on the first plane
        if(CheckModuleOverlap(tagStrip.second[hit_i].channel)){
          // Loop over all the hits on the parallel (odd) plane
          for (size_t hit_j = 0; hit_j < taggerStrips[otherPlane].size(); hit_j++){
            // Get the limits in the two variable directions
            std::vector<double> limits2 = ChannelToLimits(taggerStrips[otherPlane][hit_j]);
            // If the time and position match then record the pair of hits
            std::vector<double> overlap = CrtOverlap(limits1, limits2);
            double t0_1 = tagStrip.second[hit_i].t0;
            double t0_2 = taggerStrips[otherPlane][hit_j].t0;
            if (overlap[0] != -99999 && std::abs(t0_1 - t0_2)<0.15){
              nHits++;
              // Calculate the mean and error in x, y, z
              TVector3 mean((overlap[0] + overlap[1])/2., 
                            (overlap[2] + overlap[3])/2., 
                            (overlap[4] + overlap[5])/2.);
              TVector3 error(std::abs((overlap[1] - overlap[0])/2.), 
                             std::abs((overlap[3] - overlap[2])/2.), 
                             std::abs((overlap[5] - overlap[4])/2.));
              // Average the time
              double time = (t0_1 + t0_2)/2;
              // Create a CRT hit
              // If the PID matches one of the true crossing particles calculate the resolution
              std::vector<int> ids = {tagStrip.second[hit_i].id1, 
                                      tagStrip.second[hit_i].id2, 
                                      taggerStrips[otherPlane][hit_j].id1, 
                                      taggerStrips[otherPlane][hit_j].id2};
              //int id = MostCommonID(ids);
              std::sort(ids.begin(), ids.end());
              ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
              CRTHit crtHit = {tagStrip.first.first, mean.X(), mean.Y(), mean.Z(), 
                               error.X(), error.Y(), error.Z(), 
                               time*0.5*10e-6, time*0.5*10e3, time*0.5*10e3, ids};
              crtHits.push_back(crtHit);

              int tag_i = nameToInd[tagStrip.first.first];
              for(int& ID : ids){
                if(partXYZ[tag_i].find(ID)!=partXYZ[tag_i].end()){
                  nMatchingHits++; 
                  nMatchTag[tag_i]++;
                  usedXYZ[tag_i][ID] = partXYZ[tag_i][ID];
                  TVector3 dist = partXYZ[tag_i][ID] - mean;
                  double distx = dist[widthCoord[tag_i*2]];
                  double disty = dist[lenCoord[tag_i*2]];
                  fTagXYZResolution[tag_i]->Fill(distx,disty); 
                  fTagXResolution[tag_i]->Fill(distx); 
                  fTagYResolution[tag_i]->Fill(disty);
                }
                else{ nNoTruth++; nNoTruthTag[tag_i]++; }
                truthMatch[ID].crtHits.push_back(crtHit);
              }

            }
          }
        }
        else{
          TVector3 mean((limits1[0] + limits1[1])/2., 
                        (limits1[2] + limits1[3])/2., 
                        (limits1[4] + limits1[5])/2.);
          TVector3 error(std::abs((limits1[1] - limits1[0])/2.), 
                         std::abs((limits1[3] - limits1[2])/2.), 
                         std::abs((limits1[5] - limits1[4])/2.));
          nHits++;
          double time = tagStrip.second[hit_i].t0;
          // Just use the single plane limits as the crt hit
          // If the PID matches calculate the resolution
          std::vector<int> ids = {tagStrip.second[hit_i].id1, tagStrip.second[hit_i].id2};
          //int id = MostCommonID(ids);
          std::sort(ids.begin(), ids.end());
          ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
          CRTHit crtHit = {tagStrip.first.first, mean.X(), mean.Y(), mean.Z(), 
                           error.X(), error.Y(), error.Z(), 
                           time*0.5*10e-6, time*0.5*10e3, time*0.5*10e3, ids};
          crtHits.push_back(crtHit);

          int tag_i = nameToInd[tagStrip.first.first];
          for(int& ID : ids){
            if(partXYZ[tag_i].find(ID)!=partXYZ[tag_i].end()){
              nMatchingHits++; 
              nMatchTag[tag_i]++;
              usedXYZ[tag_i][ID] = partXYZ[tag_i][ID];
              TVector3 dist = partXYZ[tag_i][ID] - mean;
              double distx = dist[widthCoord[tag_i*2]];
              double disty = dist[lenCoord[tag_i*2]];
              fTagXYZResolution[tag_i]->Fill(distx,disty); 
              fTagXResolution[tag_i]->Fill(distx); 
              fTagYResolution[tag_i]->Fill(disty);
            }
            else{ nNoTruth++; nNoTruthTag[tag_i]++; }
            truthMatch[ID].crtHits.push_back(crtHit);
          }

        }
      }
      for (size_t hit_j = 0; hit_j < taggerStrips[otherPlane].size(); hit_j++){
        // Get the limits in the two variable directions
        std::vector<double> limits1 = ChannelToLimits(taggerStrips[otherPlane][hit_j]);
        if(!CheckModuleOverlap(taggerStrips[otherPlane][hit_j].channel)){
          TVector3 mean((limits1[0] + limits1[1])/2., 
                        (limits1[2] + limits1[3])/2., 
                        (limits1[4] + limits1[5])/2.);
          TVector3 error(std::abs((limits1[1] - limits1[0])/2.), 
                         std::abs((limits1[3] - limits1[2])/2.), 
                         std::abs((limits1[5] - limits1[4])/2.));
          nHits++;
          double time = taggerStrips[otherPlane][hit_j].t0;
          // Just use the single plane limits as the crt hit
          // If the PID matches calculate the resolution
          std::vector<int> ids = {taggerStrips[otherPlane][hit_j].id1, taggerStrips[otherPlane][hit_j].id2};
          //int id = MostCommonID(ids);
          std::sort(ids.begin(), ids.end());
          ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
          CRTHit crtHit = {otherPlane.first, mean.X(), mean.Y(), mean.Z(), 
                           error.X(), error.Y(), error.Z(), 
                           time*0.5*10e-6, time*0.5*10e3, time*0.5*10e3, ids};
          crtHits.push_back(crtHit);

          int tag_i = nameToInd[tagStrip.first.first];
          for(int& ID : ids){
            if(partXYZ[tag_i].find(ID)!=partXYZ[tag_i].end()){
              nMatchingHits++; 
              nMatchTag[tag_i]++;
              usedXYZ[tag_i][ID] = partXYZ[tag_i][ID];
              TVector3 dist = partXYZ[tag_i][ID] - mean;
              double distx = dist[widthCoord[tag_i*2]];
              double disty = dist[lenCoord[tag_i*2]];
              fTagXYZResolution[tag_i]->Fill(distx,disty); 
              fTagXResolution[tag_i]->Fill(distx); 
              fTagYResolution[tag_i]->Fill(disty);
            }
            else{ nNoTruth++; nNoTruthTag[tag_i]++; }
            truthMatch[ID].crtHits.push_back(crtHit);
          }

        }
      }
    }

    // Loop over taggers
    for (size_t tag_i = 0; tag_i < nTaggers; tag_i++){
      // Loop over particle XYZ map
      for (auto & part : partXYZ[tag_i]) {
        // Find id in used list
        // If it's not there then count it
        if(usedXYZ[tag_i].find(part.first)==usedXYZ[tag_i].end()){
          nMissedHits++;
          nMissTag[tag_i]++;
        }
      }
    }

    std::vector<std::vector<CRTHit>> CRTTzeroVect;
    std::vector<int> npvec;
    int iflag[1000] = {};
    // Loop over crt hits
    for(size_t i = 0; i<crtHits.size(); i++){
      if (iflag[i]==0){
        std::vector<CRTHit> CRTTzero;
        std::map<std::string,int> nPlanes;
        double time_ns_A = crtHits[i].ts1_ns;
        iflag[i]=1;
        CRTTzero.push_back(crtHits[i]);
        nPlanes[crtHits[i].tagger]++;
        // Get the t0
        // Loop over all the other CRT hits
        for(size_t j = i+1; j<crtHits.size(); j++){
          if(iflag[j]==0){
            // If ts1_ns - ts1_ns < diff then put them in a vector
            double time_ns_B = crtHits[j].ts1_ns;
            double diff = std::abs(time_ns_B - time_ns_A)/(0.5*10e3);
            //if(crtHits[i].ids[0]==391&&crtHits[j].ids[0]==391) std::cout<<crtHits[i].tagger<<" "<<crtHits[j].tagger<<" diff = "<<diff<<"\n";
            if(diff<0.2){
              iflag[j]=1;
              CRTTzero.push_back(crtHits[j]);
              nPlanes[crtHits[j].tagger]++;
            }
          }
        }
        int np = 0;
        for(auto &nPlane : nPlanes){
          if(nPlane.second>0) np++;
        }
        CRTTzeroVect.push_back(CRTTzero);
        npvec.push_back(np);
      }
    }

    int nTracks=0;

    // Loop over tzeros
    for(size_t i = 0; i<CRTTzeroVect.size(); i++){

      //loop over hits and get average x,y,z,pe for each plane CHANGED FROM 4 TO 7
      std::map<std::string, std::vector<float>> thittime0;
      std::map<std::string, std::vector<float>> thittime1;
      std::map<std::string, std::vector<float>> tx;
      std::map<std::string, std::vector<float>> ty;
      std::map<std::string, std::vector<float>> tz;
      std::map<std::string, std::vector<float>> pe;
      std::map<std::string, std::vector<int>> ids;
      
      //double time_s_A = CRTTzeroVect[i][0].ts0_s;
      //      double time_s_err = CRTTzeroVect[i][0]->ts0_s_err;
      //double time_s_err = 0.;
      double time1_ns_A = CRTTzeroVect[i][0].ts1_ns;
      double time0_ns_A = CRTTzeroVect[i][0].ts0_ns;
      
      //loop over hits for this tzero, sort by plane
      for (size_t ah = 0; ah< CRTTzeroVect[i].size(); ++ah){        
        std::string ip = CRTTzeroVect[i][ah].tagger;       
        thittime0[ip].push_back(CRTTzeroVect[i][ah].ts0_ns-time0_ns_A);
        thittime1[ip].push_back(CRTTzeroVect[i][ah].ts1_ns-time1_ns_A);
        tx[ip].push_back(CRTTzeroVect[i][ah].xpos);
        ty[ip].push_back(CRTTzeroVect[i][ah].ypos);
        tz[ip].push_back(CRTTzeroVect[i][ah].zpos);
        ids[ip].push_back(CRTTzeroVect[i][ah].ids[0]);
      } // loop over hits
      
      std::map<std::string, CRTavehit> aveHits;
      //loop over planes and calculate average hits
      for (auto &keyVal : tx){
        std::string ip = keyVal.first;
        if (tx[ip].size()>0){
          uint32_t at0; int32_t at1; uint16_t rt0,rt1;
          float totpe=0.0;
          float avet1=0.0; float rmst1 =0.0; 
          float avet0=0.0; float rmst0 =0.0; 
          float avex=0.0; float rmsx =0.0; 
          float avey=0.0; float rmsy =0.0; 
          float avez=0.0; float rmsz =0.0;
          vmanip(thittime0[ip],&avet0,&rmst0);
          vmanip(thittime1[ip],&avet1,&rmst1);
          at0 = (uint32_t)(avet0+time0_ns_A); rt0 = (uint16_t)rmst0;   
          at1 = (int32_t)(avet1+time1_ns_A); rt1 = (uint16_t)rmst1;
          vmanip(tx[ip],&avex,&rmsx);
          vmanip(ty[ip],&avey,&rmsy);
          vmanip(tz[ip],&avez,&rmsz);
          totpe=std::accumulate(pe[ip].begin(), pe[ip].end(), 0.0);
          int id = MostCommonID(ids[ip]);
          CRTavehit aveHit = fillme(at0,rt0,at1,rt1,avex,rmsx,avey,rmsy,avez,rmsz,totpe,ip,id);
          aveHits[ip] = aveHit;

          ids[ip].erase(std::unique(ids[ip].begin(), ids[ip].end()), ids[ip].end());
          for(int& ID : ids[ip]){
            truthMatch[ID].crtAveHits.push_back(aveHit);
          }

        }
        else {
          CRTavehit aveHit = fillme(0,0,0,0,-99999,-99999,-99999,-99999,-99999,-99999,-99999,ip,-99999);
          aveHits[ip] = aveHit;
        }
      }

      if(npvec[i]>1){
        // find pairs of hits in different planes
        std::vector<std::string> usedTaggers;
        for (auto &AtagHit : aveHits){        
          CRTavehit Ahit = AtagHit.second;
          usedTaggers.push_back(AtagHit.first);
          if( Ahit.x_pos==-99999 ) continue;
          for (auto &BtagHit : aveHits){
            if (std::find(usedTaggers.begin(),usedTaggers.end(),BtagHit.first)!=usedTaggers.end()) continue; 
            CRTavehit Bhit = BtagHit.second;
            if ( Bhit.x_pos==-99999 ) continue;
      
            // Don't make tracks between the top two taggers FIXME
            if (nameToInd[BtagHit.first]==4 || nameToInd[AtagHit.first]==4) continue;  

            double time = (Ahit.ts0_ns + Bhit.ts0_ns)/2.;
            CRTTrack crtTrack = {Ahit.x_pos, Ahit.y_pos, Ahit.z_pos, Bhit.x_pos, Bhit.y_pos, Bhit.z_pos, time, Ahit.id};
            
            truthMatch[Ahit.id].crtTracks.push_back(crtTrack);
            if(Ahit.id!=Bhit.id) truthMatch[Bhit.id].crtTracks.push_back(crtTrack);

            nTracks++;
       
          }
       
        }
      }

    }

    std::cout<<"Number of hits = "<<crtHits.size()<<std::endl;
    std::cout<<"Number of T zero = "<<CRTTzeroVect.size()<<std::endl;
    std::cout<<"Number of tracks = "<<nTracks<<std::endl;

    //DrawTrueTracks(particles, truthMatch, false, false, false, true);

    for(auto const& particle : particles){
      int partId = particle.TrackId();
      RecoTruth rt = truthMatch[partId];
      fSipmHitsPerTrack->Fill(rt.sipmHits.size());
      fStripHitsPerTrack->Fill(rt.stripHits.size());
      fCrtHitsPerTrack->Fill(rt.crtHits.size());
      fAveHitsPerTrack->Fill(rt.crtAveHits.size());
      fTracksPerTrack->Fill(rt.crtTracks.size());
      if(rt.crtTracks.size()>0){
        double sx = rt.crtTracks[0].xstart;
        double sy = rt.crtTracks[0].ystart;
        double sz = rt.crtTracks[0].zstart;
        double ex = rt.crtTracks[0].xend;
        double ey = rt.crtTracks[0].yend;
        double ez = rt.crtTracks[0].zend;
        TVector3 d(ex-sx,ey-sy,ez-sz);
        int nTraj = particle.NumberTrajectoryPoints();
        int ipt = 0;
        double xres = 0;
        double yres = 0;
        double zres = 0;
        for(int j = 0; j < nTraj; j++){
          if(abs(particle.Vx(j))<500 && particle.Vy(j)<900 && particle.Vy(j)>-450 && particle.Vz(j)<700 && particle.Vz(j)>-400){
            double xpos = d[0]*(particle.Vy(j)-sy)/d[1] + sx;
            double ypos = d[1]*(particle.Vz(j)-sz)/d[2] + sy;
            double zpos = d[2]*(particle.Vx(j)-sx)/d[0] + sz;
            xres += particle.Vx(j) - xpos;
            yres += particle.Vy(j) - ypos;
            zres += particle.Vz(j) - zpos;
            ipt++;
          }
        }
        fXResidual->Fill(xres/ipt);
        fYResidual->Fill(yres/ipt);
        fZResidual->Fill(zres/ipt);
        std::cout<<"x residual = "<<xres/ipt<<std::endl;
      }
    }
/*
    for(auto const& particle : particles){
      int partId = particle.TrackId();
      //Print the crossing points
      std::cout<<"\nParticle "<<partId<<": Start("<<particle.Vx()<<","<<particle.Vy()<<","<<particle.Vz()<<") End("<<particle.EndX()<<","<<particle.EndY()<<","<<particle.EndZ()<<") time = "<<2.*(particle.T()*10e-9)/(10e-6)<<"\nTrue crossing points:\n";
      for(size_t tag_i = 0; tag_i < nTaggers; tag_i++){
        if(partXYZ[tag_i].find(partId)!=partXYZ[tag_i].end()){
          std::cout<<"Tagger "<<tag_i<<": Coordinates = ("<<partXYZ[tag_i][partId].X()<<", "<<partXYZ[tag_i][partId].Y()<<", "<<partXYZ[tag_i][partId].Z()<<")\n";
        }
      }
      RecoTruth rt = truthMatch[partId];
      std::cout<<"->Reco tracks:\n";
      for(size_t trk_i = 0; trk_i < rt.crtTracks.size(); trk_i++){
        CRTTrack tr = rt.crtTracks[trk_i];
        std::cout<<"  Start = ("<<tr.xstart<<","<<tr.ystart<<","<<tr.zstart<<") End = ("<<tr.xend<<","<<tr.yend<<","<<tr.zend<<") time = "<<tr.time/(0.5*10e3)<<" id = "<<tr.id<<"\n";
      }
      std::cout<<"-->Average hits:\n";
      for(size_t hit_i = 0; hit_i < rt.crtAveHits.size(); hit_i++){
        CRTavehit ah = rt.crtAveHits[hit_i];
        std::cout<<"   "<<ah.tagger<<": Coordinates = ("<<ah.x_pos<<","<<ah.y_pos<<","<<ah.z_pos<<") time = "<<ah.ts1_ns/(0.5*10e3)<<" id = "<<ah.id<<"\n";
      }
      std::cout<<"--->Reco crossing points:\n";
      for(size_t hit_i = 0; hit_i < rt.crtHits.size(); hit_i++){
        CRTHit ht = rt.crtHits[hit_i];
        std::cout<<"    "<<ht.tagger<<": Coordinates = ("<<ht.xpos<<", "<<ht.ypos<<", "<<ht.zpos<<") time = "<<ht.ts0_ns/(0.5*10e3);
        for(int id : ht.ids){ std::cout<<" id = "<<id;}
        std::cout<<"\n";
      }
      std::cout<<"---->Strip hits:\n";
      for(size_t hit_i = 0; hit_i < rt.stripHits.size(); hit_i++){
        CRTStrip sp = rt.stripHits[hit_i];
        std::cout<<"     "<<sp.tagger.first<<" ("<<sp.tagger.second<<"): time = "<<sp.t0<<" id1 = "<<sp.id1<<" id2 = "<<sp.id2<<"\n";
      }
      std::cout<<"----->SiPM hits:\n";
      for(size_t hit_i = 0; hit_i < rt.sipmHits.size(); hit_i++){
        std::pair<std::string,unsigned> tagger = rt.sipmHits[hit_i].first;
        art::Ptr<crt::CRTData> si = rt.sipmHits[hit_i].second;
        std::cout<<"      "<<tagger.first<<" ("<<tagger.second<<"): Channel = "<<si->Channel()<<" time = "<<(double)(int)si->T0()/8.<<" ID = "<<si->TrackID()<<"\n";
      }
    }
*/
    delete[] taggerXYZ;
    delete[] partXYZ;
    delete[] usedXYZ;

  } // CRTRecoAna::analyze()

  void CRTRecoAna::endJob(){

    TF1 *fx[7];
    TF1 *fy[7];
    for(int i = 0; i < 7; i++){
      TString fxname = Form("f%ix", i);
      TString fyname = Form("f%iy", i);
      fx[i] = new TF1(fxname,"gaus");
      fTagXResolution[i]->Fit(fxname,"Q");
      fy[i] = new TF1(fyname,"gaus");
      fTagYResolution[i]->Fit(fyname,"Q");
    }

    std::cout<<"=========================== GENERAL INFORMATION ===========================\n"
             <<"Number of hit SiPMs = "<<nSipms
             <<"\nNumber of hit strips = "<<nStrips
             <<"\nNumber of CRT hits = "<<nHits
             <<"\nTotal number of matched hits = "<<nMatchingHits
             <<"\nTotal number of missed hits = "<<nMissedHits
             <<"\nTotal number of hits with no truth = "<<nNoTruth<<"\n\n"
             <<"=========================== TAGGER INFORMATION ============================\n";
    for(int i = 0; i < 7; i++){
    std::cout<<"---->"<<indToName[i]<<":\n"
             <<"Z resolution = "<<fx[i]->GetParameter(2)<<" +/- "<<fx[i]->GetParError(2)<<" cm, Z bias = "<<fx[i]->GetParameter(1)<<" +/- "<<fx[i]->GetParError(1)<<" cm\n"
             <<"Y resolution = "<<fy[i]->GetParameter(2)<<" +/- "<<fy[i]->GetParError(2)<<" cm, Y bias = "<<fy[i]->GetParameter(1)<<" +/- "<<fy[i]->GetParError(1)<<" cm\n"
             <<"Efficiency = "<<(double)nMatchTag[i]/(nMatchTag[i]+nMissTag[i])<<"\n"
             <<nMatchTag[i]<<" matched hits, "<<nMissTag[i]<<" missed hits, "<<nNoTruthTag[i]<<" hits with no truth info\n\n";
    }

  } // CRTRecoAna::endJob()

  // Function to calculate the strip position limits in real space from channel
  std::vector<double> CRTRecoAna::ChannelToLimits(CRTStrip stripHit){
    int strip = (stripHit.channel >> 1) & 15;
    int module = (stripHit.channel >> 5);
    std::string name = fGeometryService->AuxDet(module).TotalVolume()->GetName();
    const geo::AuxDetSensitiveGeo stripGeo = fAuxDetGeoCore->ChannelToAuxDetSensitive(name, 2*strip);
    double halfWidth = stripGeo.HalfWidth1();
    double halfHeight = stripGeo.HalfHeight();
    double halfLength = stripGeo.HalfLength();
    double l1[3] = {-halfWidth+stripHit.x+stripHit.ex, halfHeight, halfLength};
    double w1[3] = {0,0,0};
    stripGeo.LocalToWorld(l1, w1);
    double l2[3] = {-halfWidth+stripHit.x-stripHit.ex, -halfHeight, -halfLength};
    double w2[3] = {0,0,0};
    stripGeo.LocalToWorld(l2, w2);
    // Use this to get the limits in the two variable directions
    std::vector<double> limits = {std::min(w1[0],w2[0]), std::max(w1[0],w2[0]), 
                                  std::min(w1[1],w2[1]), std::max(w1[1],w2[1]), 
                                  std::min(w1[2],w2[2]), std::max(w1[2],w2[2])};
    return limits;
  } // CRTRecoAna::ChannelToLimits

  // Function to calculate the overlap between two crt strips
  std::vector<double> CRTRecoAna::CrtOverlap(std::vector<double> strip1, std::vector<double> strip2){
    double minX = std::max(strip1[0], strip2[0]);
    double maxX = std::min(strip1[1], strip2[1]);
    double minY = std::max(strip1[2], strip2[2]);
    double maxY = std::min(strip1[3], strip2[3]);
    double minZ = std::max(strip1[4], strip2[4]);
    double maxZ = std::min(strip1[5], strip2[5]);
    std::vector<double> null = {-99999, -99999, -99999, -99999, -99999, -99999};
    std::vector<double> overlap = {minX, maxX, minY, maxY, minZ, maxZ};
    if ((minX<maxX && minY<maxY) || (minX<maxX && minZ<maxZ) || (minY<maxY && minZ<maxZ)) return overlap;
    return null;
  } // CRTRecoAna::CRTOverlap()
    
  bool CRTRecoAna::CrossesTagger(const simb::MCParticle& particle, int tag_i){
    double tagCenter[3] = {0, 0, 208.25};
    tagCenter[fixCoord[tag_i*2]] = (crtPlanes[tag_i*2]+crtPlanes[tag_i*2+1])/2;
    double tagDim[3] = {0, 0, 0};
    if(tag_i==0 || tag_i==1){ tagDim[0] = 1.8; tagDim[1] = 360; tagDim[2] = 450; }
    if(tag_i==2){ tagDim[0] = 399.5; tagDim[1] = 1.8; tagDim[2] = 478; }
    if(tag_i==3 || tag_i==4){ tagDim[0] = 450; tagDim[1] = 1.8; tagDim[2] = 450; }
    if(tag_i==5 || tag_i==6){ tagDim[0] = 360; tagDim[1] = 360; tagDim[2] = 1.8; }
    bool crosses = false;
    // Get the trajectory of the true particle
    size_t npts = particle.NumberTrajectoryPoints();
    // Loop over particle trajectory
    for (size_t i = 0; i < npts; i++){
      double trajPoint[3] = {particle.Vx(i), particle.Vy(i), particle.Vz(i)};
      // If the particle is inside the tagger volume then set to true.
      if(trajPoint[0]>tagCenter[0]-tagDim[0] && trajPoint[0]<tagCenter[0]+tagDim[0] &&
         trajPoint[1]>tagCenter[1]-tagDim[1] && trajPoint[1]<tagCenter[1]+tagDim[1] &&
         trajPoint[2]>tagCenter[2]-tagDim[2] && trajPoint[2]<tagCenter[2]+tagDim[2]) crosses = true;
    }
    return crosses;
  }

  TVector3 CRTRecoAna::TaggerCrossPoint(const simb::MCParticle& particle, int tag_i){
    double tagCenter[3] = {0, 0, 208.25};
    tagCenter[fixCoord[tag_i*2]] = (crtPlanes[tag_i*2]+crtPlanes[tag_i*2+1])/2;
    double tagDim[3] = {0, 0, 0};
    if(tag_i==0 || tag_i==1){ tagDim[0] = 1.8; tagDim[1] = 360; tagDim[2] = 450; }
    if(tag_i==2){ tagDim[0] = 399.5; tagDim[1] = 1.8; tagDim[2] = 478; }
    if(tag_i==3 || tag_i==4){ tagDim[0] = 450; tagDim[1] = 1.8; tagDim[2] = 450; }
    if(tag_i==5 || tag_i==6){ tagDim[0] = 360; tagDim[1] = 360; tagDim[2] = 1.8; }
    TVector3 start, end;
    bool first = true;
    // Get the trajectory of the true particle
    size_t npts = particle.NumberTrajectoryPoints();
    // Loop over particle trajectory
    for (size_t i = 0; i < npts; i++){
      TVector3 trajPoint(particle.Vx(i), particle.Vy(i), particle.Vz(i));
      // If the particle is inside the tagger volume then set to true.
      if(trajPoint[0]>tagCenter[0]-tagDim[0] && trajPoint[0]<tagCenter[0]+tagDim[0] &&
         trajPoint[1]>tagCenter[1]-tagDim[1] && trajPoint[1]<tagCenter[1]+tagDim[1] &&
         trajPoint[2]>tagCenter[2]-tagDim[2] && trajPoint[2]<tagCenter[2]+tagDim[2]){
        if(first) start = trajPoint;
        first = false;
        end = trajPoint;
      }
    }
    TVector3 crossPoint((start.X()+end.X())/2,(start.Y()+end.Y())/2,(start.Z()+end.Z())/2);
    return crossPoint;
  }

  void CRTRecoAna::DrawTrueTracks(const std::vector<simb::MCParticle>& particle, std::map<int, RecoTruth> truthMatch, bool truth, bool strips, bool hits, bool tracks){
    // Create a canvas 
    TCanvas *c1 = new TCanvas("c1","",700,700);
    // Draw the tagger planes
    for(int tag_i = 0; tag_i < 7; tag_i++){
      double tagCenter[3] = {0, 0, 208.25};
      tagCenter[fixCoord[tag_i*2]] = (crtPlanes[tag_i*2]+crtPlanes[tag_i*2+1])/2;
      double tagDim[3] = {0, 0, 0};
      if(tag_i==0 || tag_i==1){ tagDim[0] = 1.8; tagDim[1] = 360; tagDim[2] = 450; }
      if(tag_i==2){ tagDim[0] = 399.5; tagDim[1] = 1.8; tagDim[2] = 478; }
      if(tag_i==3 || tag_i==4){ tagDim[0] = 450; tagDim[1] = 1.8; tagDim[2] = 450; }
      if(tag_i==5 || tag_i==6){ tagDim[0] = 360; tagDim[1] = 360; tagDim[2] = 1.8; }
      double rmin[3] = {tagCenter[0]-tagDim[0],tagCenter[1]-tagDim[1],tagCenter[2]-tagDim[2]};
      double rmax[3] = {tagCenter[0]+tagDim[0],tagCenter[1]+tagDim[1],tagCenter[2]+tagDim[2]};
      DrawCube(c1, rmin, rmax, 1);
    }

    // Draw the true particles
    //TPolyLine3D *trajectories[100];
    TPolyLine3D *crttrack[100];
    int ncrtTracks = 0;
    /*for(size_t i = 0; i < particle.size(); i++){
      int id = particle[i].TrackId();
      if(truth){
        int nTraj = particle[i].NumberTrajectoryPoints();
        trajectories[i] = new TPolyLine3D(nTraj);
        int ipt = 0;
        for(int j = 0; j < nTraj; j++){
          if(abs(particle[i].Vx(j))<500 && particle[i].Vy(j)<900 && particle[i].Vy(j)>-450 && particle[i].Vz(j)<700 && particle[i].Vz(j)>-400){
            trajectories[i]->SetPoint(ipt, particle[i].Vx(j), particle[i].Vy(j), particle[i].Vz(j));
            ipt++;
          }
        }
        trajectories[i]->SetLineColor(851+i);
        trajectories[i]->SetLineWidth(2);
        trajectories[i]->Draw();
      }
      RecoTruth rt = truthMatch[id];*/
    for(auto &irt : truthMatch){
      RecoTruth rt = irt.second;
      if(strips){
        // Plot the hit strips
        for(size_t j = 0; j < rt.stripHits.size(); j++){
          // Calculate the limits
          std::vector<double> limits = ChannelToLimits(rt.stripHits[j]);
          // Plot a rectangle
          double rmin[3] = {limits[0], limits[2], limits[4]};
          double rmax[3] = {limits[1], limits[3], limits[5]};
          DrawCube(c1, rmin, rmax, 2);
        }
      }
      if(hits){
        // Plot the hits
        for(size_t j = 0; j < rt.crtHits.size(); j++){
          CRTHit ht = rt.crtHits[j];
          // Get the limits
          double rmin[3] = {ht.xpos-ht.xerr,ht.ypos-ht.yerr,ht.zpos-ht.yerr};
          double rmax[3] = {ht.xpos+ht.xerr,ht.ypos+ht.yerr,ht.zpos+ht.yerr};
          DrawCube(c1, rmin, rmax, 2);
        }
      }
      if(tracks){
        // Plot the tracks
        for(size_t j = 0; j < rt.crtTracks.size(); j++){
          // Get the start and end points
          CRTTrack tr = rt.crtTracks[j];
          crttrack[ncrtTracks] = new TPolyLine3D(2);
          crttrack[ncrtTracks]->SetPoint(0, tr.xstart, tr.ystart, tr.zstart);
          crttrack[ncrtTracks]->SetPoint(1, tr.xend, tr.yend, tr.zend);
          // Draw a line between them
          crttrack[ncrtTracks]->SetLineColor(2);
          crttrack[ncrtTracks]->SetLineWidth(2);
          crttrack[ncrtTracks]->Draw();
          ncrtTracks++;
        }
      }
    }

    c1->SaveAs("crtTagger.root");
  }

  void CRTRecoAna::DrawCube(TCanvas *c1, double *rmin, double *rmax, int col){
    c1->cd();
    TList *outline = new TList;
    TPolyLine3D *p1 = new TPolyLine3D(4);
    TPolyLine3D *p2 = new TPolyLine3D(4);
    TPolyLine3D *p3 = new TPolyLine3D(4);
    TPolyLine3D *p4 = new TPolyLine3D(4);
    p1->SetLineColor(col);
    p1->SetLineWidth(2);
    p1->Copy(*p2);
    p1->Copy(*p3);
    p1->Copy(*p4);
    outline->Add(p1);
    outline->Add(p2);
    outline->Add(p3);
    outline->Add(p4); 
    TPolyLine3D::DrawOutlineCube(outline, rmin, rmax);
    p1->Draw();
    p2->Draw();
    p3->Draw();
    p4->Draw();
  }

  bool CRTRecoAna::IsThroughGoing(const simb::MCParticle& particle){
    // Check if particle starts and ends outside the CRT planes
    bool startOutside = false;
    bool endOutside = false;
    TVector3 start(particle.Vx(), particle.Vy(), particle.Vz());
    TVector3 end(particle.EndX(), particle.EndY(), particle.EndZ());
    if(start[0]<crtPlanes[0] || start[0]>crtPlanes[3] ||
       start[1]<crtPlanes[4] || start[1]>crtPlanes[9] ||
       start[2]<crtPlanes[10] || start[2]>crtPlanes[13]) startOutside = true;
    if(end[0]<crtPlanes[0] || end[0]>crtPlanes[3] ||
       end[1]<crtPlanes[4] || end[1]>crtPlanes[9] ||
       end[2]<crtPlanes[10] || end[2]>crtPlanes[13]) endOutside = true;

    // Check if particle enters the TPC
    bool enters = false;
    double xmin = -2.0 * fGeometryService->DetHalfWidth();
    double xmax = 2.0 * fGeometryService->DetHalfWidth();
    double ymin = -fGeometryService->DetHalfHeight();
    double ymax = fGeometryService->DetHalfHeight();
    double zmin = 0.;
    double zmax = fGeometryService->DetLength();
    // Loop over trajectory points
    int nTrajPoints = particle.NumberTrajectoryPoints();
    for (int traj_i = 0; traj_i < nTrajPoints; traj_i++){
      TVector3 trajPoint(particle.Vx(traj_i), particle.Vy(traj_i), particle.Vz(traj_i));
      // Check if point is within reconstructable volume
      if (trajPoint[0] >= xmin && trajPoint[0] <= xmax && trajPoint[1] >= ymin && trajPoint[1] <= ymax && trajPoint[2] >= zmin && trajPoint[2] <= zmax){
        enters = true;
      }
    }

    // Only count as through going if particle starts and ends outside the CRT 
    // enclosed volume and enters the TPC
    if(startOutside && endOutside && enters) return true;
    return false;
  }

  void CRTRecoAna::vmanip(std::vector<float> v, float* ave, float* rms)
{
  *ave=0.0; *rms =0.0;
  if (v.size()>0) {
    /*
    int np=v.size();
    for (int i=0;i<np;++i) {
      std::cout << v[i] << " " ;
    }
    std::cout << std::endl;
    */

    //  find the mean and *rms of all the vector elements
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    double mean = sum / v.size();
    *ave=mean;
    
    if (v.size()>1) {
    double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / v.size() - mean * mean);
    *rms=stdev;
    }

  }
  //    std::cout << "inside vmanip " << *ave << " " << *rms << std::endl;
}

CRTavehit CRTRecoAna::fillme(uint32_t i,uint16_t j,int32_t k,uint16_t l,float a,float b, 
                 float c, float d, float e,float f,float g,std::string p, int id)
{
  CRTavehit h;
  h.ts0_ns=i;
  h.ts0_ns_err=j;
  h.ts1_ns=k; 
  h.ts1_ns_err=l;                                                        
  
  h.x_pos=a;
  h.x_err=b;
  h.y_pos=c;
  h.y_err=d;
  h.z_pos=e;
  h.z_err=f;
  h.tagger=p;
  h.id=id;

  return(h);
}

CRTavehit CRTRecoAna::copyme(CRTHit myhit)
{
  CRTavehit h;
  h.ts0_ns=myhit.ts0_ns;
  h.ts0_ns_err=0;
  h.ts1_ns=myhit.ts1_ns;; 
  h.ts1_ns_err=0;       
  h.x_pos=myhit.xpos;
  h.x_err=myhit.xerr;
  h.y_pos=myhit.ypos;
  h.y_err=myhit.yerr;
  h.z_pos=myhit.zpos;
  h.z_err=myhit.zerr;
  h.tagger=myhit.tagger;
  return(h);
}

  std::pair<std::string,unsigned> CRTRecoAna::ChannelToTagger(uint32_t channel){
    int strip = (channel >> 1) & 15;
    int module = (channel >> 5);
    std::string name = fGeometryService->AuxDet(module).TotalVolume()->GetName();
    TVector3 center = fAuxDetGeoCore->AuxDetChannelToPosition(2*strip, name);
    const geo::AuxDetSensitiveGeo stripGeo = fAuxDetGeoCore->ChannelToAuxDetSensitive(name, 2*strip);

    std::set<std::string> volNames = {stripGeo.TotalVolume()->GetName()};
    std::vector<std::vector<TGeoNode const*> > paths = fGeometryService->FindAllVolumePaths(volNames);
    std::string path = "";
    for (size_t inode=0; inode<paths.at(0).size(); inode++) {
      path += paths.at(0).at(inode)->GetName();
      if (inode < paths.at(0).size() - 1) {
        path += "/";
      }
    }
    TGeoManager* manager = fGeometryService->ROOTGeoManager();
    manager->cd(path.c_str());
    TGeoNode* nodeModule = manager->GetMother(2);
    TGeoNode* nodeTagger = manager->GetMother(3);
    // Module position in parent (tagger) frame
    double origin[3] = {0, 0, 0};
    double modulePosMother[3];
    nodeModule->LocalToMaster(origin, modulePosMother);
    unsigned planeID = (modulePosMother[2] > 0);
    std::string tagName = nodeTagger->GetName();
    std::pair<std::string, unsigned> output = std::make_pair(tagName, planeID);
    return output;
  }

  // WARNING: Relies on all modules in a tagger having the same dimensions
  bool CRTRecoAna::CheckModuleOverlap(uint32_t channel){
    bool hasOverlap = false;
    // Get the module ID
    int strip = (channel >> 1) & 15;
    int module = (channel >> 5);
    // Get the name of the module
    std::string name = fGeometryService->AuxDet(module).TotalVolume()->GetName();
    // Get the tagger TGeoNode
    const geo::AuxDetSensitiveGeo stripGeo = fAuxDetGeoCore->ChannelToAuxDetSensitive(name, 2*strip);
    std::set<std::string> volNames = {stripGeo.TotalVolume()->GetName()};
    std::vector<std::vector<TGeoNode const*> > paths = fGeometryService->FindAllVolumePaths(volNames);
    std::string path = "";
    for (size_t inode=0; inode<paths.at(0).size(); inode++) {
      path += paths.at(0).at(inode)->GetName();
      if (inode < paths.at(0).size() - 1) {
        path += "/";
      }
    }
    TGeoManager* manager = fGeometryService->ROOTGeoManager();
    manager->cd(path.c_str());
    TGeoNode* nodeModule = manager->GetMother(2);
    TGeoNode* nodeTagger = manager->GetMother(3);
    std::string modName = nodeModule->GetName();
    // Get the limits of the module in the tagger frame
    double height = fGeometryService->AuxDet(module).HalfHeight();
    double width = fGeometryService->AuxDet(module).HalfWidth1();
    double length = fGeometryService->AuxDet(module).Length()/2.;
    double pos1[3] = {width, height, length};
    double tagp1[3];
    nodeModule->LocalToMaster(pos1, tagp1);
    double pos2[3] = {-width, -height, -length};
    double tagp2[3];
    nodeModule->LocalToMaster(pos2, tagp2);
    std::vector<double> limits = {std::min(tagp1[0],tagp2[0]),
                                  std::max(tagp1[0],tagp2[0]),
                                  std::min(tagp1[1],tagp2[1]),
                                  std::max(tagp1[1],tagp2[1]),
                                  std::min(tagp1[2],tagp2[2]),
                                  std::max(tagp1[2],tagp2[2])};
    double origin[3] = {0, 0, 0};
    double modulePosMother[3];
    nodeModule->LocalToMaster(origin, modulePosMother);
    unsigned planeID = (modulePosMother[2] > 0);

    // Get the number of daughters from the tagger
    int nDaughters = nodeTagger->GetNdaughters();
    // Loop over the daughters
    for(int mod_i = 0; mod_i < nDaughters; mod_i++){
      // Check the name not the same as the current module
      TGeoNode* nodeDaughter = nodeTagger->GetDaughter(mod_i);
      std::string d_name = nodeDaughter->GetName();
      // Remove last two characters from name to match the AuxDet name
      if(d_name == modName) continue;
      // Get the limits of the module in the tagger frame
      double d_tagp1[3];
      nodeDaughter->LocalToMaster(pos1, d_tagp1);
      double d_tagp2[3];
      nodeDaughter->LocalToMaster(pos2, d_tagp2);
      std::vector<double> d_limits = {std::min(d_tagp1[0],d_tagp2[0]),
                                      std::max(d_tagp1[0],d_tagp2[0]),
                                      std::min(d_tagp1[1],d_tagp2[1]),
                                      std::max(d_tagp1[1],d_tagp2[1]),
                                      std::min(d_tagp1[2],d_tagp2[2]),
                                      std::max(d_tagp1[2],d_tagp2[2])};
      double d_modulePosMother[3];
      nodeDaughter->LocalToMaster(origin, d_modulePosMother);
      unsigned d_planeID = (d_modulePosMother[2] > 0);

      // Check the overlap of the two modules
      std::vector<double> overlap = CrtOverlap(limits, d_limits);
      // If there is an overlap set to true
      if(overlap[0]!=-99999 && d_planeID!=planeID) hasOverlap = true;
    }
    return hasOverlap;
  }

  DEFINE_ART_MODULE(CRTRecoAna)
} // namespace sbnd

// Back to local namespace.
namespace {

  int MostCommonID(std::vector<int> ids){
    std::map<int, int> mydict = {};
    int count = 0;
    int idout = 0;
    for (auto& id : ids){
      mydict[id] = mydict.emplace(id, 0).first->second +1;
      if (mydict[id] > count || (mydict[id]==count && id>=0)) {
        std::tie(count, idout) = std::tie(mydict[id], id);
      }
    }
    return idout;
  }

} // local namespace


