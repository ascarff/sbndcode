#ifndef CRTTOF_H
#define CRTTOF_H

#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/View.h"
//#include "canvas/Persistency/Common/Ptr.h"
//#include "canvas/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "larcore/Geometry/AuxDetGeometry.h"
#include "larcore/Geometry/Geometry.h"
#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"

#include <vector>
#include <iterator>
#include <string>

#include "TFile.h"
#include "TNtuple.h"

namespace crt {

  class CRTTOF : public art::EDAnalyzer {
    public:
      explicit CRTTOF(fhicl::ParameterSet const& pset);
      ~CRTTOF();

      void beginJob() {}
      void analyze(const art::Event& evt);
      void beginSubRun (const art::SubRun& subrun) {}
      void reset() {}

    private:
      std::string fLArG4ModuleLabel;
      TFile* f;
      TNtuple* nt;
  };


  CRTTOF::CRTTOF(fhicl::ParameterSet const& pset)
      : EDAnalyzer(pset),
        fLArG4ModuleLabel(pset.get<std::string>("G4ModuleLabel")) { 
    // FIXME: Use the TFileService
    f = new TFile("crt_hits.root", "recreate");
    nt = new TNtuple("nt", "", "dt:x1:y1:z1:edep1:x2:y2:z2:edep2");
  }


  CRTTOF::~CRTTOF() {
    f->cd();
    nt->Write();
    f->Close();
  }

  void CRTTOF::analyze(const art::Event& evt) {
    art::ServiceHandle<geo::Geometry> geoService;

    art::ServiceHandle<geo::AuxDetGeometry> adGeoService;
    const geo::AuxDetGeometry* adG = &(*adGeoService);  // wut!
    const geo::AuxDetGeometryCore* adGeoCore = adG->GetProviderPtr();

    std::cout << adGeoCore << std::endl;

    std::vector<const sim::AuxDetSimChannel*> auxDetSimChannels;
    evt.getView(fLArG4ModuleLabel, auxDetSimChannels);

    for (const sim::AuxDetSimChannel* adsc : auxDetSimChannels) {
      std::cout << adsc << std::endl;
    }

    //if (nAD < kMaxAuxDets) {
    //  fData->AuxDetID[iPart][nAD] = c->AuxDetID();
    //  fData->entryX[iPart][nAD]   = iIDE->entryX;
    //  fData->entryY[iPart][nAD]   = iIDE->entryY;
    //  fData->entryZ[iPart][nAD]   = iIDE->entryZ;
    //  fData->entryT[iPart][nAD]   = iIDE->entryT;
    //  fData->exitX[iPart][nAD]    = iIDE->exitX;
    //  fData->exitY[iPart][nAD]    = iIDE->exitY;
    //  fData->exitZ[iPart][nAD]    = iIDE->exitZ;
    //  fData->exitT[iPart][nAD]    = iIDE->exitT;
    //  fData->exitPx[iPart][nAD]   = iIDE->exitMomentumX;
    //  fData->exitPy[iPart][nAD]   = iIDE->exitMomentumY;
    //  fData->exitPz[iPart][nAD]   = iIDE->exitMomentumZ;
    //  fData->CombinedEnergyDep[iPart][nAD] = totalE;
    //}
    //++nAD;
    //} // for aux det sim channels
    //fData->NAuxDets[iPart] = nAD; 
  }

  DEFINE_ART_MODULE(CRTTOF)

}  // namespace sbnd

#endif  // CRTTOF_H

