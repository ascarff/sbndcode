////////////////////////////////////////////////////////////////////////
//
//  \file LariatOpDetResponse_service.cc
// /modified by pkryczyn@fnal.gov
////////////////////////////////////////////////////////////////////////


#include "lar1ndcode/OpDet/LariatOpDetResponse.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "CLHEP/Random/RandFlat.h"


namespace opdet{


    //--------------------------------------------------------------------
    LariatOpDetResponse::LariatOpDetResponse(fhicl::ParameterSet const& pset, 
                                         art::ActivityRegistry &/*reg*/)
    {
        this->doReconfigure(pset);
    }
    
    //--------------------------------------------------------------------
    LariatOpDetResponse::~LariatOpDetResponse() throw()
    { }


    //--------------------------------------------------------------------
    void LariatOpDetResponse::doReconfigure(fhicl::ParameterSet const& pset)
    {
    fDetailedQE=       pset.get<bool>("QuantumEfficiencyDetails");
  //  fUseQE=       pset.get<bool>("UsePMTEff");
    fQE=                       pset.get<double>("QuantumEfficiency");
    fLariatfast=           pset.get<bool>("LariatFast");
    //channel type map
	if(fDetailedQE){


    fQE2H=                       pset.get< std::vector<double> >("QuantumEfficiencyVectorHmm");
    fQE2E=                       pset.get< std::vector<double> >("QuantumEfficiencyVectorEtl");
    fQE2Si=                       pset.get< std::vector<double> >("QuantumEfficiencyVectorSi"); 
    fQEn=                       pset.get< std::vector<double> >("QuantumEfficiencyEnergies");
	//Double_t qen[10];
	TVectorT<double> qen2(10);
	for(int i=0;i<int(fQEn.size());i++) qen2[i]=fQEn[i];
	
	

	QeEnergyHist=new TH1D(qen2);
	QEHmm=new TH1D(qen2);
	QEEtl=new TH1D(qen2);
	QESipm=new TH1D(qen2);

	for(int j=0;j<10;j++) {
	QEHmm->SetBinContent(j,fQE2H[j]);
	QEEtl->SetBinContent(j,fQE2E[j]);
	QESipm->SetBinContent(j,fQE2Si[j]);
	
	}

		//if(fNewDetectorApproach){
   			fDetectorNumber=                pset.get<int>("DetectorNumber", 1);
    			fDetectorTypes=     pset.get<int>("DetectorTypes", 1);
    			fChannelTypes=   pset.get< std::vector<int> >("ChannelTypes");
    			fDetEff=   pset.get< std::vector<std::vector<double>> >("QuantumEfficiencyVector");
    			fEnEff=    pset.get< std::vector<std::vector<double>> >("QuantumEfficiencyEnergiesVector");
				std::vector<TVectorT<double>> qen3;
				qen3.resize(fDetectorTypes);


				for(int j=0;j<fDetectorTypes;j++){
					for(int jj=0;jj<int(fEnEff[j].size());jj++){
						if(jj==0) qen3[j].ResizeTo(int(fEnEff[j].size()));
						qen3[j][jj]=fEnEff[j][jj];
						std::cout<<"setting energy "<<qen3[j][jj]<<std::endl;
						}
					}
	


				for(int j=0;j<fDetectorTypes;j++) {
					QEDets.push_back(new TH1D(qen3[j]));
					for(int ii=0;ii<int(fDetEff[j].size());ii++){
						QEDets[j]->SetBinContent(ii,fDetEff[j][ii]);
					}
				}

			//}//new detector approach - discard the previous one and replace with this after the new way of accessing data will work

	}//detailed QE
    fWavelengthCutLow=         pset.get<double>("WavelengthCutLow");
    fWavelengthCutHigh=         pset.get<double>("WavelengthCutHigh");
std::cout<<"reconfigured opdetresponse for lariat, got "<<fDetectorNumber<<" detectors of "<<fDetectorTypes<<" types "<<fDetailedQE<<" "<<std::endl;
    }


    //--------------------------------------------------------------------
    bool LariatOpDetResponse::doDetected(int OpChannel, const sim::OnePhoton& Phot, int &newOpChannel) const
    {
        newOpChannel = OpChannel;
        bool isdetect = true;
        // Check QE
	if(fLariatfast){
	std::cout<<"lariatopdet response running:) "<<std::endl;			
	fftest=double(CLHEP::RandFlat::shoot(1.0));
	typedet=fChannelTypes[OpChannel];
                         
        TSpline5 *splineeff=new TSpline5(QEDets[typedet]);

        if(fftest>splineeff->Eval(Phot.Energy/0.000001)) isdetect = false;
	delete splineeff;


        double wavel = wavelength(Phot.Energy);
        // Check wavelength acceptance
        if (wavel < fWavelengthCutLow) isdetect = false;
        if (wavel > fWavelengthCutHigh) isdetect = false;

        
	}
	else isdetect = true;
	return isdetect;
    }
    
    //--------------------------------------------------------------------
    bool LariatOpDetResponse::doDetectedLite(int OpChannel, int &newOpChannel) const
    {
        newOpChannel = OpChannel;
        
        // Check QE
        if ( CLHEP::RandFlat::shoot(1.0) > fQE ) return false;

        return true;
    }



} // namespace

DEFINE_ART_SERVICE_INTERFACE_IMPL(opdet::LariatOpDetResponse, opdet::OpDetResponseInterface)

