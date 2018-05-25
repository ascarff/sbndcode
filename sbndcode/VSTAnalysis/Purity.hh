#ifndef VSTPURITY_H_SEEN
#define VSTPURITY_H_SEEN


///////////////////////////////////////////////
// Purity.h
//
// Functions for VST online purity analysis
///////////////////////////////////////////////

// framework
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

// LArSoft 
#include "lardataobj/RecoBase/Hit.h"
#include "larcore/Geometry/Geometry.h"

// c++
#include <vector>

// ROOT
#include "TTree.h"
#include "TGraph.h"
#include "TCanvas.h"

namespace daqAnalysis{
  double CalculateLifetime(std::vector<art::Ptr<recob::Hit>> rawhits); //Calculate electron lifetime from collection of hits
}

#endif

