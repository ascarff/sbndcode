#include "sbndcode/OpDetSim/sbndPDMapAlg.h"


//------------------------------------------------------------------------------
//--- opdet::sbndPDMapAlg implementation
//------------------------------------------------------------------------------

namespace opdet {

  sbndPDMapAlg::sbndPDMapAlg()
  {
    std::string fname;
    cet::search_path sp("FW_SEARCH_PATH");
    sp.find_file("sbnd_pds_mapping.json", fname);
    std::ifstream i(fname);
    i >> PDmap;
  }

  sbndPDMapAlg::~sbndPDMapAlg()
  { }

  bool sbndPDMapAlg::isPDType(size_t ch, std::string pdname) const
  {
    if(PDmap.at(ch)["pd_type"] == std::string(pdname)) return true;
    return false;
  }

  std::string sbndPDMapAlg::pdType(size_t ch) const
  {
    if(ch < PDmap.size()) return PDmap.at(ch)["pd_type"];
    return "There is no such channel";
  }

  size_t sbndPDMapAlg::size() const
  {
    return PDmap.size();
  }

  auto sbndPDMapAlg::getChannelEntry(size_t ch) const
  {
    return PDmap.at(ch);
  }

  // Look in the header for the implementation:
  // template<typename T>
  // nlohmann::json sbndPDMapAlg::getCollectionWithProperty(std::string property, T property_value)

}
