////////////////////////////////////////////////////////////////////////
//
//  \file LAr1NDOpDetResponse_service.cc
//
////////////////////////////////////////////////////////////////////////


#include "lar1ndcode/OpDet/LAr1NDOpDetResponse.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "CLHEP/Random/RandFlat.h"


namespace opdet{


    //--------------------------------------------------------------------
    LAr1NDOpDetResponse::LAr1NDOpDetResponse(fhicl::ParameterSet const& pset, 
                                         art::ActivityRegistry &/*reg*/)
    {
        this->doReconfigure(pset);
    }
    
    //--------------------------------------------------------------------
    LAr1NDOpDetResponse::~LAr1NDOpDetResponse() throw()
    { }


    //--------------------------------------------------------------------
    void LAr1NDOpDetResponse::doReconfigure(fhicl::ParameterSet const& pset)
    {
        fQE=                       pset.get<double>("QuantumEfficiency");
        fWavelengthCutLow=         pset.get<double>("WavelengthCutLow");
        fWavelengthCutHigh=        pset.get<double>("WavelengthCutHigh");
    }


    //--------------------------------------------------------------------
    bool LAr1NDOpDetResponse::doDetected(int OpChannel, const sim::OnePhoton& Phot, int &newOpChannel) const
    {
        newOpChannel = OpChannel;
        
        // Check QE
        if ( CLHEP::RandFlat::shoot(1.0) > fQE ) return false;

        double wavel = wavelength(Phot.Energy);
        // Check wavelength acceptance
        if (wavel < fWavelengthCutLow) return false;
        if (wavel > fWavelengthCutHigh) return false;

        return true;
    }
    
    //--------------------------------------------------------------------
    bool LAr1NDOpDetResponse::doDetectedLite(int OpChannel, int &newOpChannel) const
    {
        newOpChannel = OpChannel;
        
        // Check QE
        if ( CLHEP::RandFlat::shoot(1.0) > fQE ) return false;

        return true;
    }



} // namespace

DEFINE_ART_SERVICE_INTERFACE_IMPL(opdet::LAr1NDOpDetResponse, opdet::OpDetResponseInterface)

