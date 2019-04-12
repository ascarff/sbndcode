#include "DigiArapucaSBNDAlg.h" 

#ifndef DIGIARAPUCASBNDALG_CXX
#define DIGIARAPUCASBNDALG_CXX
 
//------------------------------------------------------------------------------
//--- opdet::simarapucasbndAlg implementation
//------------------------------------------------------------------------------

namespace opdet{

    /*DigiArapucaSBNDAlg::DigiArapucaSBNDAlg(fhicl::ParameterSet const& p) 
    {
    // Reading the fcl file
    fADC                = p.get< double >("VoltageToADC"     );
    fBaselineRMS        = p.get< double >("BaselineRMS"      );
    fDarkNoiseRate      = p.get< double >("DarkNoiseRate"    );
    fCrossTalk          = p.get< double >("CrossTalk"        );
    fBaseline           = p.get< double >("Baseline"         );
    fReadoutWindow      = p.get< double >("ReadoutWindow"    );
    fPulseLength        = p.get< double >("PulseLength"      );
    fPeakTime           = p.get< double >("PeakTime"         );
    fMaxAmplitude       = p.get< double >("MaxAmplitude"     );
    fFrontTime          = p.get< double >("FrontTime"        );
    fBackTime           = p.get< double >("BackTime"         );
    fPreTrigger         = p.get< double >("PreTrigger"       );
    fSaturation         = p.get< double >("Saturation"       );
    double AraEffT1     = p.get< double >("ArapucaEffT1"     );
    double AraEffT2     = p.get< double >("ArapucaEffT2"     );
    double AraEffx      = p.get< double >("ArapucaEffx"     );

//Correction due to scalling factor applied during simulation
    auto const *LarProp = lar::providerFrom<detinfo::LArPropertiesService>();
    fArapucaEffT1 = AraEffT1/(LarProp->ScintPreScale());  
    fArapucaEffT2 = AraEffT2/(LarProp->ScintPreScale());
    fArapucaEffx  = AraEffx/(LarProp->ScintPreScale());

    std::cout << "arapucas corrected efficiencies = " << fArapucaEffT1 << ", " << fArapucaEffT2 << " and " << fArapucaEffx << std::endl;

    if(fArapucaEffT1>1.0001 || fArapucaEffT2>1.0001 || fArapucaEffx>1.0001)
	std::cout << "WARNING: Quantum efficiency set in fhicl file " << AraEffT1 << " or " << AraEffT2 << " or " << AraEffx << " seems to be too large! Final QE must be equal to or smaller than the scintillation pre scale applied at simulation time. Please check this number (ScintPreScale): " << LarProp->ScintPreScale() << std::endl;

    TimeArapucaT1 = new TH1D("Time Profile T1", "", 150, 0.0, 150.0);//histogRandomam that stores the arrival time of photons at SiPM (t=0 is the time is reaches the outside of the optical window) for arapuca T1

    double x[150] = {374, 1455, 2002, 2230, 2336, 2296, 2087, 2006, 1831, 1716, 1623, 1553, 1437, 1327, 1348, 1260, 1237, 1234, 1164, 1122, 1070, 1092, 993, 1002, 892, 969, 951, 907, 909, 961, 863, 857, 915, 900, 853, 842, 790, 780, 779, 808, 781, 749, 723, 737, 723, 755, 732, 680, 724, 631, 656, 693, 669, 632, 636, 643, 632, 640, 608, 615, 597, 633, 602, 545, 591, 595, 551, 574, 567, 507, 545, 535, 552, 519, 537, 563, 523, 461, 550, 510, 514, 469, 517, 493, 466, 460, 488, 446, 474, 516, 451, 451, 457, 465, 450, 456, 493, 441, 441, 475, 433, 419, 435, 405, 392, 410, 430, 404, 392, 407, 435, 411, 383, 422, 394, 397, 413, 366, 389, 376, 366, 372, 375, 345, 370, 368, 370, 390, 351, 382, 373, 380, 377, 339, 372, 371, 351, 360, 338, 365, 309, 187, 95, 41, 19, 7, 2, 3, 0, 0};
 
    for(size_t i=1; i<150; i++)TimeArapucaT1->SetBinContent(i,x[i]);

    TimeArapucaT2 = new TH1D("Time Profile T1", "", 90, 0.0, 90.0);//histogRandomam that stores the arrival time of photons at SiPM (t=0 is the time is reaches the outside of the optical window) for arapuca T2
 
    double x2[90]={5051, 8791, 9054, 8777, 8045, 7009, 6304, 5637, 4828, 4320, 3821, 3333, 2968, 2629, 2364, 2060, 1786, 1624, 1368, 1174, 1115, 935, 820, 725, 627, 535, 500, 453, 386, 355, 315, 279, 221, 196, 198, 181, 120, 115, 128, 109, 79, 67, 77, 59, 48, 39, 40, 37, 32, 39, 22, 25, 18, 20, 14, 19, 8, 9, 6, 11, 13, 4, 11, 4, 4, 5, 3, 2, 4, 4, 5, 2, 6, 0, 0, 2, 1, 0, 0, 0, 1, 2, 1, 0, 0, 1, 0, 1, 0, 0};

    for(size_t i=1; i<90; i++)TimeArapucaT2->SetBinContent(i,x2[i]);


    pulsesize=fPulseLength*fSampling;
    wsp.resize(pulsesize);

    for(int i=0; i<pulsesize; i++)
	wsp[i]=(Pulse1PE(static_cast< double >(i)/fSampling));
//Random number engine initialization
    //int seed = time(NULL);
    //gRandom = new TRandom3(seed);
  }

  DigiArapucaSBNDAlg::~DigiArapucaSBNDAlg()
  { }

  void DigiArapucaSBNDAlg::ConstructWaveform(int ch, sim::SimPhotons const& simphotons, std::vector<std::vector<short unsigned int>>& waveforms, std::string pdName){	
    double t_min=1e15;
    std::vector<double> waves(std::vector<double>(fNsamples,fBaseline));
    t_min=FindMinimumTime(simphotons);
    CreatePDWaveform(simphotons, t_min, waves, pdName);
    waveforms[ch] = std::vector<short unsigned int> (waves.begin(), waves.end());
  }

  void DigiArapucaSBNDAlg::ConstructWaveformLite(int ch, sim::SimPhotonsLite const& litesimphotons, std::vector<std::vector<short unsigned int>>& waveforms, std::string pdName){	
    double t_min=1e15;
    std::vector<double> waves(std::vector<double>(fNsamples,fBaseline));
    std::map< int, int > const& photonMap = litesimphotons.DetectedPhotons;
    t_min=FindMinimumTimeLite(photonMap);
    CreatePDWaveformLite(photonMap, t_min, waves, pdName);
    waveforms[ch] = std::vector<short unsigned int> (waves.begin(), waves.end());
  }

  void DigiArapucaSBNDAlg::AddSPE(size_t time_bin, std::vector<double>& wave, int nphotons){//adding single pulse
    size_t min=0, max=0;

    if(time_bin<fNsamples){
	min=time_bin;
	max=time_bin+pulsesize < fNsamples ? time_bin+pulsesize : fNsamples;
	for(size_t i = min; i<= max; i++){
		wave[i]+= (wsp[i-min])*(double)nphotons;	
	}		
    }
  }

  double DigiArapucaSBNDAlg::Pulse1PE(double time) const//single pulse waveform
  {
    if (time < fPeakTime) return (fADC*fMaxAmplitude*std::exp((time - fPeakTime)/fFrontTime));
    else return (fADC*fMaxAmplitude*std::exp(-(time - fPeakTime)/fBackTime));
  }

  void DigiArapucaSBNDAlg::AddLineNoise(std::vector< double >& wave)
  {
    double noise;
    for(size_t i = 0; i < wave.size(); i++){
        noise= gRandom->Gaus(0, fBaselineRMS); //gaussian baseline noise  
        wave[i] += noise; 
    }
  }

  void DigiArapucaSBNDAlg::AddDarkNoise(std::vector< double >& wave)
  {
    int nCT;
    // Multiply by 10^9 since fDarkNoiseRate is in Hz (conversion from s to ns)
    double darkNoiseTime = static_cast< double >(gRandom->Exp((1.0/fDarkNoiseRate)*1000000000.0));
    while (darkNoiseTime < wave.size()){
	size_t timeBin = (darkNoiseTime);
	  if(fCrossTalk>0.0 && (gRandom->Uniform(1.0))<fCrossTalk) nCT=2;
	  else nCT=1;
	  AddSPE(timeBin,wave,nCT);
        // Find next time to add dark noise
        darkNoiseTime += static_cast< double >(gRandom->Exp((1.0/fDarkNoiseRate)*1000000000.0));
    }
  }

  double DigiArapucaSBNDAlg::FindMinimumTime(sim::SimPhotons const& simphotons){
    double t_min=1e15;
      for(size_t i=0; i<simphotons.size(); i++){	 	 
      	if(simphotons[i].Time<t_min) t_min = simphotons[i].Time;
      }
    return t_min;
  }

  void DigiArapucaSBNDAlg::CreatePDWaveform(sim::SimPhotons const& simphotons, double t_min, std::vector<double>& wave, std::string pdtype){
    int nCT=1;
    double tphoton=0;
    if(pdtype=="arapucaT1"){
	for(size_t i=0; i<simphotons.size(); i++){
	  if((gRandom->Uniform(1.0))<fArapucaEffT1){ //Sample a random subset according to Arapuca's efficiency
	    tphoton=simphotons[i].Time;
	    tphoton+=(TimeArapucaT1->GetRandom());
	    tphoton+=(fPreTrigger-t_min);
 	    if(fCrossTalk>0.0 && (gRandom->Uniform(1.0))<fCrossTalk) nCT=2;
	    else nCT=1;
	    AddSPE(tphoton*fSampling,wave,nCT);
	  }
	}
    }
    if(pdtype=="arapucaT2"){   
	for(size_t i=0; i<simphotons.size(); i++){
 	  if((gRandom->Uniform(1.0))<fArapucaEffT2){ //Sample a random subset according to Arapuca's efficiency.
	    tphoton=simphotons[i].Time;
  	    tphoton+=(TimeArapucaT2->GetRandom());
	    tphoton+=(fPreTrigger-t_min);
 	    if(fCrossTalk>0.0 && (gRandom->Uniform(1.0))<fCrossTalk) nCT=2;
	    else nCT=1;
	    AddSPE(tphoton*fSampling,wave,nCT);
	  }
	}
    }
    if(pdtype=="xarapucaprime"){   
	for(size_t i=0; i<simphotons.size(); i++){
 	  if((gRandom->Uniform(1.0))<fArapucaEffx){ 
	    tphoton=simphotons[i].Time;
 // 	    tphoton+=(TimeArapucaX->GetRandom());//PROPER TIMING YET TO BE IMPLEMENTED FOR X-ARAPUCA
	    tphoton+=(fPreTrigger-t_min);
 	    if(fCrossTalk>0.0 && (gRandom->Uniform(1.0))<fCrossTalk) nCT=2;
	    else nCT=1;
	    AddSPE(tphoton*fSampling,wave,nCT);
	  }
	}
    }

    if(fBaselineRMS>0.0) AddLineNoise(wave);
    if(fDarkNoiseRate > 0.0) AddDarkNoise(wave);
    CreateSaturation(wave);
  }

  void DigiArapucaSBNDAlg::CreateSaturation(std::vector<double>& wave){ //Implementing saturation effects
    for(size_t k=0; k<fNsamples; k++){
	if(wave[k]>(fBaseline+fSaturation*fADC*fMaxAmplitude))
	  wave[k]=fBaseline+fSaturation*fADC*fMaxAmplitude;	  
    }
  }

  void DigiArapucaSBNDAlg::CreatePDWaveformLite(std::map< int, int > const& photonMap, double t_min, std::vector<double>& wave, std::string pdtype){
    double tphoton=0;
    int nCT=1;
    for (auto const& mapMember: photonMap){
      for(int i=0; i<mapMember.second; i++){
         if(pdtype=="arapucaT1" && (gRandom->Uniform(1.0))<fArapucaEffT1){
  	    tphoton=(TimeArapucaT1->GetRandom());
 	    tphoton+=mapMember.first+fPreTrigger-t_min;
 	    if(fCrossTalk>0.0 && (gRandom->Uniform(1.0))<fCrossTalk) nCT=2;
	    else nCT=1;
	    AddSPE(tphoton*fSampling,wave,nCT);
         }
         if(pdtype=="arapucaT2" && (gRandom->Uniform(1.0))<fArapucaEffT2){
  	    tphoton=(TimeArapucaT2->GetRandom());
 	    tphoton+=mapMember.first+fPreTrigger-t_min;
 	    if(fCrossTalk>0.0 && (gRandom->Uniform(1.0))<fCrossTalk) nCT=2;
	    else nCT=1;
	    AddSPE(tphoton*fSampling,wave,nCT);
         }
	if(pdtype=="xarapucaprime" && (gRandom->Uniform(1.0))<fArapucaEffx){
	   tphoton=(TimeArapucaT1->GetRandom()); //TO BE CORRECTED LATER
	   tphoton+=mapMember.first+fPreTrigger-t_min;
 	   if(fCrossTalk>0.0 && (gRandom->Uniform(1.0))<fCrossTalk) nCT=2;
	   else nCT=1;
	   AddSPE(tphoton*fSampling,wave,nCT);
	}
      }
    }
    if(fBaselineRMS>0.0) AddLineNoise(wave);
    if(fDarkNoiseRate > 0.0) AddDarkNoise(wave);
    CreateSaturation(wave);
  }

  double DigiArapucaSBNDAlg::FindMinimumTimeLite(std::map< int, int > const& photonMap){
    for (auto const& mapMember: photonMap){
 	 if(mapMember.second!=0) return (double)mapMember.first;
    }
    return 1e5;
  }*/

}

#endif
