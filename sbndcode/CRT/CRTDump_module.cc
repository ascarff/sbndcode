////////////////////////////////////////////////////////////////////////
// Class:       CRTDump
// Module Type: analyzer
// File:        CRTDump_module.cc
//
// Generated at Wed Aug 30 09:59:49 2017 by Andrew Mastbaum using artmod
// from cetpkgsupport v1_10_01.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "larcore/Geometry/Geometry.h"
#include "larcore/Geometry/AuxDetGeometry.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTrajectory.h"
#include "larcorealg/Geometry/AuxDetGeo.h"
#include "larcorealg/Geometry/AuxDetSensitiveGeo.h"
#include "larcorealg/Geometry/AuxDetGeometryCore.h"

#include "sbndcode/CRT/CRTData.hh"
#include "TGeoManager.h"
#include "TFile.h"
#include "TNtuple.h"
#include "TString.h"

#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include "lardata/DetectorInfo/ElecClock.h"
#include <iostream>
#include <vector>

namespace sbnd {
  namespace crt {
    class CRTDump;
  }
}

class sbnd::crt::CRTDump : public art::EDAnalyzer {
public:
  explicit CRTDump(fhicl::ParameterSet const & p);
  ~CRTDump();

  CRTDump(CRTDump const &) = delete;
  CRTDump(CRTDump &&) = delete;
  CRTDump & operator = (CRTDump const &) = delete;
  CRTDump & operator = (CRTDump &&) = delete;

  void analyze(art::Event const & e) override;

  void beginJob() override {}
  void reconfigure(fhicl::ParameterSet const & p) override {}

private:
  TFile* fout;
  TNtuple* track;
  uint32_t t0;
  TString volname;
  TVector3 position;
  TVector3 momentumEnd;
  int channel;
  TTree* hits;
  int eventIndex;
  double t;
};

sbnd::crt::CRTDump::CRTDump(fhicl::ParameterSet const & p): EDAnalyzer(p) {
  //create the track root file
  fout = TFile::Open("track.root", "recreate");
  fout->cd();
  //make the track ntuple
  track = new TNtuple("track", "", "x:y:z:t:pdg");
  // make the hits Tree
  hits = new TTree("hits", "");
  hits->Branch("volname", &volname);
  hits->Branch("t0", &t0);
  hits->Branch("hitpos", &position);
  hits->Branch("channel", &channel);
  hits->Branch("eventIndex", &eventIndex);
  hits->Branch("t", &t);
  hits->Branch("Momentum", &momentumEnd);

  eventIndex = 0;
}

sbnd::crt::CRTDump::~CRTDump() {
  fout->cd();
  track->Write();
  hits->Write();
}

void sbnd::crt::CRTDump::analyze(art::Event const & e) {
  // Services
  art::ServiceHandle<geo::Geometry> geoService;

  art::ServiceHandle<geo::AuxDetGeometry> adGeoService;
  const geo::AuxDetGeometry* adG = &(*adGeoService);
  const geo::AuxDetGeometryCore* adGeoCore = adG->GetProviderPtr();

  // Handle for (truth) AuxDetSimChannels
  art::Handle<std::vector<sim::AuxDetSimChannel> > channels;
  e.getByLabel("largeant", channels);

  // Handle for CRTData
  art::Handle<std::vector<sbnd::crt::CRTData> > crtData;
  e.getByLabel("crt", crtData);

  std::cout << "channels.size(): " << channels.product()->size() << std::endl;
  std::cout << "crtData.size(): " << crtData.product()->size() << std::endl;

  // Loop through truth AD channels
  for (auto& adsc : *channels) {
    const geo::AuxDetGeo& adGeo = \
        geoService->AuxDet(adsc.AuxDetID());

    const geo::AuxDetSensitiveGeo& adsGeo = \
        adGeo.SensitiveVolume(adsc.AuxDetSensitiveID());

    // Output the CRT response for each hit
    for (auto ide : adsc.AuxDetIDEs()) {
      // Find the path to the strip geo node, to locate it in the hierarchy
      std::set<std::string> volNames = { adsGeo.TotalVolume()->GetName() };
      std::vector<std::vector<TGeoNode const*> > paths = \
        geoService->FindAllVolumePaths(volNames);

      std::string path = "";
      for (size_t inode=0; inode<paths.at(0).size(); inode++) {
        path += paths.at(0).at(inode)->GetName();
        if (inode < paths.at(0).size() - 1) {
          path += "/";
        }
      }

      TGeoManager* manager = geoService->ROOTGeoManager();
      manager->cd(path.c_str());

      //TGeoNode* nodeStrip = manager->GetCurrentNode();
      //TGeoNode* nodeArray = manager->GetMother(1);
      TGeoNode* nodeModule = manager->GetMother(2);
      //TGeoNode* nodeTagger = manager->GetMother(3);

      // Module position in parent (tagger) frame
      double origin[3] = {0, 0, 0};
      double modulePosMother[3];
      nodeModule->LocalToMaster(origin, modulePosMother);

      // Determine plane ID (1 for z > 0, 0 for z < 0 in local coordinates)
      unsigned planeID = (modulePosMother[2] > 0);

      // Determine module orientation: which way is the top (readout end)?
      bool top = (planeID == 1) ? (modulePosMother[1] > 0) : (modulePosMother[0] < 0);

      // Finally, what is the distance from the hit (centroid of the entry
      // and exit points) to the readout end?
      double x = (ide.entryX + ide.exitX) / 2;
      double y = (ide.entryY + ide.exitY) / 2;
      double z = (ide.entryZ + ide.exitZ) / 2;
      double world[3] = {x, y, z};
      double svHitPosLocal[3];
      adsGeo.WorldToLocal(world, svHitPosLocal);

      double poss[3];
      adsGeo.LocalToWorld(origin, poss);

      uint32_t moduleID = adsc.AuxDetID();
      uint32_t stripID = adsc.AuxDetSensitiveID();
      uint32_t channel0ID = 32 * moduleID + 2 * stripID + 0;
      uint32_t channel1ID = 32 * moduleID + 2 * stripID + 1;

      std::cout << "End Momentum: " << momentumEnd[0] << ", " << momentumEnd[1] << ", " << momentumEnd[2] << std::endl;
      std::cout << "= CRT HIT ===============================\n"
                << "Module " << moduleID << " / Strip " << stripID
                << " (ch " << channel0ID << ", " << channel1ID << ")" << "\n"
                << "Path: " << path << "\n"
                << "Global hit position: " << x << " " << y << " " << z << "\n"
                << "Strip origin: " << poss[0] << " " << poss[1] << " " << poss[2] << "\n"
                << "Module local position: " << modulePosMother[0] << " "
                                             << modulePosMother[1] << " "
                                             << modulePosMother[2] << " "
                << "(Plane " << planeID << ", side: " << (top ? "top" : "bottom") << ")\n"
                << "=========================================\n";
    }
  }

  // Loop through CRTData
  for (auto& crt : *crtData) {
    // Get IDs
    uint32_t gcid = crt.Channel();
    uint32_t channelID = gcid & 0x1;
    uint32_t stripID = (gcid>>1) & 0x1f / 2;
    uint32_t moduleID = gcid >> 5;
  momentumEnd = crt.Momentum();
  t0 = crt.T0();
    std::string name = geoService->AuxDet(moduleID).TotalVolume()->GetName();
    TVector3 pos = crt.HitPos(); //adGeoCore->AuxDetChannelToPosition(2*stripID, name);

  //  t0 = crt.T0();

    std::cout << "Test Print: " << t0 << std::endl;
    const geo::AuxDetSensitiveGeo& adGeo = adGeoCore->ChannelToAuxDetSensitive(name, 2*stripID+channelID);

    std::cout << "CRTData " << gcid << " " << channelID << " " << stripID << " " << moduleID << "\n";
    std::cout << "= CRTData ================================\n"
              << "Channel " << gcid << ": "
              << "Module " << moduleID << " / Strip " << stripID << " / Channel " << channelID << "\n"
              << "AD Name " << name << "\n"
              << "Position " << pos.X() << " " << pos.Y() << " " << pos.Z() << "\n"
              << "AD Name " << adGeo.TotalVolume()->GetName() << "\n"
              << "ADC " << crt.ADC() << ", CRTDump T0 " << crt.T0() << ", T1 " << crt.T1() << "\n"
              << "=========================================\n";

    volname = adGeo.TotalVolume()->GetName();
//    t0 = crt.T0();
    t = crt.Time();
    position = pos;
    channel = channelID;
    momentumEnd = crt.Momentum();
    hits->Fill();
  }

  // Get MCParticles
  art::Handle<std::vector<simb::MCParticle> > particles;
  e.getByLabel("largeant", particles);

  fout->cd();
  for (auto& p : *particles) {
    const simb::MCTrajectory& traj = p.Trajectory();
    for (size_t j=0; j<traj.size(); j++) {
      track->Fill(traj.X(j), traj.Y(j), traj.Z(j), traj.T(j), p.PdgCode(), traj.Momentum(j).Px(), traj.Momentum(j).Px(), traj.Momentum(j).Pz());
    } 
  }

  eventIndex++;
}

DEFINE_ART_MODULE(sbnd::crt::CRTDump)

