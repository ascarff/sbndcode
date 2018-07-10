#ifndef GoodHeader_h
#define GoodHeader_h

#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "fhiclcpp/ParameterSet.h"

namespace daqAnalysis {

class GoodHeader {
public:
  explicit GoodHeader(fhicl::ParameterSet const & p, art::ActivityRegistry & areg) {}
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  bool is_good_header;

};

}// end namespace

DECLARE_ART_SERVICE(daqAnalysis::GoodHeader, LEGACY)
#endif

