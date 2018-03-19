////////////////////////////////////////////////////////////////////////
/// \file  SingleGen_plugin.cc
/// \brief Generator for cosmic-rays
///
/// Module designed to produce a set list of particles for a MC event
///
/// \version $Id: SingleGen.cxx,v 1.4 2010/03/29 09:54:01 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef EVGEN_SINGLEFROMNUGEN
#define EVGEN_SINGLEFROMNUGEN

// C++ includes.
#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <memory>
#include <iterator>
#include <vector>


// Framework includes
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// art extensions
//#include "artextensions/SeedService/SeedService.hh"

// nutools includes
//#include "SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/MCTruth.h"
//#include "SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCParticle.h"
//#include "EventGeneratorBase/evgenbase.h"
#include "nutools/EventGeneratorBase/evgenbase.h"

// lar includes
//#include "Geometry/Geometry.h"
#include "larcore/Geometry/Geometry.h"
//#include "SummaryData/RunData.h"
#include "larcoreobj/SummaryData/RunData.h"

#include "TVector3.h"
#include "TDatabasePDG.h"
#include "TMath.h"

#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/RandGaussQ.h"

#include "TTree.h"
#include "TFile.h"

namespace simb { class MCTruth; }

namespace evgen {

  /// module to produce single or multiple specified particles in the detector
  class SingleFromNuGen : public art::EDProducer {

  public:
    explicit SingleFromNuGen(fhicl::ParameterSet const& pset);
    virtual ~SingleFromNuGen();

    // This is called for each event.
    void produce(art::Event& evt);
    void beginRun(art::Run& run);

  private:
    simb::MCTruth* fBranchPtr;
    TFile* fFile;
    TTree* fTree;
    std::vector<std::string> fFileList;
    std::vector<int> fPDGCodes;
    double fPThreshLow;
    double fPThreshHigh;
    int fFileIndex;
    int fEntryIndex;
  };
}

namespace evgen{

  //____________________________________________________________________________
  SingleFromNuGen::SingleFromNuGen(fhicl::ParameterSet const& pset):
    fFile(NULL),
    fTree(NULL),
    fFileList(pset.get<std::vector<std::string>>("InputFiles")),
    fPDGCodes(pset.get<std::vector<int>>("Particles")),
    fPThreshLow(pset.get<double>("PThreshLow",0.)),
    fPThreshHigh(pset.get<double>("PThreshLow",1E9)),
    fFileIndex(-1)
  {
    produces< std::vector<simb::MCTruth> >();
    //    produces< sumdata::RunData, art::InRun >();
  }

  //____________________________________________________________________________
  SingleFromNuGen::~SingleFromNuGen()
  {
  }

  //____________________________________________________________________________
  void SingleFromNuGen::beginRun(art::Run& run)
  {

    // grab the geometry object to see what geometry we are using
    //art::ServiceHandle<geo::Geometry> geo;
    //std::unique_ptr<sumdata::RunData> runcol(new sumdata::RunData(geo->DetectorName()));

    //run.put(std::move(runcol));

    return;
  }

  //____________________________________________________________________________
  void SingleFromNuGen::produce(art::Event& evt)
  {

    bool foundParticle=false;
    bool outOfEvents=false;

    while(!foundParticle&&!outOfEvents){
      std::cout<<"1"<<std::endl;
      //If we are at the end of a file, open the next one
      while(fFileIndex<0||fEntryIndex==fTree->GetEntries()){
	if(fFile){
	  fFile->Close();
	  delete fFile;
	  fFile=0;
	  fTree=0;
	  std::cout<<"1a"<<std::endl;
	}
	std::cout<<"2"<<std::endl;
	//If we have moved past the last file, flag that
	//we cannot provide any more events
	if(fFileIndex==(int)fFileList.size()-1){
	  std::cout<<"2a"<<std::endl;
	  outOfEvents=true;
	  break;
	}
	++fFileIndex;
	fBranchPtr=0;
	fEntryIndex=0;
	fFile=new TFile(fFileList[fFileIndex].c_str());
	fTree=(TTree*)fFile->Get("Particles");
	fTree->SetBranchAddress("MCTruth",&fBranchPtr);
	std::cout<<"2b"<<std::endl;
      }
      std::cout<<"3"<<std::endl;
      if(fTree){
	fTree->GetEntry(fEntryIndex++);
	int partPDG=fBranchPtr->GetParticle(0).PdgCode();
	double partMom=fBranchPtr->GetParticle(0).P();
	std::cout<<"4"<<std::endl;
	if(std::find(fPDGCodes.begin(),fPDGCodes.end(),partPDG)!=fPDGCodes.end()
	   &&partMom<fPThreshHigh&&partMom>fPThreshLow){
	  foundParticle=true;
	  std::cout<<"5"<<std::endl;
	  }
      }
      std::cout<<"6"<<std::endl;
    }

    ///unique_ptr allows ownership to be transferred to the art::Event after the put statement
    std::unique_ptr< std::vector<simb::MCTruth> > truthcol(new std::vector<simb::MCTruth>);
    if(foundParticle){
      truthcol->push_back(*fBranchPtr);
      std::cout<<"7"<<std::endl;
    }
    else{
      std::cout<<"Out of particles... will start producing empty events"<<std::endl;
    }
    //When we run out of data then the module will start producing empty events
    std::cout<<"9"<<std::endl;
    evt.put(std::move(truthcol));
    std::cout<<"10"<<std::endl;
    return;
  }


}//end namespace evgen

namespace evgen{

  DEFINE_ART_MODULE(SingleFromNuGen)

}//end namespace evgen

#endif
////////////////////////////////////////////////////////////////////////
