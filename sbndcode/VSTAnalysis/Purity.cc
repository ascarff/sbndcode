#include "Purity.hh"

/*Adrian Orea: 2018
using Dominic's code and 
https://cdcvs.fnal.gov/redmine/projects/lardbt/repository/revisions/master/entry/LArIATAnaModule/PurityOnlineT1034_module.cc
as references*/

using namespace std;
  // Function to calculate the electron lifetime from a collection of hits
  // You have a vector of recob::Hit objects (you can google recob::Hit to find 
  // out what info is stored in them)

//THIS IS FROM THE LArIAT CODE. It defines the likelihood estimator function.
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
	   while (file.good() && !file.eof())
	   {
	      file >> aa >> bb;
	      dtime.push_back(aa);
	      charge.push_back(bb); 
	   }
	   index=dtime.size()-1;
	   dtime.resize(index);
	   charge.resize(index); 
	   lik=0;
	   for (int ii = 0; ii<index; ++ii)              // Loop over hits
	   { 
	      dqdx=dqdxo*TMath::Exp(-(dtime.at(ii)/tau));
	      mpc = dqdx-mpshift*sigma;                      
	      prob = TMath::Landau(charge.at(ii),mpc,sigma,kTRUE); 
	      lik-=TMath::Log(prob);
	   }     // loop over hits   
	   
	   file.close();
	   dtime.clear();
	   charge.clear();     
	   return lik;
	}

double daqAnalysis::CalculateLifetime(std::vector<art::Ptr<recob::Hit>> rawhits, bool fVerbose){	
	FILE *values_file;							//File where the minimization points are stored
	FILE *taulengthangle;							//File with the Tau values, track length, and angle
	ROOT::Math::Functor f(&likely,3);					//Creates the function based on the Likelihood in likely function
	ROOT::Math::Minimizer* min = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");	//Initializes the object to minimize
	geo::GeometryCore const* geom = lar::providerFrom<geo::Geometry>();	//Looks at geometry of the hit
	double lowtaulimit;     						// lower limit on tau minimization
	double hitaulimit;      						// upper limit on tau minimization
	double lowsigmalimit;   						// lower limit on sigma minimization
	double hisigmalimit;    						// upper limit on sigma minimization
	double lowdqdxolimit;   						// lower limit on dqdxo minimization
	double hidqdxolimit;    						// upper limit on dqdxo minimization
	int mincount=100;
	int minuniqcount=50;
	int rhsize=rawhits.size();
	if (rhsize < mincount){
		if(fVerbose) cout << "Minimum Hits of " << mincount << " not met. Skipping event..." << endl;
		return 1;
	}

	if(fVerbose) cout << "Amount of hits in event is: " << rhsize << ". Passed the minimum hit cut of: " << mincount << endl;
	
										//Sets vectors for hits/information we need.
	vector<float> PTime, Collect, Unique, Integ, IPTime, Induction, New_Integ, New_PTime, New_Collect,Normal_Integ, INew_Induction, INew_PTime,FitTrialTime, FitTrialCharge, FinalCharge, FinalTime;

	if(fVerbose) cout << "Made first couple values, vectors, functor, minimizer, and geometry" << endl;

	Collect.clear(); PTime.clear(); Integ.clear(); Induction.clear(); IPTime.clear();
	for (int i=0; i<rhsize; i++){
		int channel = rawhits[i]->Channel();				//Gets the channel of the specific hit
		geo::SigType_t sigType = geom->SignalType(channel);		//Stores geometry of the hit
		if(sigType == geo::kCollection){				//Checks for Collection plane
			Collect.push_back((*rawhits[i]).WireID().Wire);		//WireID
			PTime.push_back((*rawhits[i]).PeakTime());		//PTime
			Integ.push_back((*rawhits[i]).Integral());		//Charge
		}
		if(sigType == geo::kInduction){					//Checks for Induction plane
			Induction.push_back((*rawhits[i]).WireID().Wire);	//WireID for Induction Plane
			IPTime.push_back((*rawhits[i]).PeakTime());		//PTime for Induction Plane
		}
	} 									//Collecting Values needed (Charge, WireID, PeakTime)

	if(fVerbose) cout << "Filled Collect, PTime, and Integ for Collection plane." << endl;
	if(fVerbose) cout << "Filled Induction and IPTime for Induction plane." << endl;
	
	int n = Collect.size(); 
	int nI = Induction.size();

	if(fVerbose) cout << "Size of Collect vector is: " << n << endl;			//Size of Collection hits	
	if(fVerbose) cout << "Size of Induction vector is: " << nI << endl;			//Size of Induction hits

	Unique.clear();
	Unique.push_back(Collect[0]);						//Unique vector to store only 1 of each hit
	int uniqval = 1;							//Initialize to first element to compare to
        for(int b = 1;b < n; b++){						//Value is unique unless found otherwise
                bool uniq = true;
     	        for(int c = 0; c < uniqval; c++){
             	        if(Collect[b]==Unique[c]){
             	        uniq = false;
         	        }
     	        }
                if(uniq){							//If the value is unique then it adds it to the Unique vector
                        Unique.push_back(Collect[b]);
			uniqval = Unique.size();
                }
        }
	if(uniqval < minuniqcount){						//Minimum Collecection unique hit # cut
		if(fVerbose) cout << "Minimum Unique Hits of " << minuniqcount << " not met. Skipping event..." << endl;
		return 1;
	}
	
	if(fVerbose) cout << "Amount of unique collection plane hits in event is: " << uniqval << ". Passed the minimum unique hit cut of: " << minuniqcount << endl;

	TGraph *WvT = new TGraph(n,&Collect[0],&PTime[0]);			//Two graphs for Wire and PTime
	TGraph *IWvT = new TGraph(nI,&Induction[0],&IPTime[0]);			//Need the & and [0] notation in order for TGraph to take a vector instead of an array
	if(fVerbose) cout << "Made PTime vs WireID for Collection and Induction planes" << endl;
	
	float minx = *max_element(begin(Collect),end(Collect));			//Getting min and max of the Colletion Wire hits
	float maxx = *min_element(begin(Collect),end(Collect));									
	float Iminx = *max_element(begin(Induction),end(Induction));		//Getting min and max of the Induction Wire hits
	float Imaxx = *min_element(begin(Induction),end(Induction));

	TF1 *fFunc = new TF1("Linear","[0]*x+[1]", minx, maxx);			//(Collection)Two parameters are slope and y-intercept.
	TF1 *fIFunc = new TF1("ILinear","[0]*x+[1]", Iminx, Imaxx);		//(Induction)Two parameters are slope and y-intercept

	if(fVerbose) cout << "Made functions for Collection and Induction Planes" << endl;

	fFunc->SetParNames("Slope","Peak Time Intercept");			//Initializing for collection. Completely Arbitrary
	fFunc->SetParameters(1,1);						//Setting slope and intercept to 1

	fIFunc->SetParNames("Slope1","Peak Time Intercept 1");			//Initializing for induction. Completely Arbitrary
	fIFunc->SetParameters(1,1);
	if(fVerbose) cout << "Initialized parameters for fits on Wvt and IWvt" << endl;

	WvT->Fit("Linear","NQ");							//Does the fitting for Collection plane
	IWvT->Fit("ILinear","NQ");						//Does the fitting for Induction plane

	if(fVerbose) cout << "Completed first fitting of Collection and Induction planes" << endl;
	
										//Collection Plane
   	Double_t chisqr = fFunc->GetChisquare()/fFunc->GetNDF();  		// Obtain chi^2 divided by number of degrees of freedom
   	if(chisqr!=chisqr) return 1;                   				// Skip the event if the chi square is a nan value

	float slope = fFunc->GetParameter(0);
	float yint = fFunc->GetParameter(1);
	if(fVerbose) cout << "First Collection slope: " << slope << "\n First Collection y-intercept: " << yint << endl;

	TF1 *ofFunc = new TF1("oLinear","[0]*x+[1]", minx, maxx);
	ofFunc->SetParameters(slope,yint);

	float Islope = fIFunc->GetParameter(0);
	float Iyint = fIFunc->GetParameter(1);
	if(fVerbose) cout << "First Induction slope: " << Islope << "\n First Induction y-intercept: " << Iyint << endl;

	TF1 *ofIFunc = new TF1("oILinear","[0]*x+[1]", Iminx, Imaxx);
	ofIFunc->SetParameters(Islope,Iyint);

	float fitdev = 0.02;							//Originally a set distance, now just a percentage.
	if(fVerbose) cout << "Set the value of deviation percentage cut" << endl;
	double fitvalue;
	int Newn;
	int INewn;
	TGraph *New_WvT;
	TGraph *INew_WvT;
	//Collection
	for(int z=0; z<2; z++){					  		// make fit twice to make sure to pick up every hit
		New_PTime.clear();
		New_Integ.clear();
		New_Collect.clear();
		for(int i=0; i< n; i++){  					//select only hits close to the fit
	 		fitvalue = slope*Collect[i]+yint;
	 		if(abs((PTime[i]-fitvalue)/fitvalue)<fitdev){
	    			New_PTime.push_back(PTime[i]);
	    			New_Integ.push_back(Integ[i]);
	   			New_Collect.push_back(Collect[i]);
	 		}
		}
		Newn = New_Integ.size();
		New_WvT = new TGraph(Newn,&New_Collect[0],&New_PTime[0]);
		New_WvT->Fit("Linear","NQ");
	}
	slope = fFunc->GetParameter(0);						//Updating the slope value for the new fit
	if(fVerbose) cout << "Refitted Collection slope is: " << slope << endl;

	//Induction
	for(int q=0; q<2; q++){					  		// make fit twice to make sure to pick up every hit
		INew_PTime.clear();
		INew_Induction.clear();
		for(int i=0; i< nI; i++){  					//select only hits close to the fit
	 		fitvalue = Islope*Induction[i]+Iyint;
	 		if(abs((IPTime[i]-fitvalue)/fitvalue)<fitdev){
	    			INew_PTime.push_back(IPTime[i]);
	   			INew_Induction.push_back(Induction[i]);
	 		}
		}
		INewn = INew_PTime.size();
		INew_WvT = new TGraph(INewn,&INew_Induction[0],&INew_PTime[0]);
		INew_WvT->Fit("ILinear","NQ");
	} 
	Islope = fIFunc->GetParameter(0);						//Updating the slope value for the new fit
	if(fVerbose) cout << "Refitted Induction slope is: " << Islope << endl;

										//Pretty much all of the parts prior to fitting from here are from Dom's code
 	float shaping_time = 2;							//In microseconds. Given by Tom
	float drift_vel =  1.50638;						//In mm/microsecond. Calculated by Tom
	float wire_spacing = 4;							//In mm. Assumed
	//float angle = atan((shaping_time*drift_vel)/wire_spacing);
	//if(fVerbose) cout << "Dom's angle cut is at: " << angle << endl;
	float us2tick = 0.5;
	float tick_to_wire = 0.256;						//From Dom's code. Not explicitly in Tom's email...
	slope *= tick_to_wire;
	Islope*= tick_to_wire;
	if(fVerbose) cout << "Converted Slope " << slope << endl;
	if(fVerbose) cout << "Converted ISlope " << Islope << endl;

	float theta = atan(abs(slope/Islope));					//Dom has a "toofewhits" cut but I don't know what this is. It's not in the code. So I assume it isn't needed...
	float phi = atan(abs(1/slope));
	if(fVerbose) cout << "The value of theta is: " << theta << endl;
	if(fVerbose) cout << "The value of phi is: " << phi << endl;
	
	if(slope<0){								//In Dom's code
		phi = 3.14159 - phi;
	}
	//if((abs(phi-1.57080)<angle)){						//In Dom's code
	//	if(fVerbose) cout << "Dom's Angle cut. Skipping event..." << endl;
	//	//return 1;
	//}
		
	float normalization = abs(cos(theta)*sin(phi));				//This is part of the normalization factor for the charges	
	Normal_Integ.resize(Newn);						//Making the vector the same size as New_Integ
	transform(New_Integ.begin(), New_Integ.end(), Normal_Integ.begin(),bind(multiplies<float>(),std::placeholders::_1,normalization*(1/(sqrt(6.28319)*shaping_time*us2tick))));
										//^^^^^^^Normalization^^^^^^^
	TGraph *NormalIvT = new TGraph(Newn, &New_PTime[0],&Normal_Integ[0]); 	//Graph like IvT but normalized
	
	Double_t NewPTminx = *min_element(begin(New_PTime),end(New_PTime));	//Getting min of the Collection PTime hits
	Double_t NewPTmaxx = *max_element(begin(New_PTime),end(New_PTime));	//Getting max of the Collection PTime hits
	
	TF1 *fNewFunc = new TF1("New_Int","expo", NewPTminx, NewPTmaxx);	//Exponential plot for fitting NormalIvT
	NormalIvT->Fit("New_Int","NQ");						//Fitting NormalIvT with fNewFunc

	double expo0 = fNewFunc->GetParameter(0);				//Getting the "Normalization constant"
	double expo1 = fNewFunc->GetParameter(1);				//Decay rate
	if(fVerbose) cout << "The 'normalization' constant is: " << expo0 << endl;
	if(fVerbose) cout << "The decay rate is: " << expo1 << endl;

	TH1D *h1 = new TH1D("h1","Charge Distance", 200, -4000, 4000); 		//Pretty much a copy and paste from here with my code
	
	for(int y = 0; y<Newn; y++){
		h1->Fill(Normal_Integ[y]-exp(expo0+(expo1*New_PTime[y])));
	}
	
	TF1 *gfun = new TF1("gfun","landau", -2000, 4000);			//From LArIAT code
	h1->Fit("gfun","NQ");							//Fitting an Landau to the histogram
	double landau1=gfun->GetParameter(2);					//Sigma of the Landau distribution
	if(fVerbose) cout << "Sigma of the Original Landau fit is: " << landau1 << endl;

	double lowcut;
	double hicut;
	Double_t FitTrialTimeminx;
	Double_t FitTrialTimemaxx;
	TGraph *FitRe;
	TH1D *h2;
	if(fVerbose) cout << "Defined the parameters" << endl;
//THIS IS THE PART THAT ACTUALLY MAKES THE CUT					//This part will refit the exponential between +/- #sigma
	int FitRen;
	for(int r=0; r<2;r++){
		FitTrialCharge.clear();
		FitTrialTime.clear();
		if(expo1 < 0.0){ 						//Charge should decay
			if(fVerbose) cout << "Cleared the two FitTrial vectors" << endl;
			for (int i = 0; i<Newn; i++){

				if((4*landau1)<1200){
					hicut=exp(expo0+(expo1*New_PTime[i]))+(3.0*landau1);
					//lowcut=exp(expo0+(expo1*New_PTime[i]))-(6.0*landau1);
					lowcut=0;
				}
				else{ 
					lowcut=exp(expo0+(expo1*New_PTime[i]))-1200;
					hicut=exp(expo0+(expo1*New_PTime[i]))+1200;
				}
				if(Normal_Integ[i] > lowcut && Normal_Integ[i] < hicut){
					FitTrialCharge.push_back(Normal_Integ[i]);
					FitTrialTime.push_back(New_PTime[i]);
				
				}
			}
			FitRen = FitTrialTime.size();
			if(fVerbose) cout << "The size of FitTrialTime is " << FitRen << endl;
			FitRe = new TGraph(FitRen,&FitTrialTime[0],&FitTrialCharge[0]);
			FitRe->Fit("New_Int","NQ");
			expo0 = fNewFunc->GetParameter(0);			//Getting the "Normalization constant"
			expo1 = fNewFunc->GetParameter(1);			//Decay rate
			h2 = new TH1D("h2","Charge Distance", 200, -4000, 4000); //New histogram
			if(r==1) h2->Reset();
			for(int y=0; y<FitRen; y++){
				h2->Fill(FitTrialCharge[y]-exp(expo0+(expo1*FitTrialTime[y])));
			}
			h2->Fit("gfun","NQ");
			landau1=gfun->GetParameter(2);
		}
			//else{
			//	if(fVerbose) cout << "The exponential rate is positive. Skipping event..." << endl;
			//	return 1;
			//}
	}
	FitTrialTimeminx = *min_element(begin(FitTrialTime),end(FitTrialTime));
	FitTrialTimemaxx = *max_element(begin(FitTrialTime),end(FitTrialTime));
	if(fVerbose) cout << "Smallest value is: " << FitTrialTimeminx << endl;
	if(fVerbose) cout << "Largest value is: " << FitTrialTimemaxx << endl;		//Decay rate
	if(fVerbose) cout << "The refitted 'normalization' constant is: " << expo0 << endl;
	if(fVerbose) cout << "The refitted decay rate is: " << expo1 << endl;
	if(fVerbose) cout << "Sigma of the Refitted Landau fit is: " << landau1 << endl;

//THIS PART CHANGES THE ACTUAL CUT
	TGraph *FinalFit;
	int Finaln;
	for(int u=0;u<2;u++){	
		if(expo1 < 0.0){ 						//Charge should decay
			FinalCharge.clear();
			FinalTime.clear();
			values_file=fopen("tmpdontcareaboutthis.txt","w");
			for (int i = 0; i<FitRen; i++){
				if((4*landau1)<1200){
					hicut	=exp(expo0+(expo1*FitTrialTime[i]))+(1.5*landau1);
					//lowcut=exp(expo0+(expo1*FitTrialTime[i]))-(5.0*landau1);
					lowcut=0;
				}
				else{ 
					lowcut=exp(expo0+(expo1*FitTrialTime[i]))-1200;
					hicut=exp(expo0+(expo1*FitTrialTime[i]))+1200;
				}
				if(FitTrialCharge[i] > lowcut && FitTrialCharge[i] < hicut){
					FinalCharge.push_back(FitTrialCharge[i]);
					FinalTime.push_back(FitTrialTime[i]);
					fprintf(values_file,"%6.2f %6.0f\n",FitTrialTime[i],FitTrialCharge[i]);
				}
				if(i==(FitRen-1)) fprintf(values_file,"////////////////////////");
			}
			Finaln = FinalTime.size();
			FinalFit = new TGraph(Finaln,&FinalTime[0],&FinalCharge[0]);
			FinalFit->Fit("New_Int","NQ");
			expo0 = fNewFunc->GetParameter(0);			//Getting the "Normalization constant"
			expo1 = fNewFunc->GetParameter(1);			//Decay rate
			h2 = new TH1D("h2","Charge Distance", 200, -4000, 4000); //New histogram
			h2->Reset();
			for(int y=0; y<Finaln; y++){
				h2->Fill(FinalCharge[y]-exp(expo0+(expo1*FinalTime[y])));
			}
			h2->Fit("gfun","NQ");
			landau1=gfun->GetParameter(2);
			fclose(values_file);
		}
	}
	//if(fVerbose) cout << "Size of values in final cut is: " << Finaln << endl;

//AGAIN, THIS PART ONLY CHANGES THE GRAPHING SECTION
	TF1 *p4sig;								//Create the Function +4sig defined later below
	TF1 *m4sig;								//Create the Function -4sig defined later below

	if((4*landau1)<1200){
		p4sig = new TF1("p4sig", "exp([0]+([1]*x))+1.5*[2]",FitTrialTimeminx,FitTrialTimemaxx);
		m4sig = new TF1("m4sig", "0",FitTrialTimeminx,FitTrialTimemaxx);
		//m4sig = new TF1("m4sig", "exp([0]+([1]*x))-5.0*[2]",FitTrialTimeminx,FitTrialTimemaxx);
	}else{
		p4sig = new TF1("p4sig", "exp([0]+([1]*x))+1200",FitTrialTimeminx,FitTrialTimemaxx);
		m4sig = new TF1("m4sig", "exp([0]+([1]*x))-1200",FitTrialTimeminx,FitTrialTimemaxx);                     
	}

	p4sig->SetParameter(0,expo0);						//I'm going off of the website and using +/- 4*sigma 
	p4sig->SetParameter(1,expo1);
	p4sig->SetParameter(2,landau1);
	m4sig->SetParameter(0,expo0);
	m4sig->SetParameter(1,expo1);
	m4sig->SetParameter(2,landau1);

	if(fVerbose) cout << "The final refitted 'normalization' constant is: " << expo0 << endl;
	if(fVerbose) cout << "The final refitted decay rate is: " << expo1 << endl;
	float lifetime = 1/(-1*expo1);						//The lifetime of the exponential fit
	double icharge = exp(expo0);						//Initial charge dQ/dx0
///////////////////////////////////////////////////////////////////////////
////////////////////// THIS IS FROM THE LArIAT CODE EXACTLY ///////////////
///////////////////////////////////////////////////////////////////////////	

	min->SetFunction(f);
										// set tolerance , etc... This is in the LArIAT code
	min->SetMaxFunctionCalls(10000000); 					// for Minuit/Minuit2 
	min->SetMaxIterations(10000); 	 					// for GSL 
	min->SetTolerance(0.0001);
	min->SetPrintLevel(1);
 
										// Set the free variables to be minimized!
	min->SetVariable(0,"Tau",lifetime, 1);					//Originally had fvariable[1] [2] and [3] for Tau, Sigma, and dqdx0 respectively
	min->SetVariable(1,"Sigma",landau1,1);					//I'm just using the values I found for this event since they are just initial values for minimization
	min->SetVariable(2,"dqdx0",icharge,1);					//Also had fstep[1] " " but I just used 1, which is the same as the fstep array I believe
	lowtaulimit=10;     							// lower limit on tau minimization
	hitaulimit=50000;      							// upper limit on tau minimization
	lowsigmalimit=0;  							// lower limit on sigma minimization
	hisigmalimit=6000;    							// upper limit on sigma minimization
	lowdqdxolimit=100;   							// lower limit on dqdxo minimization
	hidqdxolimit=40000;    							// upper limit on dqdxo minimization
	min->SetVariableLimits(0,lowtaulimit,hitaulimit);
	min->SetVariableLimits(1,lowsigmalimit,hisigmalimit);
	min->SetVariableLimits(2,lowdqdxolimit,hidqdxolimit);
	min->SetVariableValue(2,icharge);

	try{min->Minimize();} 							// do the minimization
  catch(...){}

	const double *xs = min->X();						//Making sure we actually get values we want
	if( xs[0]!=xs[0] || xs[1]!=xs[1] || xs[2]!=xs[2] ) 
	{
	 if(false) if(fVerbose) cout << "MINIMIZATION GAVE NAN VALUE!!!! Skipping the event" << endl; 
	 return 1;
	}

	double tauvec = xs[0];
	if(fVerbose) cout << "LArIAT FUll ANALYSIS LIFETIME IS: " << (tauvec/2000) << "ms" << endl;

	double x0 = New_Collect[0]*wire_spacing;				//Converting to mm
	double y0 = New_PTime[0]*0.5*drift_vel;					//Converting to mm
	double xf = New_Collect[Newn-1]*wire_spacing;				//Converting to mm
	double yf = New_PTime[Newn-1]*0.5*drift_vel;				//Converting to mm
	double length = sqrt(pow((xf-x0),2)+pow((yf-y0),2));			//Length in mm
	double Mangle = atan((yf-y0)/(xf-x0))*180/3.14;				//In Degrees
	taulengthangle=fopen("TTLMA","a");
	fprintf(taulengthangle,"%6.2f %6.2f %6.0f\n",tauvec,length,Mangle);
	fclose(taulengthangle);


////////////////////////////////////////////////////////////////////
////////////////////// JUST PLOTTING STUFF /////////////////////////
////////////////////////////////////////////////////////////////////
/*
	TCanvas *cWvT  = new TCanvas("WvT","Peaktime vs WireID",10,10,800,600); 	//Creating canvases to draw onto. Purely for visual inspection
	TCanvas *cIWvT  = new TCanvas("IWvT","Peaktime vs WireID",10,10,800,600);
	TCanvas *cIvT  = new TCanvas("IvT","Integral vs WireID",10,10,800,600);	
	//TCanvas *cTrial  = new TCanvas("Trial","Trial",10,10,800,600);	
	cWvT->cd();
		gStyle->SetOptStat(0);
		WvT->SetMarkerStyle(3);
		WvT->GetXaxis()->SetTitle("WireID");
		WvT->GetYaxis()->SetTitle("PeakTime");
		WvT->SetTitle("Collection Plane");
		WvT->Draw("AP"); 							//Data.No Connecting Line
		
		New_WvT->SetMarkerStyle(29);
		New_WvT->SetMarkerColor(kMagenta);
		New_WvT->Draw("SAME,AP");

		ofFunc->SetLineColor(kBlue);						//Original Fit
		ofFunc->Draw("SAME");

		fFunc->SetLineColor(kGreen);						//Refit. Also, the color code will be followed
		fFunc->Draw("SAME");
	cWvT->SaveAs("Peak.root");

	cIWvT->cd();
		gStyle->SetOptStat(0);
		IWvT->SetMarkerStyle(3);
		IWvT->GetXaxis()->SetTitle("WireID");
		IWvT->GetYaxis()->SetTitle("PeakTime");
		IWvT->SetTitle("Induction Plane");
		IWvT->Draw("AP"); 							//Data. No Connecting Line

		ofIFunc->SetLineColor(kBlue);						//Original Fit
		ofIFunc->Draw("SAME");

		fIFunc->SetLineColor(kGreen);						//Refit
		fIFunc->Draw("SAME");

	cIWvT->SaveAs("IPeak.root");

  
	cIvT->cd();
		gStyle->SetOptStat(0);
		NormalIvT->GetXaxis()->SetTitle("Peak Time");
		NormalIvT->GetXaxis()->SetRangeUser(0,800);
		NormalIvT->GetYaxis()->SetTitle("Integral");
		NormalIvT->GetYaxis()->SetRangeUser(0,1500);
		NormalIvT->SetMarkerStyle(3);
		NormalIvT->Draw("AP");							//Data. No connecting line

		Final->SetMarkerStyle(29);
		Final->SetMarkerColor(kMagenta);
		Final->Draw("SAME");

		FfNewFunc->SetLineColor(kGreen);
		FfNewFunc->Draw("SAME");
		p4sig->SetLineColor(kGreen);
		p4sig->Draw("SAME");
		m4sig->SetLineColor(kGreen);
		m4sig->Draw("SAME");							//Second refit with +/-#Sigma

		fNewFunc->SetLineColor(kOrange);
		fNewFunc->SetLineStyle(10);
		fNewFunc->Draw("SAME");							//Final Refit
	cIvT->SaveAs("Integral.root");	
	
	cTrial->cd();
		gStyle->SetOptStat(0);
		h1->Draw();
		ogfun->SetLineColor(kBlue);
		ogfun->Draw("SAME");

		gfun->SetLineColor(kGreen);
		gfun->Draw("SAME");
	cTrial->SaveAs("Trial.root");

	if(fVerbose) cout << "Plotted All canvases" << endl;
	if(fVerbose) cout << "Lifetime is: " << (lifetime/2000)  << "ms" << endl;
	if(fVerbose) cout << endl << endl << endl << endl;
*/
  return tauvec;
}
