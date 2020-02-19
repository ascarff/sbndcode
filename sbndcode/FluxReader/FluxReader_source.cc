#include "art/Framework/Core/InputSourceMacros.h"
#include "art/Framework/IO/Sources/Source.h"
#include "sbndcode/FluxReader/FluxReader.h"

namespace sbnd {
  typedef art::Source<FluxReader> FluxReaderSource;
}

DEFINE_ART_INPUT_SOURCE(sbnd::FluxReaderSource)
