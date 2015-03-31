////////////////////////////////////////////////////////////////////////
// \file LAr1NDOpDetResponse.h
//
// \brief service containing information about the response of optical detectors in LAr1ND
//
// \author ahimmel@phy.duke.edu
//
////////////////////////////////////////////////////////////////////////


#ifndef LAR1ND_OPDET_RESPONSE_H
#define LAR1ND_OPDET_RESPONSE_H

// LArSoft includes
#include "Simulation/SimPhotons.h"
#include "OpticalDetector/OpDetResponseInterface.h"



namespace opdet
{
    class LAr1NDOpDetResponse : public opdet::OpDetResponseInterface {
    public:

        LAr1NDOpDetResponse(fhicl::ParameterSet const& pset, art::ActivityRegistry& reg);
        ~LAr1NDOpDetResponse() throw();



    private:

        virtual void doReconfigure(fhicl::ParameterSet const& p);
        virtual bool doDetected(int OpChannel, const sim::OnePhoton& Phot, int &newOpChannel) const;
        virtual bool doDetectedLite(int OpChannel, int &newOpChannel) const;

        float fQE;                     // Quantum efficiency of tube
        
        float fWavelengthCutLow;       // Sensitive wavelength range 
        float fWavelengthCutHigh;      // 
        


    }; // class LAr1NDOpDetResponse

    
} //namespace opdet


DECLARE_ART_SERVICE_INTERFACE_IMPL(opdet::LAr1NDOpDetResponse, opdet::OpDetResponseInterface, LEGACY)

#endif //LAR1ND_OPDET_RESPONSE_H
