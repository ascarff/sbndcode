////////////////////////////////////////////////////////////////////////
//
//  LArPropertiesOpLib_plugin
//
////////////////////////////////////////////////////////////////////////


// C++ language includes
#include <cmath>
#include <iostream>

// LArSoft includes
#include "lar1ndcode/Utilities/LArPropertiesOpLib.h"
#include "SimpleTypesAndConstants/PhysicalConstants.h"
#include "Utilities/DatabaseUtil.h"

// ROOT includes
#include "TMath.h"
#include "TSpline.h"
#include "TH1.h"
#include <Rtypes.h>
// Framework includes
#include "art/Framework/Principal/Run.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"
//-----------------------------------------------
util::LArPropertiesOpLib::LArPropertiesOpLib(fhicl::ParameterSet const& pset, art::ActivityRegistry &reg)
   //fval(0),
 //fElectronlifetime(  ( )
{
  this->reconfigure(pset);
  reg.sPreBeginRun.watch(this, &LArPropertiesOpLib::preBeginRun);
}

//------------------------------------------------
util::LArPropertiesOpLib::~LArPropertiesOpLib()
{
}

//----------------------------------------------
void util::LArPropertiesOpLib::preBeginRun(art::Run const& run)
{
  int nrun = run.id().run();
  art::ServiceHandle<util::DatabaseUtil> DButil;
  if (nrun != 0){

    double inpvalue = 0.;

    //get lifetime for a given run. If it doesn't work return to default value.
    if(DButil->GetLifetimeFromDB(nrun,inpvalue)!=-1)
      fElectronlifetime=inpvalue;

    //get temperature for a given run. If it doesn't work return to default value.
    if(DButil->GetTemperatureFromDB(nrun,inpvalue)!=-1)
      fTemperature=inpvalue;

    //get Efield vlaues for a given run. If it doesn't work return to default value.
    DButil->GetEfieldValuesFromDB(nrun,fEfield);

  }
  else
    mf::LogWarning("LArPropertiesOpLib") << "run number == 0, not extracting info from DB\n" ;

  fAlreadyReadFromDB=true;
}



//------------------------------------------------
/// \todo these values should eventually come from a database
void util::LArPropertiesOpLib::reconfigure(fhicl::ParameterSet const& pset)
{
  fEfield            = pset.get< std::vector<double> >("Efield"          );
  fTemperature       = pset.get< double              >("Temperature"     );
  fElectronlifetime  = pset.get< double              >("Electronlifetime");
  fRadiationLength   = pset.get< double              >("RadiationLength" );
  fZ                 = pset.get< double              >("AtomicNumber"    );
  fA                 = pset.get< double              >("AtomicMass"      );
  fI                 = pset.get< double              >("ExcitationEnergy");
  fSa                = pset.get< double              >("SternheimerA"    );
  fSk                = pset.get< double              >("SternheimerK"    );
  fSx0               = pset.get< double              >("SternheimerX0"   );
  fSx1               = pset.get< double              >("SternheimerX1"   );
  fScbar             = pset.get< double              >("SternheimerCbar" );

  fArgon39DecayRate  = pset.get< double              >("Argon39DecayRate");

  fFastScintEnergies = pset.get< std::vector<double> >("FastScintEnergies");
  fFastScintSpectrum = pset.get< std::vector<double> >("FastScintSpectrum");
  fSlowScintEnergies = pset.get< std::vector<double> >("SlowScintEnergies");
  fSlowScintSpectrum = pset.get< std::vector<double> >("SlowScintSpectrum");
  fAbsLengthEnergies = pset.get< std::vector<double> >("AbsLengthEnergies");
  fAbsLengthSpectrum = pset.get< std::vector<double> >("AbsLengthSpectrum");
  fRIndexEnergies    = pset.get< std::vector<double> >("RIndexEnergies"   );
  fRIndexSpectrum    = pset.get< std::vector<double> >("RIndexSpectrum"   );
  fRayleighEnergies  = pset.get< std::vector<double> >("RayleighEnergies" );
  fRayleighSpectrum  = pset.get< std::vector<double> >("RayleighSpectrum" );

  fScintResolutionScale = pset.get<double>("ScintResolutionScale");
  fScintFastTimeConst   = pset.get<double>("ScintFastTimeConst"  );
  fScintSlowTimeConst   = pset.get<double>("ScintSlowTimeConst"  );
  fScintBirksConstant   = pset.get<double>("ScintBirksConstant"  );
  fScintByParticleType  = pset.get<bool>("ScintByParticleType"   );
  fScintYield           = pset.get<double>("ScintYield"          );
  fScintYieldRatio      = pset.get<double>("ScintYieldRatio"     );
  fExtraMatProperties      = pset.get<bool>("LoadExtraMatProperties"     );
  fSimpleBoundary     = pset.get<bool>("SimpleBoundaryProcess");
  fSimpleScint     = pset.get<bool>("SimpleScintillation");
  if(fScintByParticleType){
    fProtonScintYield        = pset.get<double>("ProtonScintYield"     );
    fProtonScintYieldRatio   = pset.get<double>("ProtonScintYieldRatio");
    fMuonScintYield          = pset.get<double>("MuonScintYield"       );
    fMuonScintYieldRatio     = pset.get<double>("MuonScintYieldRatio"  );
    fPionScintYield          = pset.get<double>("PionScintYield"       );
    fPionScintYieldRatio     = pset.get<double>("PionScintYieldRatio"  );
    fKaonScintYield          = pset.get<double>("KaonScintYield"       );
    fKaonScintYieldRatio     = pset.get<double>("KaonScintYieldRatio"  );
    fElectronScintYield      = pset.get<double>("ElectronScintYield"   );
    fElectronScintYieldRatio = pset.get<double>("ElectronScintYieldRatio");
    fAlphaScintYield         = pset.get<double>("AlphaScintYield"      );
    fAlphaScintYieldRatio    = pset.get<double>("AlphaScintYieldRatio" );
  }
  
if(fExtraMatProperties){
// Used data to be found e.g. in:  JINST 7 P05008 (reflectances estimated from measurements at Cracow University of Technology (thanks to dr. J. Jaglarz and dr. N. Nosidlak) + http://refractiveindex.info and refs. therein), G.M. Seidel, et al.,Nucl. Instr. and Meth. A 489 (2002)189; arXiv:1108.5584 [physics.ins-det]; Journal of Luminescence 81 (1999) 285}291;  arXiv:1304.6117v3 [physics.ins-det]; „Optical characterization and GEANT4 simulation of the light collection system for the WArP 100 liters detector: analysis of the event reconstruction capability”, F. Di Pompeo PhD thesis; //http://gentitfx.fr/litrani/AllModules/FitMacros/RIndexRev_vm2000.C.html for vm2000 (VM2000 (TM)) and refs. Therein - list will be updated for reference
//std::cout<<"EXTRA MATERIAL PROPERTIES BEING LOADED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
fTpbTimeConstant      = pset.get<double>("TpbTimeConstant"     );     
  fTpbEmmisionEnergies       = pset.get<std::vector<double> >              ("TpbEmmisionEnergies"        );
  fTpbEmmisionSpectrum        = pset.get<std::vector<double> >              ("TpbEmmisionSpectrum"        );
  fTpbAbsorptionEnergies        = pset.get<std::vector<double> >              ("TpbAbsorptionEnergies"        );
  fTpbAbsorptionSpectrum        = pset.get<std::vector<double> >              ("TpbAbsorptionSpectrum"        );
fReflectiveSurfaceTpbNames           = pset.get<std::vector<std::string> >         ("ReflectiveSurfaceTpbNames"           );
  fReflectiveSurfaceTpbEnergies        = pset.get<std::vector<double> >              ("ReflectiveSurfaceTpbEnergies"        );
  fReflectiveSurfaceTpbReflectances    = pset.get<std::vector<std::vector<double> > >("ReflectiveSurfaceTpbReflectances"    );
  fReflectiveSurfaceTpbDiffuseFractions= pset.get<std::vector<std::vector<double> > >("ReflectiveSurfaceTpbDiffuseFractions");
}

  fEnableCerenkovLight  = pset.get<bool>("EnableCerenkovLight"       );

  fReflectiveSurfaceNames           = pset.get<std::vector<std::string> >         ("ReflectiveSurfaceNames"           );
  fReflectiveSurfaceEnergies        = pset.get<std::vector<double> >              ("ReflectiveSurfaceEnergies"        );
  fReflectiveSurfaceReflectances    = pset.get<std::vector<std::vector<double> > >("ReflectiveSurfaceReflectances"    );
  fReflectiveSurfaceDiffuseFractions= pset.get<std::vector<std::vector<double> > >("ReflectiveSurfaceDiffuseFractions");

  fAlreadyReadFromDB=false;

  return;
}

//------------------------------------------------------------------------------------//
// temperature is assumed to be in degrees Kelvin
// density is nearly a linear function of temperature.  see the NIST tables for details
// slope is between -6.2 and -6.1, intercept is 1928 kg/m^3
// this parameterization will be good to better than 0.5%.
// density is returned in g/cm^3
double util::LArPropertiesOpLib::Density(double temperature) const
{
  // Default temperature use internal value.
  if(temperature == 0.)
    temperature = Temperature();

  double density = -0.00615*temperature + 1.928;

  return density;
}


//------------------------------------------------------------------------------------//
double util::LArPropertiesOpLib::Efield(unsigned int planegap) const
{
  this->checkDBstatus();

  if(planegap >= fEfield.size())
    throw cet::exception("LArPropertiesOpLib") << "requesting Electric field in a plane gap that is not defined";

  return fEfield[planegap];
}

//------------------------------------------------------------------------------------//
double util::LArPropertiesOpLib::Temperature() const
{
  this->checkDBstatus();
  return fTemperature;
}

//------------------------------------------------------------------------------------//
double util::LArPropertiesOpLib::ElectronLifetime() const
{
  this->checkDBstatus();
  return fElectronlifetime;

}




//------------------------------------------------------------------------------------//
double util::LArPropertiesOpLib::DriftVelocity(double efield, double temperature) const {

  // Drift Velocity as a function of Electric Field and LAr Temperature
  // from : W. Walkowiak, NIM A 449 (2000) 288-294
  //
  // Efield should have units of kV/cm
  // Temperature should have units of Kelvin

  // Default Efield, use internal value.
  if(efield == 0.)
    efield = Efield();
  //
  if(efield > 4.0)
    mf::LogWarning("LArPropertiesOpLib") << "DriftVelocity Warning! : E-field value of "
				    << efield
				    << " kV/cm is outside of range covered by drift"
				    << " velocity parameterization. Returned value"
				    << " may not be correct";


  // Default temperature use internal value.
  if(temperature == 0.)
    temperature = Temperature();

  if(temperature < 87.0 || temperature > 94.0)
    mf::LogWarning("LArPropertiesOpLib") << "DriftVelocity Warning! : Temperature value of "
				    << temperature
				    << " K is outside of range covered by drift velocity"
				    << " parameterization. Returned value may not be"
				    << " correct";




  double tshift = -87.203+temperature;
  double xFit = 0.0938163-0.0052563*tshift-0.0001470*tshift*tshift;
  double uFit = 5.18406+0.01448*tshift-0.003497*tshift*tshift-0.000516*tshift*tshift*tshift;
  double vd;


// Icarus Parameter Set, use as default
  double  P1 = -0.04640; // K^-1
  double  P2 = 0.01712;  // K^-1
  double  P3 = 1.88125;   // (kV/cm)^-1
  double  P4 =  0.99408;    // kV/cm
  double  P5 =  0.01172;   // (kV/cm)^-P6
  double  P6 =  4.20214;
  double  T0 =  105.749;  // K
      // Walkowiak Parameter Set
  double    P1W = -0.01481; // K^-1
  double  P2W = -0.0075;  // K^-1
  double   P3W =  0.141;   // (kV/cm)^-1
  double   P4W =  12.4;    // kV/cm
  double   P5W =  1.627;   // (kV/cm)^-P6
  double   P6W =  0.317;
  double   T0W =  90.371;  // K

// From Craig Thorne . . . currently not documented
// smooth transition from linear at small fields to 
//     icarus fit at most fields to Walkowiak at very high fields
   if (efield < xFit) vd=efield*uFit;
   else if (efield<0.619) { 
     vd = ((P1*(temperature-T0)+1)
	       *(P3*efield*std::log(1+P4/efield) + P5*std::pow(efield,P6))
	       +P2*(temperature-T0));
   }
   else if (efield<0.699) {
     vd = 12.5*(efield-0.619)*((P1W*(temperature-T0W)+1)
	       *(P3W*efield*std::log(1+P4W/efield) + P5W*std::pow(efield,P6W))
	       +P2W*(temperature-T0W))+
       12.5*(0.699-efield)*((P1*(temperature-T0)+1)
	       *(P3*efield*std::log(1+P4/efield) + P5*std::pow(efield,P6))
	       +P2*(temperature-T0));
   }
   else {
     vd = ((P1W*(temperature-T0W)+1)
	       *(P3W*efield*std::log(1+P4W/efield) + P5W*std::pow(efield,P6W))
	       +P2W*(temperature-T0W));     
   }

  vd /= 10.;

  return vd; // in cm/us
}

//----------------------------------------------------------------------------------
// The below function assumes that the user has applied the lifetime correction and
// effective pitch between the wires (usually after 3D reconstruction). Using with
// mean wire pitch will not give correct results.
// parameters:
//  dQdX in electrons/cm, charge (amplitude or integral obtained) divided by effective pitch for a given 3D track.
// returns dEdX in MeV/cm
double util::LArPropertiesOpLib::BirksCorrection(double dQdx) const
{
  // Correction for charge quenching using parameterization from
  // S.Amoruso et al., NIM A 523 (2004) 275

  double  A3t    = util::kRecombA;
  double  K3t    = util::kRecombk;                     // in KV/cm*(g/cm^2)/MeV
  double  rho    = this->Density();                    // LAr density in g/cm^3
  double Wion    = 1000./util::kGeVToElectrons;        // 23.6 eV = 1e, Wion in MeV/e
  double Efield  = this->Efield();                     // Electric Field in the drift region in KV/cm
  K3t           /= rho;                                // KV/MeV
  double dEdx    = dQdx/(A3t/Wion-K3t/Efield*dQdx);    //MeV/cm

  return dEdx;
}

// Modified Box model correction 
double util::LArPropertiesOpLib::ModBoxCorrection(double dQdx) const
{
  // Modified Box model correction has better behavior than the Birks
  // correction at high values of dQ/dx.
  double  rho    = this->Density();                    // LAr density in g/cm^3
  double Wion    = 1000./util::kGeVToElectrons;        // 23.6 eV = 1e, Wion in MeV/e
  double Efield  = this->Efield();                     // Electric Field in the drift region in KV/cm
  double Beta    = util::kModBoxB / (rho * Efield);
  double Alpha   = util::kModBoxA;
  double dEdx = (exp(Beta * Wion * dQdx ) - Alpha) / Beta;
  
  return dEdx;

}

//----------------------------------------------------------------------------------
// Restricted mean energy loss (dE/dx) in units of MeV/cm.
//
// For unrestricted mean energy loss, set tcut = 0, or tcut large.
//
// Arguments:
//
// mom  - Momentum of incident particle in GeV/c.
// mass - Mass of incident particle in GeV/c^2.
// tcut - Maximum kinetic energy of delta rays (MeV).
//
// Returned value is positive.
//
// Based on Bethe-Bloch formula as contained in particle data book.
// Material parameters (stored in larproperties.fcl) are taken from
// pdg web site http://pdg.lbl.gov/AtomicNuclearProperties/
//
double util::LArPropertiesOpLib::Eloss(double mom, double mass, double tcut) const
{
  // Some constants.

  double K = 0.307075;     // 4 pi N_A r_e^2 m_e c^2 (MeV cm^2/mol).
  double me = 0.510998918; // Electron mass (MeV/c^2).

  // Calculate kinematic quantities.

  double bg = mom / mass;           // beta*gamma.
  double gamma = sqrt(1. + bg*bg);  // gamma.
  double beta = bg / gamma;         // beta (velocity).
  double mer = 0.001 * me / mass;   // electron mass / mass of incident particle.
  double tmax = 2.*me* bg*bg / (1. + 2.*gamma*mer + mer*mer);  // Maximum delta ray energy (MeV).

  // Make sure tcut does not exceed tmax.

  if(tcut == 0. || tcut > tmax)
    tcut = tmax;

  // Calculate density effect correction (delta).

  double x = std::log10(bg);
  double delta = 0.;
  if(x >= fSx0) {
    delta = 2. * std::log(10.) * x - fScbar;
    if(x < fSx1)
      delta += fSa * std::pow(fSx1 - x, fSk);
  }

  // Calculate stopping number.

  double B = 0.5 * std::log(2.*me*bg*bg*tcut / (1.e-12 * fI*fI))
    - 0.5 * beta*beta * (1. + tcut / tmax) - 0.5 * delta;

  // Don't let the stopping number become negative.

  if(B < 1.)
    B = 1.;

  // Calculate dE/dx.

  double dedx = Density() * K*fZ*B / (fA * beta*beta);

  // Done.

  return dedx;
}

//----------------------------------------------------------------------------------
// Energy loss fluctuation (sigma_E^2 / length in MeV^2/cm).
//
// Arguments:
//
// mom  - Momentum of incident particle in GeV/c.
//
// Based on Bichsel formula referred to but not given in pdg.
//
double util::LArPropertiesOpLib::ElossVar(double mom, double mass) const
{
  // Some constants.

  double K = 0.307075;     // 4 pi N_A r_e^2 m_e c^2 (MeV cm^2/mol).
  double me = 0.510998918; // Electron mass (MeV/c^2).

  // Calculate kinematic quantities.

  double bg = mom / mass;          // beta*gamma.
  double gamma2 = 1. + bg*bg;      // gamma^2.
  double beta2 = bg*bg / gamma2;   // beta^2.

  // Calculate final result.

  double result = gamma2 * (1. - 0.5 * beta2) * me * (fZ / fA) * K * Density();
  return result;
}

//---------------------------------------------------------------------------------
void util::LArPropertiesOpLib::checkDBstatus() const
{
  bool fToughErrorTreatment= art::ServiceHandle<util::DatabaseUtil>()->ToughErrorTreatment();
  bool fShouldConnect =  art::ServiceHandle<util::DatabaseUtil>()->ShouldConnect();
  //Have not read from DB, should read and requested tough treatment
    if(!fAlreadyReadFromDB && fToughErrorTreatment && fShouldConnect )
      throw cet::exception("LArPropertiesOpLib") << " Extracting values from LArPropertiesOpLib before they "
              << " have been read in from database. \n "
              << "Set ToughErrorTreatment or ShouldConnect "
              << " to false in databaseutil.fcl if you want "
              << " to avoid this. \n";
   //Have not read from DB, should read and requested soft treatment
    else if(!fAlreadyReadFromDB && !fToughErrorTreatment && fShouldConnect )
      mf::LogWarning("LArPropertiesOpLib") <<  "!!! Extracting values from LArPropertiesOpLib before they "
              << " have been read in from the database. \n "
              << " You may not be using the correct values of "
              << " electron lifetime, temperature and electric field!"
              << " You should not be initializing"
              << " Database originating values in BeginJob()s or constructors."
              << " You have been warned !!! \n ";

    //In other cases, either already read from DB, or should not connect so it doesn't matter
}


//---------------------------------------------------------------------------------
std::map<double,double> util::LArPropertiesOpLib::FastScintSpectrum()
{
  if(fFastScintSpectrum.size()!=fFastScintEnergies.size()){
    throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
      << "The vectors specifying the fast scintillation spectrum are "
      << " different sizes - " << fFastScintSpectrum.size()
      << " " << fFastScintEnergies.size();
  }

  std::map<double, double> ToReturn;
  for(size_t i=0; i!=fFastScintSpectrum.size(); ++i)
    ToReturn[fFastScintEnergies.at(i)]=fFastScintSpectrum.at(i);

  return ToReturn;
}

//---------------------------------------------------------------------------------
std::map<double, double> util::LArPropertiesOpLib::SlowScintSpectrum()
{
  if(fSlowScintSpectrum.size()!=fSlowScintEnergies.size()){
      throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
  << "The vectors specifying the slow scintillation spectrum are "
  << " different sizes - " << fFastScintSpectrum.size()
  << " " << fFastScintEnergies.size();
    }

  std::map<double, double> ToReturn;
  for(size_t i=0; i!=fSlowScintSpectrum.size(); ++i)
    ToReturn[fSlowScintEnergies.at(i)]=fSlowScintSpectrum.at(i);

  return ToReturn;
}

//---------------------------------------------------------------------------------
std::map<double, double> util::LArPropertiesOpLib::RIndexSpectrum()
{
  if(fRIndexSpectrum.size()!=fRIndexEnergies.size()){
      throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
  << "The vectors specifying the RIndex spectrum are "
  << " different sizes - " << fRIndexSpectrum.size()
  << " " << fRIndexEnergies.size();
  }

  std::map<double, double> ToReturn;
  for(size_t i=0; i!=fRIndexSpectrum.size(); ++i)
    ToReturn[fRIndexEnergies.at(i)]=fRIndexSpectrum.at(i);

  return ToReturn;
}


//---------------------------------------------------------------------------------
std::map<double, double> util::LArPropertiesOpLib::AbsLengthSpectrum()
{
  if(fAbsLengthSpectrum.size()!=fAbsLengthEnergies.size()){
    throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
      << "The vectors specifying the Abs Length spectrum are "
      << " different sizes - " << fAbsLengthSpectrum.size()
      << " " << fAbsLengthEnergies.size();
  }

  std::map<double, double> ToReturn;
  for(size_t i=0; i!=fAbsLengthSpectrum.size(); ++i)
    ToReturn[fAbsLengthEnergies.at(i)]=fAbsLengthSpectrum.at(i);

  return ToReturn;
}

//---------------------------------------------------------------------------------
std::map<double, double> util::LArPropertiesOpLib::RayleighSpectrum()
{
  if(fRayleighSpectrum.size()!=fRayleighEnergies.size()){
    throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
      << "The vectors specifying the rayleigh spectrum are "
      << " different sizes - " << fRayleighSpectrum.size()
      << " " << fRayleighEnergies.size();
  }

  std::map<double, double> ToReturn;
  for(size_t i=0; i!=fRayleighSpectrum.size(); ++i)
    ToReturn[fRayleighEnergies.at(i)]=fRayleighSpectrum.at(i);

  return ToReturn;
}

//---------------------------------------------------------------------------------
std::map<double, double> util::LArPropertiesOpLib::TpbAbs()
{
  if(fTpbAbsorptionEnergies.size()!=fTpbAbsorptionSpectrum.size()){
    throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
      << "The vectors specifying the TpbAbsorption spectrum are "
      << " different sizes - " << fTpbAbsorptionEnergies.size()
      << " " << fTpbAbsorptionSpectrum.size();
  }

  std::map<double, double> ToReturn;
  for(size_t i=0; i!=fTpbAbsorptionSpectrum.size(); ++i)
    ToReturn[fTpbAbsorptionEnergies.at(i)]=fTpbAbsorptionSpectrum.at(i);

  return ToReturn;
}

//---------------------------------------------------------------------------------
std::map<double, double> util::LArPropertiesOpLib::TpbEm()
{
  if(fTpbEmmisionEnergies.size()!=fTpbEmmisionSpectrum.size()){
    throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
      << "The vectors specifying the TpbEmmision spectrum are "
      << " different sizes - " << fTpbEmmisionEnergies.size()
      << " " << fTpbEmmisionSpectrum.size();
  }
//using interpolation for more smooth spectrum of TPB emmision - won't affect anything but the effective size of table passed to G4
Int_t tablesize=100;
std::vector<double> new_x;
double xrange=0.0;
Double_t *en = new Double_t[int(fTpbEmmisionSpectrum.size())+1];
Double_t *spectr = new Double_t[int(fTpbEmmisionSpectrum.size())+1];
	for(int j=0;j<int(fTpbEmmisionSpectrum.size())+1;j++){
		if(j==0){ 
			en[j]=0.;
			en[j]=0.;
			}
		else{
			en[j]=fTpbEmmisionEnergies[j-1];
			spectr[j]=fTpbEmmisionSpectrum[j-1];
			//if(j==int(fTpbEmmisionSpectrum.size())) spectr[j]=+0.5;
			}
		//std::cout<<j<<" "<<int(fTpbEmmisionSpectrum.size())<<" energiestpb "<<en[j]<<std::endl;
	}
TH1D *energyhist=new TH1D();
energyhist->SetBins(int(fTpbEmmisionSpectrum.size()),en);
for(int ii=0;ii<int(fTpbEmmisionSpectrum.size());ii++) energyhist->SetBinContent(ii,spectr[ii]);
xrange=double((en[int(fTpbEmmisionSpectrum.size())]-en[0])/double(fTpbEmmisionSpectrum.size()));
new_x.clear();
  for(int jj=0; jj<int(tablesize); jj++){

 new_x.push_back(jj*(xrange/double(tablesize)));
//std::cout<<"position "<<jj<<" "<<new_x[jj]<<" size of table "<<tablesize<<" range x "<<xrange<<std::endl;
}
  std::map<double, double> ToReturn;
  //for(size_t i=0; i!=fTpbEmmisionSpectrum.size(); ++i)
  //  ToReturn[fTpbEmmisionEnergies.at(i)]=fTpbEmmisionSpectrum.at(i);


  for(int i=0; i<tablesize; i++){
    ToReturn[new_x.at(i)]=energyhist->Interpolate(new_x[i]);
//std::cout<<ToReturn[new_x[i]]<< " is set in material propertiestpb at energy "<<new_x[i]<<" size of x "<<new_x.size()<<" "<<energyhist->Interpolate(new_x[i])<<std::endl;
		}
delete energyhist;

delete[] en;
delete[] spectr;
  return ToReturn;
}

//---------------------------------------------------------------------------------
std::map<std::string, std::map<double,double> > util::LArPropertiesOpLib::SurfaceReflectances()
{
  std::map<std::string, std::map<double, double> > ToReturn;

  if(fReflectiveSurfaceNames.size()!=fReflectiveSurfaceReflectances.size()){
    throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
      << "The vectors specifying the surface reflectivities "
      << "do not have consistent sizes";
  }
  for(size_t i=0; i!=fReflectiveSurfaceNames.size(); ++i){
    if(fReflectiveSurfaceEnergies.size()!=fReflectiveSurfaceReflectances.at(i).size()){
      throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
  << "The vectors specifying the surface reflectivities do not have consistent sizes";
    }
  }
  for(size_t iName=0; iName!=fReflectiveSurfaceNames.size(); ++iName)
    for(size_t iEnergy=0; iEnergy!=fReflectiveSurfaceEnergies.size(); ++iEnergy)
      ToReturn[fReflectiveSurfaceNames.at(iName)][fReflectiveSurfaceEnergies.at(iEnergy)]=fReflectiveSurfaceReflectances[iName][iEnergy];

  return ToReturn;

}

//---------------------------------------------------------------------------------
std::map<std::string, std::map<double,double> > util::LArPropertiesOpLib::SurfaceReflectanceDiffuseFractions()
{
  std::map<std::string, std::map<double, double> > ToReturn;

  if(fReflectiveSurfaceNames.size()!=fReflectiveSurfaceDiffuseFractions.size()){
    throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
      << "The vectors specifying the surface reflectivities do not have consistent sizes";
  }
  for(size_t i=0; i!=fReflectiveSurfaceNames.size(); ++i){
    if(fReflectiveSurfaceEnergies.size()!=fReflectiveSurfaceDiffuseFractions.at(i).size()){
      throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
  << "The vectors specifying the surface reflectivities do not have consistent sizes";

    }
  }
  for(size_t iName=0; iName!=fReflectiveSurfaceNames.size(); ++iName)
    for(size_t iEnergy=0; iEnergy!=fReflectiveSurfaceEnergies.size(); ++iEnergy)
      ToReturn[fReflectiveSurfaceNames.at(iName)][fReflectiveSurfaceEnergies.at(iEnergy)]=fReflectiveSurfaceDiffuseFractions[iName][iEnergy];

  return ToReturn;
}

//---------------------------------------------------------------------------------
std::map<std::string, std::map<double,double> > util::LArPropertiesOpLib::SurfaceTpbReflectances()
{
  std::map<std::string, std::map<double, double> > ToReturn;
std::cout<<"if yousee this that means you're setting tpb reflectances in a wrong way !!"<<std::endl;
  if(fReflectiveSurfaceTpbNames.size()!=fReflectiveSurfaceTpbReflectances.size()){
    throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
      << "The vectors specifying the surface Tpb reflectivities "
      << "do not have consistent sizes";
  }
  for(size_t i=0; i!=fReflectiveSurfaceTpbNames.size(); ++i){
    if(fReflectiveSurfaceTpbEnergies.size()!=fReflectiveSurfaceTpbReflectances.at(i).size()){
      throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
  << "The vectors specifying the surface Tpb reflectivities do not have consistent sizes";
    }
  }
  for(size_t iName=0; iName!=fReflectiveSurfaceTpbNames.size(); ++iName)
    for(size_t iEnergy=0; iEnergy!=fReflectiveSurfaceTpbEnergies.size(); ++iEnergy)
      ToReturn[fReflectiveSurfaceTpbNames.at(iName)][fReflectiveSurfaceTpbEnergies.at(iEnergy)]=fReflectiveSurfaceTpbReflectances[iName][iEnergy];

  return ToReturn;

}

//---------------------------------------------------------------------------------
std::map<std::string, std::map<double,double> > util::LArPropertiesOpLib::SurfaceReflectanceTpbDiffuseFractions()
{
  std::map<std::string, std::map<double, double> > ToReturn;

  if(fReflectiveSurfaceTpbNames.size()!=fReflectiveSurfaceTpbDiffuseFractions.size()){
    throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
      << "The vectors specifying the surface Tpb reflectivities do not have consistent sizes";
  }
  for(size_t i=0; i!=fReflectiveSurfaceTpbNames.size(); ++i){
    if(fReflectiveSurfaceTpbEnergies.size()!=fReflectiveSurfaceTpbDiffuseFractions.at(i).size()){
      throw cet::exception("Incorrect vector sizes in LArPropertiesOpLib")
  << "The vectors specifying the surface Tpb reflectivities do not have consistent sizes";

    }
  }
  for(size_t iName=0; iName!=fReflectiveSurfaceTpbNames.size(); ++iName)
    for(size_t iEnergy=0; iEnergy!=fReflectiveSurfaceTpbEnergies.size(); ++iEnergy)
      ToReturn[fReflectiveSurfaceTpbNames.at(iName)][fReflectiveSurfaceTpbEnergies.at(iEnergy)]=fReflectiveSurfaceTpbDiffuseFractions[iName][iEnergy];

  return ToReturn;
}



namespace util{
 
  DEFINE_ART_SERVICE(LArPropertiesOpLib)

} // namespace util
