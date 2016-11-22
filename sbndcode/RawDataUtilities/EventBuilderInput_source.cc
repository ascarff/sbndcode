#ifndef EventBuilderInput_source
#define EventBuilderInput_source

/**
 * Source events from artdaq output ROOT files.
 *
 * Substantial portions ported from lariatsoft (Author: Johnny Ho)
 *
 * The class EventBuilder is to be used as the template parameter for
 * art::Source<T>. It understands how to read art's ROOT data files,
 * to extract artdaq::Fragments from them, how to convert the
 * artdaq::Fragments into vectors of raw data objects, and
 * finally how to re-combine those raw data objects to present
 * the user of the art::Source<EventBuilder> with a sequence of
 * art::Events that have the desired event structure, different from
 * the event structure of the data file(s) being read.
 *
 * A. Mastbaum <mastbaum@uchicago.edu>, 2016/11/09
 */

// art includes
#include "art/Framework/Core/FileBlock.h"
#include "art/Framework/Core/InputSourceMacros.h"
#include "art/Framework/Core/ProductRegistryHelper.h"
#include "art/Framework/IO/Root/rootNames.h"
#include "art/Framework/IO/Sources/Source.h"
#include "art/Framework/IO/Sources/SourceHelper.h"
#include "art/Framework/IO/Sources/SourceTraits.h"
#include "art/Framework/IO/Sources/put_product_in_principal.h"
#include "art/Framework/Principal/EventPrincipal.h"
#include "art/Framework/Principal/RunPrincipal.h"
#include "art/Framework/Principal/SubRunPrincipal.h"
#include "art/Persistency/Provenance/EventID.h"
#include "art/Persistency/Provenance/MasterProductRegistry.h"
#include "art/Persistency/Provenance/RunID.h"
#include "art/Persistency/Provenance/SubRunID.h"
#include "art/Utilities/InputTag.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// artdaq includes
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/Fragments.hh"

// LArSoft includes
#include "lardata/RawData/AuxDetDigit.h"
#include "larcore/SimpleTypesAndConstants/RawTypes.h"
#include "larcore/SummaryData/RunData.h"

// sbndcode includes
#include "CAENFragment.hh"

// ROOT includes
#include "TBranch.h"
#include "TFile.h"
#include "TTree.h"

// C++ includes
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

// N.B. Currently CAENFragment is in the sbndcode source, copied from that in
// sbnddaq. This should be moved to a new product ("sbndfragments") on which
// both sbndcode and sbnddaq depend.
using sbnddaq::CAENFragment;
namespace rdu {
  static const size_t kMaxChannelsV1730 = 16;
}

//-------------------------------------------------------------------------
namespace {

  // Retrieves branch name (a la art convention) where object resides
  template <typename PROD>
  const char* getBranchName(art::InputTag const& tag) {
    std::ostringstream oss;
    oss << art::TypeID(typeid(PROD)).friendlyClassName()
        << '_'
        << tag.label()
        << '_'
        << tag.instance()
        << '_'
        << tag.process()
        << ".obj";
    return oss.str().data();
  }

  artdaq::Fragments* getFragments(TBranch* br, unsigned entry) {
    br->GetEntry(entry);
    return reinterpret_cast<artdaq::Fragments*>(br->GetAddress());
  }

}


//-------------------------------------------------------------------------
namespace rdu {

  class EventBuilder {

    public:

      // Constructor and destructor.
      explicit EventBuilder(fhicl::ParameterSet const& pset,
                            art::ProductRegistryHelper& prhelper,
                            art::SourceHelper& shelper);

      // Open the file of the given name, returning a new FileBlock.
      // If readFile is unable to return a valid FileBlock it should throw.
      bool readFile(std::string const& filename, art::FileBlock*& fileblock);

      // Read the next part of the current file. Return false if nothing
      // was read; return true and set the appropriate 'out' arguments if
      // something was read.
      bool readNext(art::RunPrincipal* const& inRun,
                    art::SubRunPrincipal* const& inSubRun,
                    art::RunPrincipal*& outRun,
                    art::SubRunPrincipal*& outSubRun,
                    art::EventPrincipal*& outEvent);

      // Close the current file.
      void closeCurrentFile();

      // Read in any parameters from the .fcl files.
      void reconfigure(fhicl::ParameterSet const& pset);

    private:

      std::string fSourceName;
      std::unique_ptr<TFile> fFile;
      bool fDoneWithFile;
      art::InputTag fInputTag;
      art::SourceHelper fSourceHelper;
      TTree* fEventTree;
      TBranch* fFragmentsBranch;
      TBranch* fEventAuxBranch;
      size_t fNumberInputEvents;
      art::RunNumber_t fRunNumber;
      art::SubRunNumber_t fSubRunNumber;
      art::EventNumber_t fEventNumber;
      art::RunNumber_t fCachedRunNumber;
      art::SubRunNumber_t fCachedSubRunNumber;

      // EventAuxiliary for fetching run and sub-run numbers
      art::EventAuxiliary fEventAux;

  }; // class EventBuilder


  //-----------------------------------------------------------------------
  EventBuilder::EventBuilder(fhicl::ParameterSet const& pset,
                             art::ProductRegistryHelper& prhelper,
                             art::SourceHelper& shelper)
      : fSourceName("daq")
      , fFile()
      , fDoneWithFile(false)
      , fInputTag("daq:V1730:DAQ")
      , fSourceHelper(shelper)
      , fFragmentsBranch(nullptr)
      , fEventAuxBranch(nullptr)
      , fNumberInputEvents()
      , fRunNumber(1)
      , fSubRunNumber(0)
      , fEventNumber(0)
      , fCachedRunNumber(-1)
      , fCachedSubRunNumber(-1) {
    // Configure from FHICL parameters
    this->reconfigure(pset);

    // Define which data products this source sources
    prhelper.reconstitutes<std::vector<raw::AuxDetDigit>, art::InEvent>(fSourceName);
  }

  //-----------------------------------------------------------------------
  void EventBuilder::reconfigure(fhicl::ParameterSet const& pset) {
    fSourceName = pset.get<std::string>("SourceName", "daq");
    fInputTag = pset.get<std::string>("InputTag", "daq:V1730:DAQ");
  }

  //-----------------------------------------------------------------------
  bool EventBuilder::readFile(std::string const& filename,
                              art::FileBlock*& fileblock) {
    // Get artdaq::Fragments branch
    fFile.reset(new TFile(filename.data()));
    fEventTree = \
      reinterpret_cast<TTree*>(fFile->Get(art::rootNames::eventTreeName().c_str()));

    if (fEventTree == nullptr) {
      throw cet::exception("EventBuilder::readFile")
          << "Unable to load tree " << art::rootNames::eventTreeName() << " "
          << "from file " << filename << "\n";
    }

    // Get branch for specific input tag
    fFragmentsBranch = \
      fEventTree->GetBranch(getBranchName<artdaq::Fragments>(fInputTag));

    if (fFragmentsBranch == nullptr) {
      throw cet::exception("EventBuilder::readFile")
          << "Unable to load branch "
          << getBranchName<artdaq::Fragments>(fInputTag) << " "
          << "for input tag " << fInputTag << "\n";
    }

    std::cout << "fFragmentsBranch ("
              << static_cast<const void*>(fFragmentsBranch)
              << "): "
              << fFragmentsBranch->GetEntries()
              << " entries" << std::endl;

    fNumberInputEvents = static_cast<size_t>(fFragmentsBranch->GetEntries());

    // Auxiliary event information
    fEventAuxBranch = fEventTree->GetBranch("EventAuxiliary");
    if (fEventAuxBranch == nullptr) {
      throw cet::exception("EventBuilder::readFile")
          << "Unable to load EventAuxiliary branch\n";
    }

    art::EventAuxiliary* eventAuxPtr = &fEventAux;
    fEventAuxBranch->SetAddress(&eventAuxPtr);
    fEventAuxBranch->GetEntry(0);

    fRunNumber = fEventAux.run();
    fSubRunNumber = fEventAux.subRun();

    LOG_VERBATIM("EventBuilderInput")
        << "\n////////////////////////////////////"
        << "\nfRunNumber ....... " << fRunNumber
        << "\nfSubRunNumber .... " << fSubRunNumber
        << "\nfCachedRunNumber . " << fCachedRunNumber
        << "\nfNumberInputEvents " << fNumberInputEvents
        << "\n////////////////////////////////////\n";

    // New fileblock
    fileblock = new art::FileBlock(art::FileFormatVersion(), filename);
    if (fileblock == nullptr) {
      throw art::Exception(art::errors::FileOpenError)
          << "Unable to open file " << filename << ".\n";
    }

    return true;
  }

  //-----------------------------------------------------------------------
  bool EventBuilder::readNext(art::RunPrincipal* const& inRun,
                              art::SubRunPrincipal* const& inSubRun,
                              art::RunPrincipal*& outRun,
                              art::SubRunPrincipal*& outSubRun,
                              art::EventPrincipal*& outEvent) {
    if (fDoneWithFile) {
      return false;
    }

    LOG_VERBATIM("EventBuilderInput")
        << "Processing event index "
        << fEventNumber << "/" << fNumberInputEvents << std::endl;

    artdaq::Fragments* fragments = \
      getFragments(fFragmentsBranch, fEventNumber);

    // Output: stash waveforms in AuxDetDigits for now
    std::vector<raw::AuxDetDigit> auxDigits;

    // Loop through boards
    unsigned board_id = 0;  // FIXME: Board ID should be in the header

    for (auto& fragment : *fragments) {
      // Fragment::size is size in units of Fragment::size_type!
      size_t dataBytes = fragment.dataSize() * sizeof(artdaq::Fragment::size_type);
      CAENFragment caenFrag((char*)fragment.dataAddress(), dataBytes);

      if (caenFrag.header.nChannels != caenFrag.waveForms.size()) {
        cet::exception("EventBuilder::readNext")
            << "CAEN header and data disagree on channel count ("
            << caenFrag.header.nChannels << " vs. "
            << caenFrag.waveForms.size() << ")\n";
      }

      // Determine which channels are populated based on the channel mask
      std::vector<size_t> channelMap;
      for (size_t ich=0; ich<kMaxChannelsV1730; ich++) {
        if (caenFrag.header.channelMask & (1<<ich)) {
          channelMap.push_back(ich);
        }
      }

      if (channelMap.size() != caenFrag.waveForms.size()) {
        cet::exception("EventBuilder::readNext")
            << "CAEN channel mask and data disagree on channel count ("
            << channelMap.size() << " vs. "
            << caenFrag.waveForms.size() << ")\n";
      }

      // Loop through waveforms for this board
      for (size_t ich=0; ich<channelMap.size(); ich++) {
        // Get the channel ID
        unsigned short channel = channelMap[ich];

        // Construct a name for this board
        std::ostringstream oss;
        oss << "board" << board_id;

        // Copy the data
        std::vector<short> waveform(caenFrag.waveForms[ich].data.begin(),
                                    caenFrag.waveForms[ich].data.end());

        // Add the AuxDetDigit
        auxDigits.push_back(raw::AuxDetDigit(channel, waveform, oss.str(),
                                             caenFrag.header.triggerTimeTag));
      }

      board_id++;
    }

    art::Timestamp timestamp;  // FIXME

    if (fRunNumber != fCachedRunNumber) {
      outRun = fSourceHelper.makeRunPrincipal(fRunNumber, timestamp);
      fCachedRunNumber = fRunNumber;
      fEventNumber = 0ul;
    }

    if (fSubRunNumber != fCachedSubRunNumber) {
      outSubRun = fSourceHelper.makeSubRunPrincipal(fRunNumber, fSubRunNumber, timestamp);
      fCachedSubRunNumber = fSubRunNumber;
    }

    outEvent = fSourceHelper.makeEventPrincipal(
        fRunNumber, fSubRunNumber, fEventNumber, art::Timestamp());


    art::put_product_in_principal(
        std::make_unique<std::vector<raw::AuxDetDigit> >(auxDigits),
        *outEvent,
        fSourceName);

    if (++fEventNumber == fNumberInputEvents) {
      fDoneWithFile = true;
    }

    return true;
  }

  //-----------------------------------------------------------------------
  void EventBuilder::closeCurrentFile() {
    fFile.reset(nullptr);
  }

  DEFINE_ART_INPUT_SOURCE(art::Source<rdu::EventBuilder>)

}  // namespace rdu

#endif  // EventBuilderInput_source

