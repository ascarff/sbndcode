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

double daqAnalysis::CalculateLifetime(std::vector<art::Ptr<recob::Hit>> rawhits, const struct Analysis::AnalysisConfig& _config){	

	FILE *values_file;							//File where the minimization points are stored
	FILE *taulengthangle;							//File with the Tau values, track length, and angle

  // Define minimizer
	ROOT::Math::Functor f(&likely,3);					//Creates the function based on the Likelihood in likely function
	ROOT::Math::Minimizer* min = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");	

	geo::GeometryCore const* geom = lar::providerFrom<geo::Geometry>();	//Looks at geometry of the hit

  // Set cut values from configuration
  double lowtaulimit = _config.lowtaulimit;     // lower limit on tau minimization
	double hitaulimit = _config.hitaulimit;       // upper limit on tau minimization                   
	double lowsigmalimit = _config.lowsigmalimit; // lower limit on sigma minimization
	double hisigmalimit = _config.hisigmalimit;   // upper limit on sigma minimization
	double lowdqdxolimit = _config.lowdqdxolimit; // lower limit on dqdxo minimization
	double hidqdxolimit = _config.hidqdxolimit;   // upper limit on dqdxo minimization

	double FirstSig = _config.FirstSig; //????
	double SecondSig = _config.SecondSig; //?????
	int mincount = _config.mincount;//fcl

	bool Plot = true; //_config.LifetimePlots;
	bool Dom = _config.Anglecut;
	bool fVerbose = true; //_config.verbose;

	if(fVerbose) cout << "Total number of hits in event is: " << rawhits.size() << endl;

	if ((int)rawhits.size() < mincount){
		if(fVerbose) cout << "Minimum Hits of " << mincount << " not met --> Skipping\n";
		return -1;
	}
	
	//Sets vectors for hits/information we need.
	vector<float> v_timeC, v_wireC, v_chargeC, v_timeI, v_wireI,
                v_chargeCCut, v_timeCCut, v_wireCCut, v_wireICut, v_timeICut, 
                v_chargeCNorm, v_timeCChargeCut, v_chargeCChargeCut, 
                v_chargeCFinal, v_timeCFinal;

	//Check the PCA Value.                                                                         
	TPrincipal *pca = new TPrincipal(2,"");
        double hits[2];

  // Sort hits by collection and induction plane 
	for (size_t i = 0; i < rawhits.size(); i++){
		int channel = rawhits[i]->Channel();
		geo::SigType_t sigType = geom->SignalType(channel);

		if(sigType == geo::kCollection){				//Checks for Collection plane
			int wire = (*rawhits[i]).WireID().Wire;
			float time = (*rawhits[i]).PeakTime();
			v_wireC.push_back(wire);				//Wire ID
			v_timeC.push_back(time);				//Time (ticks)
			v_chargeC.push_back((*rawhits[i]).Integral());		//Charge (ADCs)
			// Add PCA elements 
			hits[0] = wire;
			hits[1] = time;
			pca->AddRow(hits);
		}

		if(sigType == geo::kInduction){					//Checks for Induction plane
			int wire = (*rawhits[i]).WireID().Wire;
			float time = (*rawhits[i]).PeakTime();
			v_wireI.push_back(wire);				//WireID for Induction Plane
			v_timeI.push_back(time);				//Time for Induction Plane
		}
	}
	
  // Cut on number of collection plane hits
	if(fVerbose) cout << "Number of collection hits = " << v_wireC.size() << endl;
	if((int)v_wireC.size() < 50){
		if(fVerbose) cout << "Not enough collection plane hits --> Skipping\n";
		return -1;
	}

  // Cut on number of induction plane hits
	if(fVerbose) cout << "Number of induction hits = " << v_wireI.size() << endl;
	if((int)v_wireI.size() < 50){
		if(fVerbose) cout << "Not enough induction plane hits --> Skipping\n";
		return -1;
	}

	//Find the PCA eigenvalue. // SHOULD PROB BE MOVED AFTER LINEAR FITS
	pca->MakePrincipals();
  const TVectorD *Eigenvalues = pca->GetEigenValues();
	float FirstEigenvalue = (*Eigenvalues)[0];
	delete pca;
	if(fVerbose) std::cout << "PCA Value: " << TMath::Log10(1-FirstEigenvalue) << std::endl;

  // Cut on the PCA eigenvalue
	if(TMath::Log10(1-FirstEigenvalue) > _config.pcacut){
	  if(fVerbose) std::cout<< "PCA value failed cut --> Skipping\n";
	  return -1;
	}

  // Wire v Time graphs for fitting
	TGraph *WvT = new TGraph(v_wireC.size(), v_wireC.data(), v_timeC.data());
	TGraph *IWvT = new TGraph(v_wireI.size(), v_wireI.data(), v_timeI.data());
	
  // Polynomial functions for linear fitting
	TF1 *colLinFit = new TF1("colLinFit","pol1", 0, 240);	
	TF1 *indLinFit = new TF1("indLinFit","pol1", 0, 240);

	//colLinFit->SetParameters(1.0,1.0);						//Setting slope and intercept to 1
	//indLinFit->SetParameters(1.0,1.0);
	
  // Do an initial fit
	WvT->Fit("colLinFit","+Qrob = 0.75");	
	IWvT->Fit("indLinFit","+Qrob = 0.75");
	
	float slopeC = colLinFit->GetParameter(1);
	float yintC = colLinFit->GetParameter(0);
	if(fVerbose) cout << "First Collection slope: " << slopeC << ", y-intercept: " << yintC << endl;

	float slopeI = indLinFit->GetParameter(1);
	float yintI = indLinFit->GetParameter(0);
	if(fVerbose) cout << "First Induction slope: " << slopeI << ", y-intercept: " << yintI << endl;

	float fitdev = 10;							//Originally a set distance, now just a percentage.
  TGraph *New_WvT;
  TGraph *INew_WvT;

	TH1D *h2 = new TH1D("h2","Distance from fit", 250, 0, 20); 
	TH1D *h3 = new TH1D("h3","Distance from fit", 250, 0, 20);

	// Do a recursive linear fit to remove any secondary track/showers or noise hits -> Collection
	for(int z = 0; z < 2; z++){					  		// make fit twice to make sure to pick up every hit
		v_timeCCut.clear();
		v_chargeCCut.clear();
		v_wireCCut.clear();

		for(size_t i = 0; i < v_wireC.size(); i++){  					//select only hits close to the fit
	 		double fitvalue = (slopeC * v_wireC[i]) + yintC;
			h2->Fill(abs(v_timeC[i] - fitvalue));

	 		if(abs((v_timeC[i] - fitvalue)) < fitdev){
	    	v_timeCCut.push_back(v_timeC[i]);
	    	v_chargeCCut.push_back(v_chargeC[i]);
	   		v_wireCCut.push_back(v_wireC[i]);
	 		}
		}
    if(fVerbose) cout << "Pass " << z+1 << ": number of hits = " << v_wireCCut.size() << endl;
		New_WvT = new TGraph(v_wireCCut.size(), v_wireCCut.data(), v_timeCCut.data());
		New_WvT->Fit("colLinFit","QO");
    slopeC = colLinFit->GetParameter(1);
    yintC = colLinFit->GetParameter(0);
	}

	Double_t chisqr = colLinFit->GetChisquare()/colLinFit->GetNDF();  		// Obtain chi^2 divided by number of degrees of freedom

	if(fVerbose) cout << "The size of NewPT is: " << v_wireCCut.size() << endl;
	if(fVerbose) cout << "Refitted Collection slope is: " << slopeC << endl;
	if(fVerbose) cout << "Chi2/NOF: " << chisqr  << endl;

	/*TCanvas *cWvT  = new TCanvas("WvT","Peaktime vs WireID",10,10,800,600); 	//Creating canvases to draw onto. Purely for visual inspection
  New_WvT->Draw();
  colLinFit->Draw("SAME");
  cWvT->SaveAs("WvT.root");*/

  // Cut on chi2/ndof for collection fit
	if(chisqr != chisqr || chisqr > 30){ //_config.chi2cut
		if(fVerbose) cout << "Chi2/Ndof failed cut after 2 tracks --> Skipping\n"; 
		return -1;                   				// Skip the event if the chi square is a nan value
	}

  // Calculate the extent of the track in wire number and time
  int wireExtent = 0;
  float timeExtent = 0;
  if(!v_timeCCut.empty()){
    auto wresult = std::minmax_element(v_wireCCut.begin(), v_wireCCut.end());
    int startw = v_wireCCut[wresult.first-v_wireCCut.begin()];
    int endw = v_wireCCut[wresult.second-v_wireCCut.begin()];
    wireExtent = endw - startw;
    auto tresult = std::minmax_element(v_timeCCut.begin(), v_timeCCut.end());
    int startt = v_timeCCut[tresult.first-v_timeCCut.begin()];
    int endt = v_timeCCut[tresult.second-v_timeCCut.begin()];
    timeExtent = endt - startt;
  }

  // Cut on extent of track in wire number
  if(wireExtent < 50){
    if(fVerbose) std::cout<<"Not enough hit wires --> Skipping\n";
    return -1;
  }
  
  // Cut on extent of track in time
  if(timeExtent < 50){
    if(fVerbose) std::cout<<"Not enough time variation --> Skipping\n";
    return -1;
  }

	// Do a recursive linear fit to remove any secondary track/showers or noise hits -> Induction
	for(int q = 0; q < 2; q++){					  		// make fit twice to make sure to pick up every hit
		v_timeICut.clear();
		v_wireICut.clear();

		for(size_t i = 0; i < v_wireI.size(); i++){  					//select only hits close to the fit
	 		double fitvalue = (slopeI * v_wireI[i]) + yintI;
			h3->Fill(abs(v_timeI[i]-fitvalue));

	 		if(abs((v_timeI[i]-fitvalue))<fitdev){
	    	v_timeICut.push_back(v_timeI[i]);
	   		v_wireICut.push_back(v_wireI[i]);
	 		}
		}

		INew_WvT = new TGraph(v_wireICut.size(), v_wireICut.data(), v_timeICut.data());
		INew_WvT->Fit("indLinFit","QO");
    slopeI = indLinFit->GetParameter(1);
    yintI = indLinFit->GetParameter(0);
	} 

  /*TCanvas *cIWvT  = new TCanvas("IWvT","Peaktime vs WireID",10,10,800,600); 	//Creating canvases to draw onto. Purely for visual inspection
  INew_WvT->Draw();
  indLinFit->Draw("SAME");
  cIWvT->SaveAs("IWvT.root");*/

	if(fVerbose) cout << "The size of NewIPT is: " << v_wireICut.size() << endl;
	if(fVerbose) cout << "Refitted Induction slope is: " << slopeI << endl;

  // Slope on collection and induction plane should be similar
  if(abs(slopeC - slopeI) > 2){
    if(fVerbose) cout << "Time range in collection and induction planes don't match --> Skipping" << endl;
    return -1;
  }

	//Pretty much all of the parts prior to fitting from here are from Dom's code
 	float shaping_time = _config.shapingtime;				//In microseconds. GET FROM DETECTOR PROPERTIES
	float drift_vel =  1.50638;						//In mm/microsecond. GET FROM GEOMETRY
	float wire_spacing = 4;							//In mm. GET FROM DETECTOR PROPERTIES
	float angle = atan((shaping_time*drift_vel)/wire_spacing);
	if(_config.fforceanglecut) angle =(_config.anglecut*TMath::Pi()/180);
	float us2tick = 0.5; // CONVERT USING DETECTOR PROPERTIES
	float tick_to_wire = us2tick/wire_spacing*drift_vel;	
	slopeC *= tick_to_wire;
	slopeI *= tick_to_wire;
	if(fVerbose) cout << "Converted Slope " << slopeC << endl;
	if(fVerbose) cout << "Converted ISlope " << slopeI << endl;
	float theta = atan(abs(slopeC));		
	float phi = atan(abs(slopeC*0.866/slopeI));
	if(fVerbose) cout << "The value of theta is: " << theta*180/3.141 << endl;
	if(fVerbose) cout << "The value of phi is: " << phi*180/3.141 << endl;
	
	if(Dom){
		if(theta > angle){			//In Dom's code
			cout << "Dom's Angle cut. Skipping event..." << endl;
			//return 1;
		}
	}
		
  // Normalize the charge using the track pitch
	float normalization = abs(cos(theta)*cos(phi));
	if(fVerbose) cout << "normalisation is : " << normalization <<endl;
	v_chargeCNorm.resize(v_chargeCCut.size());						//Making the vector the same size as v_chargeCCut
	transform(v_chargeCCut.begin(), v_chargeCCut.end(), v_chargeCNorm.begin(),
            bind(multiplies<float>(),std::placeholders::_1,
            normalization*(1/(sqrt(6.28319)*shaping_time*us2tick))));

  // Graph of normalized charge vs time
	TGraph *NormalIvT = new TGraph(v_timeCCut.size(), v_timeCCut.data(), v_chargeCNorm.data());

  // Get the min and max times NOT SURE IF THIS IS NEEDED
	double NewPTminx = *min_element(begin(v_timeCCut), end(v_timeCCut));
	double NewPTmaxx = *max_element(begin(v_timeCCut), end(v_timeCCut));
	if(fVerbose) cout << "Minimum New PT is: " << NewPTminx << " Maximum New PT is: " << NewPTmaxx << endl;

  // Fit an exponential to charge v time
	TF1 *colExpFit = new TF1("colExpFit","expo", NewPTminx, NewPTmaxx);	//Exponential plot for fitting NormalIvT
	NormalIvT->Fit("colExpFit","NQ");						//Fitting NormalIvT with colExpFit
	double expo0 = colExpFit->GetParameter(0);				//Getting the "Normalization constant"
	double expo1 = colExpFit->GetParameter(1);				//Decay rate
	if(fVerbose) cout << "The first 'normalization' constant is: " << expo0 << endl;
	if(fVerbose) cout << "The first decay rate is: " << expo1 << endl;

  // Get an estimate for the width of the charge landau
	TH1D *h1 = new TH1D("h1","Charge Distance", 200, -600, 600);
	for(size_t y = 0; y < v_chargeCNorm.size(); y++){
		h1->Fill(v_chargeCNorm[y]-TMath::Exp(expo0+(expo1*v_timeCCut[y])));
	}
	TF1 *colLandauFit = new TF1("colLandauFit","landau", -600, 600);			//From LArIAT code
	h1->Fit("colLandauFit","NQ");							//Fitting an Landau to the histogram
	double landau1=colLandauFit->GetParameter(2);					//Sigma of the Landau distribution
	if(fVerbose) cout << "Sigma of the first Landau fit is: " << landau1 << endl;

	double lowcut;
	double hicut;
	TGraph *FitRe;
  //This part will refit the exponential between +/- #sigma
	for(int r = 0; r < 2; r++){
		v_chargeCChargeCut.clear();
		v_timeCChargeCut.clear();
		h1->Reset();
		if(true){ 						//Charge should decay
			if(fVerbose) cout << "Cleared the two FitTrial vectors" << endl;
			for (size_t i = 0; i < v_timeCCut.size(); i++){

				if((4*landau1)<1200){
					hicut=exp(expo0+(expo1*v_timeCCut[i]))+(FirstSig*landau1);
					lowcut=0;
				}
				else{ 
					lowcut=exp(expo0+(expo1*v_timeCCut[i]))-1200;
					hicut=exp(expo0+(expo1*v_timeCCut[i]))+1200;
				}

				if(v_chargeCNorm[i] > lowcut && v_chargeCNorm[i] < hicut){
					v_chargeCChargeCut.push_back(v_chargeCNorm[i]);
					v_timeCChargeCut.push_back(v_timeCCut[i]);
				
				}
			}
			if(fVerbose) cout << "The size of v_timeCChargeCut is " << v_timeCChargeCut.size() << endl;
			FitRe = new TGraph(v_timeCChargeCut.size(), v_timeCChargeCut.data(), v_chargeCChargeCut.data());
			FitRe->Fit("colExpFit","NQ");
			expo0 = colExpFit->GetParameter(0);			//Getting the "Normalization constant"
			expo1 = colExpFit->GetParameter(1);			//Decay rate

			for(size_t y = 0; y < v_chargeCChargeCut.size(); y++){
				h1->Fill(v_chargeCChargeCut[y]-exp(expo0+(expo1*v_timeCChargeCut[y])));
			}
			h1->Fit("colLandauFit","NQ");
			landau1=colLandauFit->GetParameter(2);
		}
		/*else{
			if(fVerbose) cout << "The first exponential rate is positive. Skipping event..." << endl;
			return -1;
		}*/
	}
	if(fVerbose) cout << "The refitted 'normalization' constant is: " << expo0 << endl;
	if(fVerbose) cout << "The refitted decay rate is: " << expo1 << endl;
	if(fVerbose) cout << "Sigma of the Refitted Landau fit is: " << landau1 << endl;

  if(expo1 < 0.0){
    if(fVerbose) cout << "Exponential fit gave a negative slope --> Skipping\n";
    return -1;
  }

  //THIS PART CHANGES THE ACTUAL CUT
	v_chargeCFinal.clear();
	v_timeCFinal.clear();
	values_file=fopen("tmpdontcareaboutthis.txt","w");
	for (size_t i = 0; i < v_timeCChargeCut.size(); i++){

		if((4*landau1)<1200){
			hicut	=exp(expo0+(expo1*v_timeCChargeCut[i]))+(SecondSig*landau1);
			lowcut=0;
		}
		else{ 
			lowcut=exp(expo0+(expo1*v_timeCChargeCut[i]))-1200;
			hicut=exp(expo0+(expo1*v_timeCChargeCut[i]))+1200;
		}

		if(v_chargeCChargeCut[i] > lowcut && v_chargeCChargeCut[i] < hicut){
			v_chargeCFinal.push_back(v_chargeCChargeCut[i]);
			v_timeCFinal.push_back(v_timeCChargeCut[i]);
			fprintf(values_file, "%6.2f %6.0f\n", v_timeCChargeCut[i], v_chargeCChargeCut[i]);
		}

	}

	cout << "Final decay constant and normalization constant are: " << expo1 << " , " << expo0 << endl;
	float lifetime = 1/(-1*expo1);						//The lifetime of the exponential fit
	double icharge = exp(expo0);						//Initial charge dQ/dx0
	TF1 *sigma = new TF1("sigma","[0]*[1]", NewPTminx, NewPTmaxx);
	sigma->SetParameter(0, FirstSig);
	sigma->SetParameter(1, landau1);
	TF1 *fsum = new TF1("fsum", "sigma + colExpFit", NewPTminx, NewPTmaxx);
	if(fVerbose) cout << "landau1: " << landau1 << " Firstsig: " << FirstSig << endl;
	if(fVerbose) cout << "All calculations are made. Ready for Minimization" << endl;

////////////////////////////////////////////////////////////////////
////////////////////// JUST PLOTTING STUFF /////////////////////////
////////////////////////////////////////////////////////////////////
	
	if(Plot){
		TCanvas *cWvT  = new TCanvas("WvT","Peaktime vs WireID",10,10,800,600); 	//Creating canvases to draw onto. Purely for visual inspection
		TCanvas *cIWvT  = new TCanvas("IWvT","Peaktime vs WireID",10,10,800,600);
		TCanvas *cInt  = new TCanvas("Int","Integral vs PeakTime",10,10,800,600);
		TCanvas *cHists  = new TCanvas("PtimeDev","Distance from fit",10,10,800,600);
		TCanvas *cLandau  = new TCanvas("Landau","Distance of charge from fit",10,10,800,600);

		cWvT->Divide(1,2);
		cWvT->cd(1);
		gStyle->SetOptStat(0);
		WvT->SetMarkerStyle(3);
		WvT->SetTitle("Collection Plane");
		WvT->GetXaxis()->SetTitle("WireID");
		WvT->GetYaxis()->SetTitle("PeakTime(tick)");
		WvT->GetXaxis()->SetLimits(0,240);
		WvT->Draw("AP"); 							//Data.No Connecting Line
		colLinFit->Draw("SAME");

		cWvT->cd(2);
		New_WvT->SetMarkerColor(kBlue);
		New_WvT->SetMarkerStyle(4);
		New_WvT->SetTitle("Collection Plane");
		New_WvT->GetXaxis()->SetTitle("WireID");
		New_WvT->GetYaxis()->SetTitle("PeakTime(tick)");
		New_WvT->GetXaxis()->SetLimits(0,240);
		New_WvT->Draw("AP");
		colLinFit->Draw("SAME");

		cWvT->SaveAs("Peak.root");
		
		cIWvT->Divide(1,2);
		cIWvT->cd(1);
		gStyle->SetOptStat(0);
		IWvT->SetMarkerStyle(3);
		IWvT->SetTitle("Induction Plane");
		IWvT->GetXaxis()->SetTitle("WireID");
		IWvT->GetYaxis()->SetTitle("PeakTime(tick)");
		IWvT->GetXaxis()->SetLimits(0,240);
		IWvT->Draw("AP"); 							//Data. No Connecting Line
		indLinFit->Draw("SAME");

		cIWvT->cd(2);
		INew_WvT->SetMarkerColor(kBlue);
		INew_WvT->SetMarkerStyle(4);
		INew_WvT->SetTitle("Induction Plane");
		INew_WvT->GetXaxis()->SetTitle("WireID");
		INew_WvT->GetYaxis()->SetTitle("PeakTime(tick)");
		INew_WvT->GetXaxis()->SetLimits(0,240);
		INew_WvT->Draw("APSAME");
		indLinFit->Draw("SAME");
		cIWvT->SaveAs("IPeak.root");
		
		cInt->Divide(1,2);
		cInt->cd(1);
		gStyle->SetOptStat(0);
		NormalIvT->SetMarkerColor(kRed);
		NormalIvT->SetMarkerStyle(2);
		NormalIvT->GetYaxis()->SetRangeUser(0,900);
		NormalIvT->SetTitle("Charge; Time(ticks); Charge (ADCs)");
		NormalIvT->Draw("AP");

		colExpFit->SetLineColor(kMagenta);
		colExpFit->Draw("SAME");

		fsum->SetLineColor(kMagenta);
		fsum->Draw("SAME");

		cInt->cd(2);
		FitRe->SetMarkerColor(kGreen);
		FitRe->SetMarkerStyle(3);
		FitRe->GetYaxis()->SetRangeUser(0,900);
		FitRe->SetTitle("Charge; Time(ticks); Charge (ADCs)");
		FitRe->Draw("AP");			
			
		colExpFit->SetLineColor(kOrange);
		colExpFit->Draw("SAME");
		cInt->SaveAs("Charge.root");

		cHists->Divide(1,2);
		cHists->cd(1);
		h2->SetLineColor(kBlue);
		h2->SetTitle("Collection Plane; Ticks; Entries");
		h2->GetYaxis()->SetTickLength(.01);
		h2->Draw();

		cHists->cd(2);
		h3->SetLineColor(kBlue);
		h3->SetTitle("Induction Plane; Ticks; Entries");
		h3->GetYaxis()->SetTickLength(.01);
		h3->Draw();
		cHists->SaveAs("Distance.root");
	
		cLandau->cd();
		h1->SetTitle("Charge distance to fit; ADC's; Entries");
		h1->GetYaxis()->SetTickLength(.01);
		h1->Draw();
		colLandauFit->Draw("SAME");
		cLandau->SaveAs("Landau.root");
	}


///////////////////////////////////////////////////////////////////////////
////////////////////// THIS IS FROM THE LArIAT CODE EXACTLY ///////////////
///////////////////////////////////////////////////////////////////////////	

	min->SetFunction(f);
										// set tolerance , etc... This is in the LArIAT code
	min->SetMaxFunctionCalls(100000); 					// for Minuit/Minuit2 
	min->SetMaxIterations(100000); 	 					// for GSL 
	min->SetTolerance(0.0001);
	min->SetPrintLevel(1);
 
										// Set the free variables to be minimized!
	min->SetVariable(0,"Tau",lifetime, 1);					//Originally had fvariable[1] [2] and [3] for Tau, Sigma, and dqdx0 respectively
	min->SetVariable(1,"Sigma",landau1,1);					//I'm just using the values I found for this event since they are just initial values for minimization
	min->SetVariable(2,"dqdx0",icharge,1);					//Also had fstep[1] " " but I just used 1, which is the same as the fstep array I believe
	min->SetVariableLimits(0,lowtaulimit,hitaulimit);
	min->SetVariableLimits(1,lowsigmalimit,hisigmalimit);
	min->SetVariableLimits(2,lowdqdxolimit,hidqdxolimit);
	min->SetVariableValue(2,icharge);

	try{min->Minimize();} 							// do the minimization
  catch(...){}

	const double *xs = min->X();						//Making sure we actually get values we want
	if( xs[0]!=xs[0] || xs[1]!=xs[1] || xs[2]!=xs[2] ) 
	{
	 if(fVerbose) cout << "MINIMIZATION GAVE NAN VALUE!!!! Skipping the event" << endl; 
	 return -1;
	}

	double tauvec = xs[0];
	if(fVerbose) cout << "LArIAT FUll ANALYSIS LIFETIME IS: " << (tauvec/2000) << "ms" << endl;

	if( xs[0]!=xs[0] || xs[1]!=xs[1] || xs[2]!=xs[2] ) {
	 	if(fVerbose) cout << "MINIMIZATION GAVE NAN VALUE!!!! Skipping the event" << endl; 
	 	return -1;
	}
	// Get Errors from MiNOS
	min->SetErrorDef(0.5);
	for (int l=0; l<3; l++){
		//sigminoserrlow[l] = 0;
		//sigminoserrhi[l] = 0;
	}
	/*min->GetMinosError(0, sigminoserrlow[0], sigminoserrhi[0]);		// Get MINOS errors accordingly.
	min->GetMinosError(1, sigminoserrlow[1], sigminoserrhi[1]); 		// Get MINOS errors accordingly.
	min->GetMinosError(2, sigminoserrlow[2], sigminoserrhi[2]); 		// Get MINOS errors accordingly.*/
	const double *exs = min->Errors();

	if( exs[0]!=exs[0] || exs[1]!=exs[1] || exs[2]!=exs[2] ) 
	{
	 if(fVerbose) cout << "MINIMIZATION GAVE NAN VALUE ON ERRORS!!!! Skipping the event" << endl; 
	 return -1;
	}

	double error_tauvec = exs[0];
	min->Clear();

	double x0 = v_wireCCut[0]*wire_spacing;				//Converting to mm
	double y0 = v_timeCCut[0]*0.5*drift_vel;					//Converting to mm
	double xf = v_wireCCut[v_wireCCut.size()-1]*wire_spacing;				//Converting to mm
	double yf = v_timeCCut[v_timeCCut.size()-1]*0.5*drift_vel;				//Converting to mm
	double length = sqrt(pow((xf-x0),2)+pow((yf-y0),2));			//Length in mm
	double Mangle = atan((yf-y0)/(xf-x0))*180/3.14;				//In Degrees
	taulengthangle=fopen("TTLMA","a");
	fprintf(taulengthangle,"%6.2f %6.2f %6.0f %6.2f %6.2f\n",tauvec/2000,length,Mangle,error_tauvec/2000,lifetime/2000);
	fclose(taulengthangle);

	if(fVerbose) cout << "Lifetime (exp) is: " << (lifetime/2000)  << "ms" << endl << endl << endl << endl;

	if(tauvec < 0 || (tauvec/2000) > 24){return -1;}

  return tauvec;
}
