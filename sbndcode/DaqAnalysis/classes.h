#include "canvas/Persistency/Common/Wrapper.h"
#include <vector>
#include "sbndcode/DaqAnalysis/ChannelData.hh"

namespace {
  struct dictionary {
    daqAnalysis::ChannelData c;
    std::vector<daqAnalysis::ChannelData> c_v;
    art::Wrapper<daqAnalysis::ChannelData> c_w;
    art::Wrapper<std::vector<daqAnalysis::ChannelData>> c_v_w;
  };
}


