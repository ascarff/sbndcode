// -*- mode: c++; c-basic-offset: 4; -*-
////////////////////////////////////////////////////////////////////////
//
//  \file DoubleEndOpDetResponse_service.cc
//
////////////////////////////////////////////////////////////////////////


#include "lar1ndcode/OpticalDetector/DoubleEndOpDetResponse.h"
#include "TGeoNode.h"
#include "TGeoBBox.h"
#include "Geometry/OpDetGeo.h"
//#include "Utilities/LArProperties.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "CLHEP/Random/RandFlat.h"


namespace opdet{


    //--------------------------------------------------------------------
    DoubleEndOpDetResponse::DoubleEndOpDetResponse(fhicl::ParameterSet const& pset, 
                                         art::ActivityRegistry &/*reg*/)
    {
        this->doReconfigure(pset);
    }
    
    //--------------------------------------------------------------------
    DoubleEndOpDetResponse::~DoubleEndOpDetResponse() throw()
    { }


    //--------------------------------------------------------------------
    void DoubleEndOpDetResponse::doReconfigure(fhicl::ParameterSet const& pset)
    {
        double tempfQE =         pset.get<double>("QuantumEfficiency");
        fWavelengthCutLow =      pset.get<double>("WavelengthCutLow");
        fWavelengthCutHigh =     pset.get<double>("WavelengthCutHigh");
        fLightGuideAttenuation = pset.get<bool>("LightGuideAttenuation");
        lambda =                 pset.get<double>("Lambda");
        fChannelConversion =     pset.get<std::string>("ChannelConversion");
        std::string tmpAxis =    pset.get<std::string>("LongAxis"); 

        boost::algorithm::to_lower(tmpAxis);

        if (tmpAxis == "x") fLongAxis = 0;
        if (tmpAxis == "y") fLongAxis = 1;
        if (tmpAxis == "z") fLongAxis = 2;
        
        // Only allow channel conversion once - so it must be set to happen
        // either during full simulation (library generation) or during
        // fast simulation (library use).
        
        boost::algorithm::to_lower(fChannelConversion);
        
        fFullSimChannelConvert = false;
        fFastSimChannelConvert = false;
        
        if (fChannelConversion == "full") fFullSimChannelConvert = true;
        if (fChannelConversion == "fast") fFastSimChannelConvert = true;

        // Correct out the prescaling applied during simulation
        //art::ServiceHandle<util::LArProperties>   LarProp;
        //fQE = tempfQE / LarProp->ScintPreScale();
        fQE = tempfQE;

        /*
        if (fQE > 1.0001 ) {
            mf::LogError("DoubleEndOpDetResponse_service") << "Quantum efficiency set in OpDetResponse_service, " << tempfQE
                                                      << " is too large.  It is larger than the prescaling applied during simulation, "
                                                      << LarProp->ScintPreScale()
                                                      << ".  Final QE must be equalt to or smaller than the QE applied at simulation time.";
            assert(false);
        }
        */
    }


    //--------------------------------------------------------------------
    int  DoubleEndOpDetResponse::doNOpChannels() const
    {
        art::ServiceHandle<geo::Geometry> geom;
        //if (fFastSimChannelConvert || fFullSimChannelConvert)
        return geom->NOpChannels();
        //else
        //    return geom->NOpDets();

    }


    //--------------------------------------------------------------------
    bool DoubleEndOpDetResponse::doDetected(int OpDet, const sim::OnePhoton& Phot, int &newOpChannel) const
    {
        
        // Find the Optical Detector using the geometry service
        art::ServiceHandle<geo::Geometry> geom;


        if (fFullSimChannelConvert){
            // Override default number of channels for Fiber and Plank
            //float NOpHardwareChannels = geom->NOpHardwareChannels(OpDet);
            //float NOpHardwareChannels = 12;
            //int hardwareChannel = (int) ( CLHEP::RandFlat::shoot(1.0) * NOpHardwareChannels );
            //newOpChannel = geom->OpChannel(OpDet, hardwareChannel);
            newOpChannel = OpDet;
        }
        else{
            newOpChannel = OpDet;
        }
        
        // Check QE
        if ( CLHEP::RandFlat::shoot(1.0) > fQE ) return false;

        double wavel = wavelength(Phot.Energy);
        // Check wavelength acceptance
        if (wavel < fWavelengthCutLow) return false;
        if (wavel > fWavelengthCutHigh) return false;

        if (fLightGuideAttenuation) {
            // Get the length of the photon detector
            unsigned int cryostatID=0, opdetID=0;
            art::ServiceHandle<geo::Geometry> geom;
            geom->OpChannelToCryoOpDet(OpDet, opdetID, cryostatID);
            const TGeoNode* node = geom->Cryostat(cryostatID).OpDet(opdetID).Node();

            //const TGeoNode* node = geom->//geom->OpDetGeoFromOpDet(OpDet).Node();
            
            TGeoBBox *box = (TGeoBBox*)node->GetVolume()->GetShape();
            double opdetLength = 0;
            double sipmDistance = 0;

            if (fLongAxis == 0) {
                opdetLength = box->GetDX();
                sipmDistance = opdetLength - Phot.FinalLocalPosition.x();
            }
            else if (fLongAxis == 1) {
                opdetLength = box->GetDY();
                sipmDistance = opdetLength - Phot.FinalLocalPosition.y();
            }
            else if (fLongAxis == 2) {
                opdetLength = box->GetDZ();
                sipmDistance = opdetLength - Phot.FinalLocalPosition.z();
            }
            else {
                mf::LogError("DoubleEndOpDetResponse") << "Unknown axis, fLongAxis = " << fLongAxis;
                assert(false);
            }



            double altDistance = opdetLength - sipmDistance;
            double frac        = 0.5;

            // Throw away some photons based on attenuation
            double AttenuationProb = frac*exp(-sipmDistance/lambda) + frac*exp(-altDistance/lambda);

            
            //mf::LogVerbatim("DoubleEndOpDetResponse") << "OpDet: " << OpDet 
            //                                     << " has length " << opdetLength << " in detector "
            //                                     << box->GetDX() << " x " << box->GetDY()  << " x " << box->GetDZ();
            //mf::LogVerbatim("DoubleEndOpDetResponse") << "   Local Position = (" << Phot.FinalLocalPosition.x() 
            //                                     << ", " << Phot.FinalLocalPosition.y() << ", " << Phot.FinalLocalPosition.z() << ")";
            //mf::LogVerbatim("DoubleEndOpDetResponse") << "   Distance to SiPM = " << sipmDistance << " along axis " << fLongAxis;
            //mf::LogVerbatim("DoubleEndOpDetResponse") << "   Attenuation Probability = " << AttenuationProb;
            
            if ( CLHEP::RandFlat::shoot(1.0) > AttenuationProb ) return false;
            
          
        }

        return true;
    }

    //--------------------------------------------------------------------
    bool DoubleEndOpDetResponse::doDetectedLite(int OpDet, int &newOpChannel) const
    {
        if (fFastSimChannelConvert){

            // Find the Optical Detector using the geometry service
            //art::ServiceHandle<geo::Geometry> geom;
            // Here OpDet must be opdet since we are introducing
            // channel mapping here.
            //float NOpHardwareChannels = geom->NOpHardwareChannels(OpDet);
            //int hardwareChannel = (int) ( CLHEP::RandFlat::shoot(1.0) * NOpHardwareChannels );
            //newOpChannel = geom->OpChannel(OpDet, hardwareChannel);
            newOpChannel = OpDet;
        }
        else{
            newOpChannel = OpDet;
        }
        
        // Check QE
        if ( CLHEP::RandFlat::shoot(1.0) > fQE ) return false;

        
        return true;
    }

} // namespace

DEFINE_ART_SERVICE_INTERFACE_IMPL(opdet::DoubleEndOpDetResponse, opdet::OpDetResponseInterface)

