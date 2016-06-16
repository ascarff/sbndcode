////////////////////////////////////////////////////////////////////////////////
/// \file SBNDGeometryHelper_service.cc
///
/// \version $Id
/// \author  rs@fnal.gov
////////////////////////////////////////////////////////////////////////////////

// Migration note:
// Geometry --> sbnd/Geometry
#include "sbndcode/Geo/SBNDGeometryHelper.h"

#include "Geometry/ChannelMapAlg.h"

// Migration note:
// Geometry --> sbnd/Geometry for the two below
#include "sbndcode/Geo/ChannelMapsbndAlg.h"


#include "TString.h"


namespace sbnd
{

  SBNDGeometryHelper::SBNDGeometryHelper( fhicl::ParameterSet const & pset, art::ActivityRegistry & reg )
  :  fPset( pset ),
     fReg( reg ),
     fChannelMap()
  {}

  SBNDGeometryHelper::~SBNDGeometryHelper() throw()
  {}  
  
  void SBNDGeometryHelper::doConfigureChannelMapAlg( const TString & detectorName,
                                                     fhicl::ParameterSet const & sortingParam,
                                                     std::vector<geo::CryostatGeo*> & c,
					             std::vector<geo::AuxDetGeo*>   & ad  )
  {
    fChannelMap = nullptr;
    
  
      fChannelMap = std::shared_ptr<geo::ChannelMapAlg>( new geo::ChannelMapsbndAlg( sortingParam ) );
  
    if ( fChannelMap )
    {
      fChannelMap->Initialize( c, ad );
    }
  }
  
  
  std::shared_ptr<const geo::ChannelMapAlg> SBNDGeometryHelper::doGetChannelMapAlg() const
  {
    return fChannelMap;
  }

}

DEFINE_ART_SERVICE_INTERFACE_IMPL(sbnd::SBNDGeometryHelper, geo::ExptGeoHelperInterface)
