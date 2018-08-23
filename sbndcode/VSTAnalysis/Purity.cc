#include "Purity.hh"

/*Adrian Orea: 2018
using Dominic's code and 
https://cdcvs.fnal.gov/redmine/projects/lardbt/repository/revisions/master/entry/LArIATAnaModule/PurityOnlineT1034_module.cc
as references*/

using namespace std;
  // Function to calculate the electron lifetime from a collection of hits
  // You have a vector of recob::Hit objects (you can google recob::Hit to find 
  // out what info is stored in them)

//Likelihood estimator function.
double likely(const double *par){
  // Numeric constants
  double mpshift  = -0.22278298;       				// Landau maximum 
  int index = 0;
  const double tau=par[0];
  const double sigma=par[1];
  const double dqdxo=par[2];
  double mpc;             						// Corrected most probable value
  double dqdx;            						// charge after tau scaling
  double prob;            						// Landau probability
  double lik;             						// likelihood
  double aa;
  double bb;

  vector<double> charge;
  vector<double> dtime;

  ifstream file ("tmpdontcareaboutthis.txt");

  // Read the data from the file
  while (file.good() && !file.eof())
  {
     file >> aa >> bb;
     dtime.push_back(aa);
     charge.push_back(bb); 
  }

  //Make sure vectors are the same size
  index=dtime.size()-1;
  dtime.resize(index);
  charge.resize(index); 

  lik=0;
  for (int ii = 0; ii<index; ++ii)              // Loop over hits
  { 
     // Calculate corrected charge per unit length
     dqdx=dqdxo*TMath::Exp(-(dtime.at(ii)/tau));
     // Calculate the corrected most probable value
     mpc = dqdx-mpshift*sigma;                     
     // Probability that a value of charge is observed given that the charge has a Landau distribution
     prob = TMath::Landau(charge.at(ii),mpc,sigma,kTRUE);
     // Negative log likelihood
     lik-=TMath::Log(prob);
  }     // loop over hits   
  
  file.close();
  dtime.clear();
  charge.clear(); 

  return lik;
} // likely()

void PlotAll(std::vector<TH1*> th_chargeDists, std::vector<TGraph*> tgrs){
  TFile *OutFile = new TFile("Plots.root", "RECREATE");
  for (unsigned int i = 0; i < th_chargeDists.size(); i++){
    th_chargeDists[i]->Write();
  }
  for (unsigned int i = 0; i < tgrs.size(); i++){  
    tgrs[i]->Write();
  }
  OutFile->Close();
  delete OutFile;
}

double daqAnalysis::CalculateLifetime(std::vector<art::Ptr<recob::Hit>> rawhits, const struct Analysis::AnalysisConfig& _config){	

  ///////////////////////////////////////////////////////////////////////////
  //////////////////////////       CONFIGURATION         ////////////////////
  ///////////////////////////////////////////////////////////////////////////
	int fMinColHits     = _config.min_col_hits;     // Minimum number of collection plane hits       
	int fMinIndHits     = _config.min_ind_hits;     // Minimum number of induction plane hits
  int fMinWires       = _config.min_wires;        // Minimum extent of track in wire number
  int fMinTicks       = _config.min_ticks;        // Minimum extent of track in time [ticks]
  float fTriggerTime  = _config.trigger_time;     // Time delay between the trigger and the readout
  float fChi2Cut      = _config.chi2_cut;         // Minimum chi2/ndof after 2 linear fits
  float fPcaCut       = _config.pca_cut;          // Maximum value of principal component analysis
  float fMinOverlap   = _config.min_overlap;      // Minimum percentage overlap of col/ind tracks in time
	float fChargeWidth  = _config.charge_width;     // Sigma multiplier for landau tail cut
 	float fShapingTime  = _config.shaping_time;			// Shaping time [us]
	float fDriftVel     = _config.drift_vel;	      // Drift velocity[mm/us]
	float fWireSpacing  = _config.wire_spacing;	    // Wire spacing [mm]
  float fAngleCut     = _config.angle_cut;        // Maximum value of angle to wire planes? [deg]
  bool fForceAngleCut = _config.force_angle_cut;  // Use the angle cut rather than atan(st*dv/ws)
	bool fDoAngleCut    = _config.do_angle_cut;     // Apply the angle cut
	bool fPlot          = _config.lifetime_plots;   // Make plots
	bool fVerbose       = _config.purity_verbose;   // Print stuff

  float clockSpeed = 2.; //7.8125; // 2. for VST data, 7.8125 for testing with lariat data
  float convertClocks = clockSpeed/2.;
  float driftLength = 47.; // LArIAT drift length [cm]
  float driftTime = driftLength*10.*clockSpeed/fDriftVel; 

  ///////////////////////////////////////////////////////////////////////////
  //////////////////////////     INITIALIZE THINGS       ////////////////////
  ///////////////////////////////////////////////////////////////////////////

  std::vector<TH1*> v_allTH1;
  std::vector<TGraph*> v_allTGraph;

	FILE *values_file;							//File where the minimization points are stored
	//FILE *taulengthangle;							//File with the Tau values, track length, and angle
	FILE *signal_file;							//File where the peak amplitudes are stored

  // Define minimizer
	ROOT::Math::Minimizer* min = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");	
	ROOT::Math::Functor f(&likely,3);					//Creates the function based on the Likelihood in likely function

	geo::GeometryCore const* geom = lar::providerFrom<geo::Geometry>();	//Looks at geometry of the hit

	//Sets vectors for hits/information we need.
	vector<float> v_timeC, v_wireC, v_chargeC, v_timeI, v_wireI, v_ampC,
                v_chargeCCut, v_timeCCut, v_wireCCut, v_wireICut, v_timeICut, v_ampCCut,
                v_chargeCNorm, v_chargeCFinal, v_timeCFinal, v_ampCFinal;
  
  //Check the PCA Value.                                                                         
	TPrincipal *pca = new TPrincipal(2,"");
  double hits[2];

  ///////////////////////////////////////////////////////////////////////////
  //////////////////////////   "TRACK RECONSTRUCTION"    ////////////////////
  ///////////////////////////////////////////////////////////////////////////

  if(fVerbose) cout << "----> PURITY CALCULATOR:\n"
                    << "\n--> TRACK RECONSTRUCTION:\n";

  // --------------------      CUT: TOTAL HITS     --------------------------
	if(fVerbose) cout << "Number of hits in event = " << rawhits.size() << endl;
	if ((int)rawhits.size() < fMinColHits + fMinIndHits){
		if(fVerbose) cout << "Not enough hits in event --> Skipping\n\n";
		return -1;
	}
  // ------------------------------------------------------------------------

  // Sort hits by collection and induction plane 
	for (size_t i = 0; i < rawhits.size(); i++){
		int channel = rawhits[i]->Channel();
		geo::SigType_t sigType = geom->SignalType(channel);
    float wire = (*rawhits[i]).WireID().Wire;
    float time = (*rawhits[i]).PeakTime();
    float charge = (*rawhits[i]).Integral();
    float amp = (*rawhits[i]).Integral();

    //Checks for Collection plane
		if(sigType == geo::kCollection && time > fTriggerTime && time < (fTriggerTime+driftTime+50.)){	
			v_wireC.push_back(wire);				//Wire ID
			v_timeC.push_back(time - fTriggerTime);				//Time (ticks)
			v_chargeC.push_back(charge);		//Charge (ADCs)
      v_ampC.push_back(amp);
		}
    //Checks for Induction plane
		if(sigType == geo::kInduction && time > fTriggerTime && time < (fTriggerTime+driftTime+50.)){					
			v_wireI.push_back(wire);				//WireID for Induction Plane
			v_timeI.push_back(time - fTriggerTime);				//Time for Induction Plane
		}
	}
	
  // -----------------      CUT: TOTAL COLLECTION HITS     --------------------
	if(fVerbose) cout << "Number of collection hits = " << v_wireC.size() << endl;
	if((int)v_wireC.size() < fMinColHits){
		if(fVerbose) cout << "Not enough collection plane hits --> Skipping\n\n";
		return -1;
	}
  // --------------------------------------------------------------------------

  // -----------------      CUT: TOTAL INDUCTION HITS     ---------------------
	if(fVerbose) cout << "Number of induction hits = " << v_wireI.size() << endl;
	if((int)v_wireI.size() < fMinIndHits){
		if(fVerbose) cout << "Not enough induction plane hits --> Skipping\n\n";
		return -1;
	}
  // --------------------------------------------------------------------------

  // Wire v Time graphs for fitting
	TGraph *g_wvTC = new TGraph(v_wireC.size(), v_wireC.data(), v_timeC.data());
	TGraph *g_wvTI = new TGraph(v_wireI.size(), v_wireI.data(), v_timeI.data());
  if(fPlot){
    g_wvTC->SetName("all_col_wvt");
    g_wvTC->SetTitle(";Wire number;Time (ticks)");
    g_wvTC->SetMarkerStyle(7);
    g_wvTC->SetLineWidth(0);
    v_allTGraph.push_back(g_wvTC);
    g_wvTI->SetName("all_ind_wvt");
    g_wvTI->SetTitle(";Wire number;Time (ticks)");
    g_wvTI->SetMarkerStyle(7);
    g_wvTI->SetLineWidth(0);
    v_allTGraph.push_back(g_wvTI);
  }
	
  // Polynomial functions for linear fitting
	TF1 *f_linC1 = new TF1("f_linC1","pol1", 0, 240);	
	TF1 *f_linI1 = new TF1("f_linI1","pol1", 0, 240);
  // Do an initial fit
  try{
	  g_wvTC->Fit("f_linC1","QROB=0.75");	
	  g_wvTI->Fit("f_linI1","QROB=0.75"); 
  } catch(...){
    if(fVerbose) std::cout << "Initial linear fit failed --> Skipping\n\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return -1;
  }
	
	float slopeC = f_linC1->GetParameter(1);
	float yintC = f_linC1->GetParameter(0);
	if(fVerbose) cout << "Collection slope = " << slopeC << ", y-intercept = " << yintC << endl;

	float slopeI = f_linI1->GetParameter(1);
	float yintI = f_linI1->GetParameter(0);
	if(fVerbose) cout << "Induction slope: " << slopeI << ", y-intercept: " << yintI << endl;

	float fitdev[3] = {30*convertClocks, 10*convertClocks, 3*convertClocks};

  TGraph *g_wvTCCut;
  TGraph *g_wvTICut;

	TF1 *f_linC2 = new TF1("f_linC2","pol1", 0, 240);	
	TF1 *f_linI2 = new TF1("f_linI2","pol1", 0, 240);
	// Do a recursive linear fit to remove any secondary track/showers or noise hits -> Collection
	for(int z = 0; z < 3; z++){					  		// make fit twice to make sure to pick up every hit
		v_timeCCut.clear();
		v_chargeCCut.clear();
		v_wireCCut.clear();

		for(size_t i = 0; i < v_wireC.size(); i++){  					//select only hits close to the fit
	 		double fitvalue = (slopeC * v_wireC[i]) + yintC;

	 		if(abs((v_timeC[i] - fitvalue)) < fitdev[z]){
	    	v_timeCCut.push_back(v_timeC[i]);
	    	v_chargeCCut.push_back(v_chargeC[i]);
	   		v_wireCCut.push_back(v_wireC[i]);
        v_ampCCut.push_back(v_ampC[i]);
        if(z == 2){
			    // Add PCA elements 
			    hits[0] = v_wireC[i];
			    hits[1] = v_timeC[i];
			    pca->AddRow(hits);
        }
	 		}
		}
		g_wvTCCut = new TGraph(v_wireCCut.size(), v_wireCCut.data(), v_timeCCut.data());
		try{
      g_wvTCCut->Fit("f_linC2","Q");
    } catch(...){
      if(fVerbose) std::cout << "Linear fit on collection hits failed --> Skipping\n\n";
	    if(fPlot) PlotAll(v_allTH1, v_allTGraph);
      return -1;
    }
    slopeC = f_linC2->GetParameter(1);
    yintC = f_linC2->GetParameter(0);
	}

  if(fPlot){
    g_wvTCCut->SetName("cut_col_wvt");
    g_wvTCCut->SetTitle(";Wire number;Time (ticks)");
    g_wvTCCut->SetMarkerStyle(7);
    g_wvTCCut->SetLineWidth(0);
    v_allTGraph.push_back(g_wvTCCut);
  }

	// Find the PCA eigenvalue.
	pca->MakePrincipals();
  const TVectorD *Eigenvalues = pca->GetEigenValues();
	float FirstEigenvalue = (*Eigenvalues)[0];
	delete pca;
  float pcaval = TMath::Log10(1-FirstEigenvalue);
	if(fVerbose) std::cout << "PCA Value = " << pcaval << std::endl;

  // ----------------------      CUT: PCA VALUE     --------------------------
  // Not optimized: skip for now
	if(pcaval > fPcaCut){
	  /*if(fVerbose) std::cout<< "PCA value failed cut --> Skipping\n\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
	  return -1;*/
	}
  // -------------------------------------------------------------------------

	Double_t chisqr = f_linC2->GetChisquare()/f_linC2->GetNDF();

	if(fVerbose) cout << "Number of collection hits after track cut = " << v_wireCCut.size() << endl
	                  << "Refitted Collection slope = " << slopeC << endl
	                  << "Chi2/NDF = " << chisqr  << endl;

  // -------------      CUT: CHI2/NDF AFTER 2 LINEAR FITS     ---------------
	if(chisqr != chisqr || chisqr > fChi2Cut){
		if(fVerbose) cout << "Chi2/Ndof failed cut after 2 tracks --> Skipping\n\n"; 
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
		return -1;  
	}
  // ------------------------------------------------------------------------

  // Calculate the extent of the track in wire number and time
  int wireExtent = 0;
  float timeExtent = 0;
  float startt = 0;
  float endt = 0;

  if((int)v_timeCCut.size() > fMinColHits){
    auto wresult = std::minmax_element(v_wireCCut.begin(), v_wireCCut.end());
    int startw = v_wireCCut[wresult.first-v_wireCCut.begin()];
    int endw = v_wireCCut[wresult.second-v_wireCCut.begin()];
    wireExtent = endw - startw;

    auto tresult = std::minmax_element(v_timeCCut.begin(), v_timeCCut.end());
    startt = v_timeCCut[tresult.first-v_timeCCut.begin()];
    endt = v_timeCCut[tresult.second-v_timeCCut.begin()];
    timeExtent = endt - startt;

  // -----------------    CUT: TOTAL COLLECTION HITS 2   --------------------
  } else{
    if(fVerbose) std::cout << "Not enough collection hits after track cut --> Skipping \n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return -1;
  }
  // ------------------------------------------------------------------------

  // ---------------     CUT: EXTENT OF TRACK IN WIRE      ------------------
  if(fVerbose) std::cout << "Collection track wire extent = " << wireExtent << endl;
  if(wireExtent < fMinWires){
    if(fVerbose) std::cout<<"Not enough hit wires --> Skipping\n\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return -1;
  }
  // ------------------------------------------------------------------------
  
  // ---------------     CUT: EXTENT OF TRACK IN TIME      ------------------
  if(fVerbose) std::cout << "Collection track time extent = " << timeExtent << endl;
  if(timeExtent < fMinTicks * convertClocks){
    if(fVerbose) std::cout<<"Not enough time variation --> Skipping\n\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return -1;
  }
  // ------------------------------------------------------------------------

	// Do a recursive linear fit to remove any secondary track/showers or noise hits -> Induction
	for(int q = 0; q < 3; q++){					  		// make fit twice to make sure to pick up every hit
		v_timeICut.clear();
		v_wireICut.clear();

		for(size_t i = 0; i < v_wireI.size(); i++){  					//select only hits close to the fit
	 		double fitvalue = (slopeI * v_wireI[i]) + yintI;

	 		if(abs((v_timeI[i]-fitvalue)) < fitdev[q]){
	    	v_timeICut.push_back(v_timeI[i]);
	   		v_wireICut.push_back(v_wireI[i]);
	 		}
		}

		g_wvTICut = new TGraph(v_wireICut.size(), v_wireICut.data(), v_timeICut.data());
    try{
		  g_wvTICut->Fit("f_linI2","Q");
    } catch(...){
      if(fVerbose) std::cout << "Linear fit on induction hits failed --> Skipping\n\n";
	    if(fPlot) PlotAll(v_allTH1, v_allTGraph);
      return -1;
    }
    slopeI = f_linI2->GetParameter(1);
    yintI = f_linI2->GetParameter(0);
	} 

	if(fVerbose) cout << "Number of induction hits after track cut = " << v_wireICut.size() << endl
	                  << "Refitted Induction slope = " << slopeI << endl;

  if(fPlot){
    g_wvTICut->SetName("cut_ind_wvt");
    g_wvTICut->SetTitle(";Wire number;Time (ticks)");
    g_wvTICut->SetMarkerStyle(7);
    g_wvTICut->SetLineWidth(0);
    v_allTGraph.push_back(g_wvTICut);
  }

  // Cut to check if time ranges of collection and induction hits overlap
  float overlap = 0;
  if((int)v_timeICut.size() > fMinIndHits){
    auto tresult = std::minmax_element(v_timeICut.begin(), v_timeICut.end());
    float starttI = v_timeICut[tresult.first-v_timeICut.begin()];
    float endtI = v_timeICut[tresult.second-v_timeICut.begin()];
    overlap = (std::min(endt, endtI) - std::max(startt, starttI)) / (endt - startt);
    if (fVerbose) std::cout << "% overlap in collection and induction planes = " << overlap << endl;
  // ---------------  CUT: COL AND IND OVERLAP IN TIME   --------------------
    if(overlap < fMinOverlap){
      if(fVerbose) std::cout << "Collection and induction tracks do not overlap --> Skipping\n\n";
	    if(fPlot) PlotAll(v_allTH1, v_allTGraph);
      return -1;
    }
  // ------------------------------------------------------------------------
  // ------------------  CUT: TOTAL INDUCTION HITS 2   ----------------------
  } else {
    if(fVerbose) std::cout << "Not enough induction hits after track cut --> Skipping\n\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return -1;
  }
  // ------------------------------------------------------------------------

	//Pretty much all of the parts prior to fitting from here are from Dom's code
	float angle = atan((fShapingTime * fDriftVel) / fWireSpacing);
	if(fForceAngleCut) angle =(fAngleCut*TMath::Pi()/180);

	float tick2us = 1/clockSpeed;
	float tick_to_wire = (tick2us * fDriftVel) / fWireSpacing;	
	slopeC *= tick_to_wire;
	slopeI *= tick_to_wire;

	float theta = atan(abs(slopeC));		
	float phi = atan(abs(slopeC*0.866/slopeI)); 
	//float phi = atan(sqrt(3)/(1+2*(slopeC/slopeI))); 
	if(fVerbose) cout << "Track theta = " << theta*180/TMath::Pi() << endl
	                  << "Track phi = " << phi*180/TMath::Pi() << endl;
	
  // ------------------  CUT: ANGLE TO WIRE PLANES  -------------------------
	if(fDoAngleCut && (theta > angle)){
		if(fVerbose) cout << "Failed track angle cut --> Skipping" << endl;
    if(fPlot) PlotAll(v_allTH1, v_allTGraph);
		return -1;
	}
  // ------------------------------------------------------------------------

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////   CHARGE NORMALIZATION & CUT    ///////////////////
  ///////////////////////////////////////////////////////////////////////////

  if(fVerbose) cout << "\n--> CHARGE NORMALIZATION & CUT:\n";
		
  // Normalize the charge using the track pitch
	float normalization = abs(cos(theta)*cos(phi));
	if(fVerbose) cout << "Pitch normalisation = " << normalization <<endl;
  //Making the vector the same size as v_chargeCCut
	v_chargeCNorm.resize(v_chargeCCut.size());
	transform(v_chargeCCut.begin(), v_chargeCCut.end(), v_chargeCNorm.begin(),
            bind(multiplies<float>(), std::placeholders::_1,
            normalization*(1 / (sqrt(6.28319) * fShapingTime*tick2us))));

  // Graph of normalized charge vs time
	TGraph *g_CvTNorm = new TGraph(v_timeCCut.size(), v_timeCCut.data(), v_chargeCNorm.data());
  if(fPlot){
    g_CvTNorm->SetName("norm_col_cvt");
    g_CvTNorm->SetTitle(";Time (ticks); Charge (N*ADCs)");
    g_CvTNorm->SetMarkerStyle(7);
    g_CvTNorm->SetLineWidth(0);
    v_allTGraph.push_back(g_CvTNorm);
  }

  // Fit an exponential to charge v time
	TF1 *f_exp1 = new TF1("f_exp1","expo");	
  try{
	  g_CvTNorm->Fit("f_exp1","Q");					
  } catch(...){
    if(fVerbose) std::cout << "First exponential fit failed --> Skipping\n\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return -1;
  }
	double expo0 = f_exp1->GetParameter(0);				//Getting the "Normalization constant"
	double expo1 = f_exp1->GetParameter(1);				//Decay rate
	if(fVerbose) cout << "Normalization constant from exp fit = " << expo0 << endl
	                  << "Decay rate from exp fit = " << expo1 << endl;

  // Get an estimate for the width of the charge landau
	TH1D *h_chargeDist = new TH1D("chargedist",";Charge Distance;", 50, -600*convertClocks, 600*convertClocks);
	for(size_t y = 0; y < v_chargeCNorm.size(); y++){
		h_chargeDist->Fill(v_chargeCNorm[y]-TMath::Exp(expo0+(expo1*v_timeCCut[y])));
	}
	TF1 *f_landau = new TF1("f_landau","landau", -600*convertClocks, 600*convertClocks);
  try{
	  h_chargeDist->Fit("f_landau","Q");		
  } catch(...){
    if(fVerbose) std::cout << "Landau fit failed --> Skipping\n\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return -1;
  }
	double landau1=f_landau->GetParameter(2);					//Sigma of the Landau distribution
	if(fVerbose) cout << "Sigma of Landau fit = " << landau1 << endl;
  
  if(fPlot){
    v_allTH1.push_back(h_chargeDist);
  }

  //Cut off landau tail of the charge
	double lowcut;
	double hicut;
	v_chargeCFinal.clear();
	v_timeCFinal.clear();
	values_file=fopen("tmpdontcareaboutthis.txt","w");
	for (size_t i = 0; i < v_timeCCut.size(); i++){

		if((fChargeWidth*landau1) < 1200){
			hicut	= TMath::Exp(expo0+(expo1*v_timeCCut[i])) + (fChargeWidth*landau1);
			lowcut = TMath::Exp(expo0+(expo1*v_timeCCut[i])) - (fChargeWidth*landau1);
		}
		else{ 
			lowcut = exp(expo0+(expo1*v_timeCCut[i])) - 1200;
			hicut = exp(expo0+(expo1*v_timeCCut[i])) + 1200;
		}

		if(v_chargeCNorm[i] > lowcut && v_chargeCNorm[i] < hicut){
			v_chargeCFinal.push_back(v_chargeCNorm[i]);
			v_timeCFinal.push_back(v_timeCCut[i]);
      if(v_timeCCut[i] < 100) v_ampCFinal.push_back(v_ampCCut[i]);
			fprintf(values_file, "%6.2f %6.0f\n", v_timeCCut[i], v_chargeCNorm[i]);
		}

	}
  fclose(values_file);

	TF1 *f_exp2 = new TF1("f_exp2","expo");	//Exponential plot for fitting g_CvTNorm
	TGraph *g_CvTCut = new TGraph(v_timeCFinal.size(), v_timeCFinal.data(), v_chargeCFinal.data());
  try{
	  g_CvTCut->Fit("f_exp2","Q");
  } catch(...){
    if(fVerbose) std::cout << "Second exponential fit failed --> Skipping\n\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return -1;
  }
  expo0 = f_exp2->GetParameter(0);
  expo1 = f_exp2->GetParameter(1);
	if(fVerbose) cout << "Normalization constant after charge cut = " << expo0 << endl
	                  << "Decay rate after charge cut = " << expo1 << endl;

  if(fPlot){
    g_CvTCut->SetName("cut_col_cvt");
    g_CvTCut->SetTitle(";Time (ticks); Charge (N*ADCs)");
    g_CvTCut->SetMarkerStyle(7);
    g_CvTCut->SetLineWidth(0);
    v_allTGraph.push_back(g_CvTCut);
  }

	float lifetime = 1/(-1*expo1);
	if(fVerbose) cout << "Lifetime (exponential fit) = " << (lifetime/2000)  << "ms\n";

  // ----------------  CUT: CHARGE GOING UPWARDS  ---------------------------
  if(expo1 > 0.0 || lifetime > 200000.){
    if(fVerbose) cout << "Exponential fit gave charge going up --> Skipping\n\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return -1;
  }
  // ------------------------------------------------------------------------

  float dqdx0 = TMath::Exp(f_exp2->GetParameter(0));

  ///////////////////////////////////////////////////////////////////////////
  ////////////////////////// LOG LIKELIHOOD MINIMIZATION ////////////////////
  ///////////////////////////////////////////////////////////////////////////	

	if(fVerbose) cout << "\n--> MINIMIZATION:\n";

	min->SetFunction(f);
	// set tolerance , etc... 
	min->SetMaxFunctionCalls(1000000); 					// for Minuit/Minuit2 
	min->SetMaxIterations(10000); 	 					// for GSL 
	min->SetTolerance(0.0001);
	min->SetPrintLevel(0);
  if(fVerbose) min->SetPrintLevel(1);
 
	// Set the free variables to be minimized!
	min->SetVariable(0, "Tau", 1000., 1.);	// Electron lifetime from 2nd exp fit
	min->SetVariable(1, "Sigma", 1000., 1.);	// Width of Landau
	min->SetVariable(2, "dqdx0", 5000., 1.);	// Charge at t = 0

  double lowtaulimit = 10; //0.;     // lower limit on tau minimization
  double hitaulimit = 100000; //2. * lifetime;      // upper limit on tau minimization
  double lowsigmalimit = 0; //landau1 - 0.5*landau1;   // lower limit on sigma minimization
  double hisigmalimit = 6000; //landau1 + 0.5*landau1;    // upper limit on sigma minimization
  double lowdqdxolimit = 100; //dqdx0 - 0.5*dqdx0;   // lower limit on dqdxo minimization
  double hidqdxolimit = 40000; //dqdx0 + 0.5*dqdx0;    // upper limit on dqdxo minimization

	min->SetVariableLimits(0, lowtaulimit, hitaulimit);
	min->SetVariableLimits(1, lowsigmalimit, hisigmalimit);
	min->SetVariableLimits(2, lowdqdxolimit, hidqdxolimit);

	min->SetVariableValue(2, dqdx0);

	try{min->Minimize();} 							// do the minimization
  catch(...){
    if(fVerbose) std::cout << "Minimization failed --> Lifetime from fit\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return lifetime;
  }

	const double *xs = min->X();
	if( xs[0]!=xs[0] || xs[1]!=xs[1] || xs[2]!=xs[2] ) 
	{
	  if(fVerbose) cout << "Minimization gave NaN value --> Lifetime from fit\n"; 
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
	  return lifetime;
	}

	double tauvec = xs[0];
	if(fVerbose) cout << "Lifetime (minimization) = " << (tauvec/2000) << " ms\n\n";

/*
	// Get Errors from MiNOS
	min->SetErrorDef(0.5);
	for (int l=0; l<3; l++){
		//sigminoserrlow[l] = 0;
		//sigminoserrhi[l] = 0;
	}
	min->GetMinosError(0, sigminoserrlow[0], sigminoserrhi[0]);		// Get MINOS errors accordingly.
	min->GetMinosError(1, sigminoserrlow[1], sigminoserrhi[1]); 		// Get MINOS errors accordingly.
	min->GetMinosError(2, sigminoserrlow[2], sigminoserrhi[2]); 		// Get MINOS errors accordingly.
	const double *exs = min->Errors();

	if( exs[0]!=exs[0] || exs[1]!=exs[1] || exs[2]!=exs[2] ) 
	{
	 if(fVerbose) cout << "Minimization gave NaN errors --> Lifetime from fit\n"; 
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
	 //return lifetime;
	}

	double error_tauvec = exs[0];
*/

  if(tauvec < 0 || (tauvec/2000) > 49){ 
    if(fVerbose) std::cout << "Minimizer failed to converge --> Lifetime from fit\n";
	  if(fPlot) PlotAll(v_allTH1, v_allTGraph);
    return lifetime;
  }
/*
	taulengthangle = fopen("lifetimes.txt","a");
	fprintf(taulengthangle, "%6.2f %6.2f %6.2f %6.2f %i %i %6.2f %i %6.2f %6.2f\n", 
          tauvec/2000, lifetime/2000, chisqr, pcaval, (int)v_timeCCut.size(), 
          wireExtent, timeExtent, (int)v_timeCFinal.size(), overlap, min->MinValue());
	fclose(taulengthangle);
*/
	min->Clear();

  signal_file = fopen("signal_file.txt", "a");
  for(size_t i = 0; i < v_ampCFinal.size(); i++){
    fprintf(signal_file, "%6.2f,", v_ampCFinal[i]);
  }
  fclose(signal_file);

	if(fPlot) PlotAll(v_allTH1, v_allTGraph);
  return tauvec;
}
