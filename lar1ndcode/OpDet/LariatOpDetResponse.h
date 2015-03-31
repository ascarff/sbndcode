////////////////////////////////////////////////////////////////////////
// \file LariatOpDetResponse.h
//
// \based on  MicrobooneOpDetResponse.h -> brief service containing information about the response of optical detectors in Microboone
//
// \author ahimmel@phy.duke.edu
///modified by pkryczyn@fnal.gov
////////////////////////////////////////////////////////////////////////

#ifndef LARIAT_OPDET_RESPONSE_H
#define LARIAT_OPDET_RESPONSE_H

// LArSoft includes
#include "Simulation/SimPhotons.h"
#include "OpticalDetector/OpDetResponseInterface.h"
// ROOT includes
#include <TH1D.h>
#include <TF1.h>
#include <TTree.h>
#include <TVectorT.h>
#include <TAxis.h>
#include <TSpline.h>



namespace opdet
{
    class LariatOpDetResponse : public opdet::OpDetResponseInterface {
    public:

        LariatOpDetResponse(fhicl::ParameterSet const& pset, art::ActivityRegistry& reg);
        ~LariatOpDetResponse() throw();



    private:

        virtual void doReconfigure(fhicl::ParameterSet const& p);
        virtual bool doDetected(int OpChannel, const sim::OnePhoton& Phot, int &newOpChannel) const;
        virtual bool doDetectedLite(int OpChannel, int &newOpChannel) const;

      int fVerbosity;                // Level of output to write to std::out
	bool fNewDetectorApproach;//to be discarded once the new way of definind detectors is established
	int fDetectorNumber;//total number of photodetectors
	int fDetectorTypes;//number of different types of photodets
      bool fMakeDetectedPhotonsTree; //
      bool fMakeAllPhotonsTree;      //
      bool fMakeOpDetsTree;         // Switches to turn on or off each output
      bool fMakeOpDetEventsTree; 
	bool fDetailedQE;         //
      bool fFill;
      bool fUseQE;
      float fQE; 
      float fLariatfast; 
	mutable int typedet=0;	
	mutable double fftest=0.;              // Quantum efficiency of tube
      std::vector<double> fQE2H; 
      std::vector<int> fChannelTypes;
      std::vector<double> fQE2E;
      std::vector<double> fQE2Si;    
      std::vector<double> fQEn;                   // Quantum efficiency of tube
      std::vector<std::vector<double>> fDetEff;    
      std::vector<std::vector<double>> fEnEff;
	//Histrograms to store efficiency
	TH1D *QeEnergyHist;
	TH1D *QEHmm;
	TH1D *QEEtl;
	TH1D *QESipm;
	std::vector<TH1D *> QEDets;
      float fWavelengthCutLow;       // Sensitive wavelength range 
      float fWavelengthCutHigh;      // 
        


    }; // class LariatOpDetResponse

    
} //namespace opdet


DECLARE_ART_SERVICE_INTERFACE_IMPL(opdet::LariatOpDetResponse, opdet::OpDetResponseInterface, LEGACY)

#endif //LARIAT_OPDET_RESPONSE_H
