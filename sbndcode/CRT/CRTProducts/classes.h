#include "canvas/Persistency/Common/Wrapper.h"
#include "CRTData.hh"
#include "CRTHit.hh"
#include <vector>

template class art::Wrapper<sbnd::crt::CRTData>;
template class std::vector<sbnd::crt::CRTData>;
template class art::Wrapper<std::vector<sbnd::crt::CRTData> >;

template class art::Wrapper<sbnd::crt::CRTHit>;
template class std::vector<sbnd::crt::CRTHit>;
template class art::Wrapper<std::vector<sbnd::crt::CRTHit> >;



