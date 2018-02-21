//////////////////////////////////////////////////////////////////
//
//  GenieActiveFilter
//
//  @author: Corey Adams, corey.adams@fas.harvard.edu
//
//  Design:  This module is very simple.  Since SBND has two TPCs,
//           to generate events in any active volume requires
//           generation in the volCryostat.  This module allows
//           to select only events within the active volume
//           and an optional (default 0) fiducial volume cut.
//
//////////////////////////////////////////////////////////////////
#include <algorithm>
#include <iostream>

#include "TGeoManager.h"

#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "larcore/Geometry/Geometry.h"
#include "nusimdata/SimulationBase/MCTruth.h"



namespace filt {

class GenieActiveFilter : public art::EDFilter {
 public:
  explicit GenieActiveFilter(fhicl::ParameterSet const& pset);
  virtual bool filter(art::Event& e) override;
  void reconfigure(fhicl::ParameterSet const& pset);
  void beginJob();

 private:
  std::string fGenieProducer;

  float fHighYFiducialCut;
  float fLowYFiducialCut;
  float fHighXFiducialCut;
  float fLowXFiducialCut;
  float fHighZFiducialCut;
  float fLowZFiducialCut;
};

GenieActiveFilter::GenieActiveFilter(fhicl::ParameterSet const& pset) {
  this->reconfigure(pset);
}

void GenieActiveFilter::reconfigure(fhicl::ParameterSet const& pset) {
  fHighXFiducialCut = pset.get<float>("HighXFiducialCut", 0.0);
  fLowXFiducialCut  = pset.get<float>("LowXFiducialCut", 0.0);

  fHighYFiducialCut = pset.get<float>("HighYFiducialCut", 0.0);
  fLowYFiducialCut  = pset.get<float>("LowYFiducialCut", 0.0);

  fHighZFiducialCut = pset.get<float>("HighZFiducialCut", 0.0);
  fLowZFiducialCut  = pset.get<float>("LowZFiducialCut", 0.0);

  fGenieProducer    = pset.get<std::string>("GenieProducer", "generator");
}

bool GenieActiveFilter::filter(art::Event& e) {
  // Get the neutrino data:

  art::InputTag neutrino_tag(fGenieProducer);


  // Get the coordinates of the Active volume:
  art::ServiceHandle<geo::Geometry> geom;

  int x_min = -2 * geom -> DetHalfWidth() + fHighXFiducialCut;
  int x_max =  2 * geom -> DetHalfWidth() - fLowXFiducialCut;

  int y_min =    - geom -> DetHalfHeight() + fHighYFiducialCut;
  int y_max =      geom -> DetHalfHeight() - fLowYFiducialCut;
  int z_min =  0 + fHighZFiducialCut;
  int z_max =      geom -> DetLength() - fLowZFiducialCut;


  std::cout << "geometry coordinates: \n"
            << "  X - (" << x_min << ", " << x_max << ")\n"
            << "  Y - (" << y_min << ", " << y_max << ")\n"
            << "  Z - (" << z_min << ", " << z_max << ")\n";


  art::Handle<std::vector<simb::MCTruth> > mctruth;
  if (!e.getByLabel(neutrino_tag, mctruth)) {
    return false;
  }
  auto & neutrino = mctruth->front().GetNeutrino().Nu();

  if (neutrino.Vx() < x_min || neutrino.Vx() > x_max){
    std::cout << "  X failed: " << neutrino.Vx() << std::endl;
    return false;
  }
  if (neutrino.Vy() < y_min || neutrino.Vy() > y_max){
    std::cout << "  Y failed: " << neutrino.Vy() << std::endl;
    return false;
  }
  if (neutrino.Vz() < z_min || neutrino.Vz() > z_max){
    std::cout << "  Z failed: " << neutrino.Vz() << std::endl;
    return false;
  }
  return true;
}

void GenieActiveFilter::beginJob() {}

DEFINE_ART_MODULE(GenieActiveFilter)
}
