// Class adapted for LArSoft by Ben Jones, MIT 10/10/12
//
// This class is a physics process based on the standard Geant4
// scintillation process.
//
// It has been stripped down and adapted to form the backbone of 
// the LArG4OpLib fast optical simulation.  Photons, instead of being
// produced and added to the geant4 particle stack, are logged
// and used to predict the visibility of this step to each PMT in
// the detector.
//
// The photonvisibilityservice looks up the visibility of the relevant
// xyz point, and if a photon is detected at a given PMT, one OnePhoton 
// object is logged in the OpDetPhotonTable
//
// At the end of the event, the OpDetPhotonTable is read out
// by LArG4OpLib, and detected photons are stored in the event.
//
// This process can be used alongside the standard Cerenkov process,
// which does step geant4 opticalphotons.  Both the fast scintillation
// table and the geant4 sensitive detectors are read out by LArG4OpLib to
// produce a combined SimPhoton collection.
//
// Added disclaimer : This code is gross.  Thats basically because
//  it adheres to the original, gross Geant4 implementation.
//
//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
// $Id: OpFastScintillation.cc,v 1.38 2010-12-15 07:39:26 gunter Exp $
// GEANT4 tag $Name: not supported by cvs2svn $
//
////////////////////////////////////////////////////////////////////////
// Scintillation Light Class Implementation
////////////////////////////////////////////////////////////////////////
//
// File:        OpFastScintillation.cc
// Description: RestDiscrete Process - Generation of Scintillation Photons
// Version:     1.0
// Created:     1998-11-07
// Author:      Peter Gumplinger
// Updated:     2010-10-20 Allow the scintillation yield to be a function
//              of energy deposited by particle type
//              Thanks to Zach Hartwig (Department of Nuclear
//              Science and Engineeering - MIT)
//              2010-09-22 by Peter Gumplinger
//              > scintillation rise time included, thanks to
//              > Martin Goettlich/DESY
//              2005-08-17 by Peter Gumplinger
//              > change variable name MeanNumPhotons -> MeanNumberOfPhotons
//              2005-07-28 by Peter Gumplinger
//              > add G4ProcessType to constructor
//              2004-08-05 by Peter Gumplinger
//              > changed StronglyForced back to Forced in GetMeanLifeTime
//              2002-11-21 by Peter Gumplinger
//              > change to use G4Poisson for small MeanNumberOfPhotons
//              2002-11-07 by Peter Gumplinger
//              > now allow for fast and slow scintillation component
//              2002-11-05 by Peter Gumplinger
//              > now use scintillation constants from G4Material
//              2002-05-09 by Peter Gumplinger
//              > use only the PostStepPoint location for the origin of
//                scintillation photons when energy is lost to the medium
//                by a neutral particle
//              2000-09-18 by Peter Gumplinger
//              > change: aSecondaryPosition=x0+rand*aStep.GetDeltaPosition();
//                        aSecondaryTrack->SetTouchable(0);
//              2001-09-17, migration of Materials to pure STL (mma)
//              2003-06-03, V.Ivanchenko fix compilation warnings
//
// mail:        gum@triumf.ca
//
////////////////////////////////////////////////////////////////////////

//#include "Geant4/g4ios.hh"
#include "TLorentzVector.h"

#include "Geant4/globals.hh"
#include "Geant4/G4ParticleTypes.hh"
#include "Geant4/G4EmProcessSubType.hh"

#include "lar1ndcode/LArG4_OpLib/IonizationAndScintillation.h"
#include "lar1ndcode/LArG4_OpLib/OpFastScintillation.hh"
#include "lar1ndcode/PhotonPropagation_OpLib/PhotonVisibilityServiceOpLib.h"
#include "lar1ndcode/LArG4_OpLib/OpDetPhotonTable.h"
#include "Simulation/SimPhotons.h"
#include "Simulation/LArG4Parameters.h"
#include "Geometry/Geometry.h"
#include "Geometry/CryostatGeo.h"
#include "Geometry/OpDetGeo.h"

#include "lar1ndcode/Utilities/LArPropertiesOpLib.h"
#include "Utilities/DetectorProperties.h" 

#include "art/Framework/Services/Optional/RandomNumberGenerator.h"

#include "TH1.h"
#include "TF1.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TMath.h"

namespace larg4{

/////////////////////////
// Class Implementation
/////////////////////////
  
  //////////////
  // Operators
  //////////////
  
  // OpFastScintillation::operator=(const OpFastScintillation &right)
  // {
  // }
  
  /////////////////
  // Constructors
  /////////////////
  
  OpFastScintillation::OpFastScintillation(const G4String& processName,
						 G4ProcessType type)
  : G4VRestDiscreteProcess(processName, type)
  {
        SetProcessSubType(25);

        fTrackSecondariesFirst = false;
        fFiniteRiseTime = false;


	YieldFactor=1.0;
        ExcitationRatio = 1.0;
	
	art::ServiceHandle<util::LArPropertiesOpLib> larp;
	
        scintillationByParticleType = larp->ScintByParticleType();

        theFastIntegralTable = NULL;
        theSlowIntegralTable = NULL;

        if (verboseLevel>0) {
           G4cout << GetProcessName() << " is created " << G4endl;
        }

        BuildThePhysicsTable();
        art::ServiceHandle<util::DetectorProperties> det;                 
        //fGlobalTimeOffset = det->ConvertTicksToTDC(0) * det->SamplingRate();

        emSaturation = NULL;
}

  OpFastScintillation::OpFastScintillation(const OpFastScintillation& rhs)
    :  G4VRestDiscreteProcess(rhs.GetProcessName(), rhs.GetProcessType())
  {
    theSlowIntegralTable        = rhs.GetSlowIntegralTable();
    theFastIntegralTable        = rhs.GetFastIntegralTable();
    
    fTrackSecondariesFirst      = rhs.GetTrackSecondariesFirst();
    fFiniteRiseTime             = rhs.GetFiniteRiseTime();
    YieldFactor                 = rhs.GetScintillationYieldFactor();
    ExcitationRatio             = rhs.GetScintillationExcitationRatio();
    scintillationByParticleType = rhs.GetScintillationByParticleType();
    emSaturation                = rhs.GetSaturation();

    art::ServiceHandle<util::DetectorProperties> det;                 
    //fGlobalTimeOffset = det->ConvertTicksToTDC(0) * det->SamplingRate();

    BuildThePhysicsTable();
  }

        ////////////////
        // Destructors
        ////////////////

OpFastScintillation::~OpFastScintillation()
{
	if (theFastIntegralTable != NULL) {
           theFastIntegralTable->clearAndDestroy();
           delete theFastIntegralTable;
        }
        if (theSlowIntegralTable != NULL) {
           theSlowIntegralTable->clearAndDestroy();
           delete theSlowIntegralTable;
        }
}

        ////////////
        // Methods
        ////////////

// AtRestDoIt
// ----------
//
G4VParticleChange*
OpFastScintillation::AtRestDoIt(const G4Track& aTrack, const G4Step& aStep)

// This routine simply calls the equivalent PostStepDoIt since all the
// necessary information resides in aStep.GetTotalEnergyDeposit()

{
        return OpFastScintillation::PostStepDoIt(aTrack, aStep);
}

// PostStepDoIt
// -------------
//
G4VParticleChange*
OpFastScintillation::PostStepDoIt(const G4Track& aTrack, const G4Step& aStep)
// This routine is called for each tracking step of a charged particle
// in a scintillator. A Poisson/Gauss-distributed number of photons is 
// generated according to the scintillation yield formula, distributed 
// evenly along the track segment and uniformly into 4pi.

{
        aParticleChange.Initialize(aTrack);

	// Check that we are in a material with a properties table, if not
	// just return
        const G4Material* aMaterial = aTrack.GetMaterial();
        G4MaterialPropertiesTable* aMaterialPropertiesTable =
                               aMaterial->GetMaterialPropertiesTable();
        if (!aMaterialPropertiesTable)
             return G4VRestDiscreteProcess::PostStepDoIt(aTrack, aStep);

        G4StepPoint* pPreStepPoint  = aStep.GetPreStepPoint();
      
        G4ThreeVector x0 = pPreStepPoint->GetPosition();
        G4ThreeVector p0 = aStep.GetDeltaPosition().unit();
      
	///////////////////////////////////////////////////////////////////////////////////
	//   This is the old G4 way - but we do things differently - Ben J, Oct Nov 2012.
	///////////////////////////////////////////////////////////////////////////////////
	//
	//     if (MeanNumberOfPhotons > 10.)
	//      {
	//        G4double sigma = ResolutionScale * std::sqrt(MeanNumberOfPhotons);
	//        NumPhotons = G4int(G4RandGauss::shoot(MeanNumberOfPhotons,sigma)+0.5);
	//      }
	//     else
	//      {
	//        NumPhotons = G4int(G4Poisson(MeanNumberOfPhotons));
	//      }
	//
	//
	//
	//        if (NumPhotons <= 0)
	//        {
	//  // return unchanged particle and no secondaries 
	//
	//           aParticleChange.SetNumberOfSecondaries(0);
	//
	//           return G4VRestDiscreteProcess::PostStepDoIt(aTrack, aStep);
	//        }
	//
	//
	//       aParticleChange.SetNumberOfSecondaries(NumPhotons);
	//
	//
	//        if (fTrackSecondariesFirst) {
	//           if (aTrack.GetTrackStatus() == fAlive )
	//                  aParticleChange.ProposeTrackStatus(fSuspend);
	//        }
	//
	//
	//
	//
        ////////////////////////////////////////////////////////////////////////////////////
	//

	
	////////////////////////////////////////////////////////////////////////////////////
	//  The fast sim way - Ben J, Nov 2012
	////////////////////////////////////////////////////////////////////////////////////
	//
	//

	// We don't want to produce any trackable G4 secondaries
	aParticleChange.SetNumberOfSecondaries(0);

	
        // Retrieve the Scintillation Integral for this material  
        // new G4PhysicsOrderedFreeVector allocated to hold CII's
	

	// Some explanation for later improvements to scint yield code:
	//
	// What does G4 do here?
	//  It produces light in 2 steps, fast (scnt=1) then slow (scnt=2)
	//
	// The ratio of slow photons to fast photons is related	by the yieldratio
	//  parameter.  G4's poisson fluctuating scheme is a bit different to ours
	//  - we should check that they are equivalent.
	//
	// G4 poisson fluctuates the number of initial photons then divides them
	//  with a constant factor between fast + slow, whereas we poisson 
	//  fluctuate separateyly the fast and slow detection numbers.
	//
	
	// get the number of photons produced from the IonizationAndScintillation
	// singleton
	larg4::IonizationAndScintillation::Instance()->Reset(&aStep);
	double MeanNumberOfPhotons = larg4::IonizationAndScintillation::Instance()->NumberScintillationPhotons();
        RecordPhotonsProduced(aStep, MeanNumberOfPhotons);

	
	if (verboseLevel>0) {
	  G4cout << "\n Exiting from OpFastScintillation::DoIt -- NumberOfSecondaries = " 
		 << aParticleChange.GetNumberOfSecondaries() << G4endl;
	}
	
	
	return G4VRestDiscreteProcess::PostStepDoIt(aTrack, aStep);
}


//-------------------------------------------------------------

bool OpFastScintillation::RecordPhotonsProduced(const G4Step& aStep, double MeanNumberOfPhotons)
{

  // Get the pointer to the fast scintillation table
  OpDetPhotonTable * fst = OpDetPhotonTable::Instance();
  OpDetPhotonTable* litefst = OpDetPhotonTable::Instance();

  // Get the pointer to the visibility service
  art::ServiceHandle<phot::PhotonVisibilityServiceOpLib> pvs;
  art::ServiceHandle<sim::LArG4Parameters> lgp;

  const G4Track * aTrack = aStep.GetTrack();

  G4StepPoint* pPreStepPoint  = aStep.GetPreStepPoint();
  G4StepPoint* pPostStepPoint = aStep.GetPostStepPoint();
  
  const G4DynamicParticle* aParticle = aTrack->GetDynamicParticle();
  const G4Material* aMaterial = aTrack->GetMaterial();

  G4int materialIndex = aMaterial->GetIndex();
	

  G4ThreeVector x0 = pPreStepPoint->GetPosition();
  G4ThreeVector p0 = aStep.GetDeltaPosition().unit();
  //G4double      t0 = pPreStepPoint->GetGlobalTime() - fGlobalTimeOffset;
  G4double      t0 = pPreStepPoint->GetGlobalTime();
  
  
  G4MaterialPropertiesTable* aMaterialPropertiesTable =
    aMaterial->GetMaterialPropertiesTable();

  double xyz[3];
  xyz[0]=x0[0]/cm;
  xyz[1]=x0[1]/cm;
  xyz[2]=x0[2]/cm;

  // Get the visibility vector for this point
  const std::vector<float>* Visibilities = nullptr;
  if(!pvs->UseParameterization())Visibilities = pvs->GetAllVisibilities(xyz);

  
  const std::vector<float>* ReflVisibilities = nullptr;
  if(!pvs->UseParameterization() && pvs->StoreReflected())ReflVisibilities = pvs->GetAllVisibilities(xyz,true);


  G4MaterialPropertyVector* Fast_Intensity = 
    aMaterialPropertiesTable->GetProperty("FASTCOMPONENT"); 
  G4MaterialPropertyVector* Slow_Intensity =
    aMaterialPropertiesTable->GetProperty("SLOWCOMPONENT");
  
  if (!Fast_Intensity && !Slow_Intensity )
    return 1;
  
  
  G4int nscnt = 1;
  if (Fast_Intensity && Slow_Intensity) nscnt = 2;

  
  G4int Num = 0;
  double YieldRatio=0;

  
  if (scintillationByParticleType) {
    // The scintillation response is a function of the energy
    // deposited by particle types.
    
    // Get the definition of the current particle
    G4ParticleDefinition *pDef = aParticle->GetDefinition();
    
    // Obtain the G4MaterialPropertyVectory containing the
    // scintillation light yield as a function of the deposited
    // energy for the current particle type
    
    // Protons
    if(pDef==G4Proton::ProtonDefinition()) 
      {
	YieldRatio = aMaterialPropertiesTable->
	  GetConstProperty("PROTONYIELDRATIO");

      }
    
    // Muons
    else if(pDef==G4MuonPlus::MuonPlusDefinition()||pDef==G4MuonMinus::MuonMinusDefinition())
      {
	YieldRatio = aMaterialPropertiesTable->
	  GetConstProperty("MUONYIELDRATIO");
      }
    
    // Pions
    else if(pDef==G4PionPlus::PionPlusDefinition()||pDef==G4PionMinus::PionMinusDefinition())
      {
	YieldRatio = aMaterialPropertiesTable->
	  GetConstProperty("PIONYIELDRATIO");
      }
    
    // Kaons
    else if(pDef==G4KaonPlus::KaonPlusDefinition()||pDef==G4KaonMinus::KaonMinusDefinition())
      {
	YieldRatio = aMaterialPropertiesTable->
	  GetConstProperty("KAONYIELDRATIO");
      }
    
    // Alphas
    else if(pDef==G4Alpha::AlphaDefinition())
      {
	YieldRatio = aMaterialPropertiesTable->
	  GetConstProperty("ALPHAYIELDRATIO");
      }
    
    // Electrons (must also account for shell-binding energy
    // attributed to gamma from standard PhotoElectricEffect)
    else if(pDef==G4Electron::ElectronDefinition() ||
	    pDef==G4Gamma::GammaDefinition())
      {
	YieldRatio = aMaterialPropertiesTable->
	  GetConstProperty("ELECTRONYIELDRATIO");
      }
    
    // Default for particles not enumerated/listed above
    else
      {
	YieldRatio = aMaterialPropertiesTable->
	  GetConstProperty("ELECTRONYIELDRATIO");
      }
    
    // If the user has not specified yields for (p,d,t,a,carbon)
    // then these unspecified particles will default to the 
    // electron's scintillation yield
    if(YieldRatio==0){
      {
	
	YieldRatio = aMaterialPropertiesTable->
	  GetConstProperty("ELECTRONYIELDRATIO");
	
      }
    }
  }

  
  for (G4int scnt = 1; scnt <= nscnt; scnt++) {
    
    G4double ScintillationTime = 0.*ns;
    G4double ScintillationRiseTime = 0.*ns;
    G4PhysicsOrderedFreeVector* ScintillationIntegral = NULL;
    
    if (scnt == 1) {
      if (nscnt == 1) {
	if(Fast_Intensity){
	  ScintillationTime   = aMaterialPropertiesTable->
	    GetConstProperty("FASTTIMECONSTANT");
	  if (fFiniteRiseTime) {
	    ScintillationRiseTime = aMaterialPropertiesTable->
	      GetConstProperty("FASTSCINTILLATIONRISETIME");
	  }
	  ScintillationIntegral =
	    (G4PhysicsOrderedFreeVector*)((*theFastIntegralTable)(materialIndex));
	}
	if(Slow_Intensity){
	  ScintillationTime   = aMaterialPropertiesTable->
	    GetConstProperty("SLOWTIMECONSTANT");
	  if (fFiniteRiseTime) {
	    ScintillationRiseTime = aMaterialPropertiesTable->
	      GetConstProperty("SLOWSCINTILLATIONRISETIME");
	  }
	  ScintillationIntegral =
	    (G4PhysicsOrderedFreeVector*)((*theSlowIntegralTable)(materialIndex));
	}
      }
      else {
	if(YieldRatio==0) 
	  YieldRatio = aMaterialPropertiesTable->
	    GetConstProperty("YIELDRATIO");
	
	
	if ( ExcitationRatio == 1.0 ) {
	  Num = G4int (std::min(YieldRatio,1.0)*MeanNumberOfPhotons);
	}
	else {
	  Num = G4int (std::min(ExcitationRatio,1.0)*MeanNumberOfPhotons);
	}
	ScintillationTime   = aMaterialPropertiesTable->
		  GetConstProperty("FASTTIMECONSTANT");
	if (fFiniteRiseTime) {
	  ScintillationRiseTime = aMaterialPropertiesTable->
	    GetConstProperty("FASTSCINTILLATIONRISETIME");
	}
	ScintillationIntegral =
	  (G4PhysicsOrderedFreeVector*)((*theFastIntegralTable)(materialIndex));
      }
    }
    
    else {
      Num = MeanNumberOfPhotons - Num;
      ScintillationTime   =   aMaterialPropertiesTable->
	GetConstProperty("SLOWTIMECONSTANT");
      if (fFiniteRiseTime) {
                    ScintillationRiseTime = aMaterialPropertiesTable->
		      GetConstProperty("SLOWSCINTILLATIONRISETIME");
      }
      ScintillationIntegral =
	(G4PhysicsOrderedFreeVector*)((*theSlowIntegralTable)(materialIndex));
    }
    
    if (!ScintillationIntegral) continue;
    
    // Max Scintillation Integral
    
    //            G4double CIImax = ScintillationIntegral->GetMaxValue();
    
    
    //std::cout << "++++++++++++" << Num << "++++++++++" << std::endl;
    



    std::map<int, int> DetectedNum;
    std::map<int, int> ReflDetectedNum;
    if(!Visibilities && !(pvs->UseParameterization()))
      {
	// if null pointer, this means no data for this voxel - in 
	// this case do nothing.
	//mf::LogInfo("OpFastScintillation")<<"Warning : null vis vector"<<std::endl;
      }
    else
    {
	 if(pvs->UseParameterization())
	  {
        art::ServiceHandle<geo::Geometry> geo;

        double OpDetCenter[3];
        unsigned int o=0; unsigned int c=0;
        TVector3 fPlaneNorm(1,0,0);
        int CageId = -1;

        for(int i = 0; i < 6; i++)
        {
          geo->OpChannelToCryoOpDet(200*i,o,c);
          geo->Cryostat(c).OpDet(o).GetCenter(OpDetCenter);
          if(std::abs(xyz[0] - OpDetCenter[0])< 231) 
          {
            CageId = i;
            break;
          }
        }

        if(CageId == -1) break;
              
        for(int OpChan= 200*CageId; OpChan < 200*CageId + 200; OpChan++)
        {
          geo->OpChannelToCryoOpDet(OpChan,o,c);
          geo->Cryostat(c).OpDet(o).GetCenter(OpDetCenter);

          if(Num > 0) 
          {
            double normalizedvisibility = 0.;
            double distance;
            double dx = xyz[0]-OpDetCenter[0];
            double dy = xyz[1]-OpDetCenter[1];

            //Calculate the visibility along the paddle
            for(int i = 0; i<10; i++)
            {
              double dz = xyz[2]-OpDetCenter[2] - 22.5*(i-4.5);
              TVector3 PhotonToDetector(dx,dy,dz);
              //double CosTheta = fPlaneNorm.Dot(PhotonToDetector.Unit());

              double distancesquare = dx*dx + dy*dy + dz*dz;
              if(distancesquare > 1000000) continue;
              distance = std::sqrt(distancesquare);

              //Stitch the attenuation function of the Acrylic bar to visibility function
              //normalizedvisibility += std::exp(-0.2*i)*(7.98*10000000000/distancesquare - 1.38*1000000000/distance 
              //+ 4940000 + 37480*distance - 306.3*distancesquare + 0.604 * distancesquare * distance ) 
              //* std::exp(-distance*0.0374)*std::abs(xyz[0] - OpDetCenter[0])/2000000000;
              //Without attenuation in the Acrylic bar
              //normalizedvisibility += (7.98*10000000000/distancesquare - 1.38*1000000000/distance + 4940000 + 
              //37480*  distance - 306.3*distancesquare + 0.604 * distancesquare * distance ) 
              //* std::exp(-distance*0.0374)*std::abs(xyz[0] - OpDetCenter[0])/       2000000000;
                
              //Another fitting function;
              normalizedvisibility += /*(1-0.03/CosTheta)**/(477200 - 11980*distance + 120*distancesquare - 
                      0.5726*distancesquare*distance + 0.001086*distancesquare*distancesquare ) 
                     * std::exp(-0.0528*distance)/40000;
            }
            G4int DetThisPMT = G4int(G4Poisson(normalizedvisibility*Num));
            if(DetThisPMT>0)
            {
              DetectedNum[OpChan] = DetThisPMT;
            }
          }
       }

    std::map<int, std::map<int, int>> StepPhotonTable;
    // And then add these to the total collection for the event	    
	for(std::map<int,int>::const_iterator itdetphot = DetectedNum.begin();
	    itdetphot!=DetectedNum.end(); ++itdetphot)
	  {
          std::map<int, int>  StepPhotons;
          for (G4int i = 0; i < itdetphot->second; ++i) 
	      {
            G4double deltaTime = aStep.GetStepLength() /
                ((pPreStepPoint->GetVelocity()+ pPostStepPoint->GetVelocity())/2.);
		
		
            if (ScintillationRiseTime==0.0) {
                deltaTime = deltaTime - 
                    ScintillationTime * std::log( G4UniformRand() );
            } else {
                deltaTime = deltaTime +
                    sample_time(ScintillationRiseTime, ScintillationTime);
            }		
		
            G4double aSecondaryTime = t0 + deltaTime;
            float Time = aSecondaryTime;
            int ticks = static_cast<int>(Time);
            StepPhotons[ticks]++;
	      }
         StepPhotonTable[itdetphot->first] = StepPhotons;
	  }
    litefst->AddPhoton(&StepPhotonTable);
	  }
	else
    {
	  for(size_t OpChan=0; OpChan!=Visibilities->size(); OpChan++)
      {
		G4int DetThisPMT = G4int(G4Poisson(Visibilities->at(OpChan) * Num));
		if(DetThisPMT>0) 
		{
		    DetectedNum[OpChan]=DetThisPMT;
		  //     std::cout <<("OpFastScintillation") << "FastScint: " <<
		  //   OpChan <<" " << Num << " " << DetThisPMT << std::endl;  
		}
		G4int ReflDetThisPMT = G4int(G4Poisson(ReflVisibilities->at(OpChan) * Num));
		if(ReflDetThisPMT>0) 
		{
		    ReflDetectedNum[OpChan]=ReflDetThisPMT;
		    //   std::cout <<("OpFastScintillation") << "FastScintRefl: " <<
		   //  OpChan <<" " << Num << " " << DetThisPMT << std::endl;  
		}
		
      }
	  // Now we run through each PMT figuring out num of detected photons
	
      if(lgp->UseLitePhotons())
      {
        std::map<int, std::map<int, int>> StepPhotonTable;
        // And then add these to the total collection for the event     
        for(std::map<int,int>::const_iterator itdetphot = DetectedNum.begin();
                itdetphot!=DetectedNum.end(); ++itdetphot)
        {
          std::map<int, int>  StepPhotons;
          for (G4int i = 0; i < itdetphot->second; ++i)
          {
            G4double deltaTime = aStep.GetStepLength() /
                ((pPreStepPoint->GetVelocity()+ pPostStepPoint->GetVelocity())/2.);


            if (ScintillationRiseTime==0.0) {
                deltaTime = deltaTime -
                    ScintillationTime * std::log( G4UniformRand() );
            } else {
                deltaTime = deltaTime +
                    sample_time(ScintillationRiseTime, ScintillationTime);
            }

            G4double aSecondaryTime = t0 + deltaTime;
            float Time = aSecondaryTime;
            int ticks = static_cast<int>(Time);
            StepPhotons[ticks]++;
          }
         StepPhotonTable[itdetphot->first] = StepPhotons;
        }
        litefst->AddPhoton(&StepPhotonTable);
      }
      else
      {
	  // And then add these to the total collection for the event	    
      for(std::map<int,int>::const_iterator itdetphot = DetectedNum.begin();
	    itdetphot!=DetectedNum.end(); ++itdetphot)
      {
	    for (G4int i = 0; i < itdetphot->second; ++i) 
        {
            G4double deltaTime = aStep.GetStepLength() /
                ((pPreStepPoint->GetVelocity()+
                  pPostStepPoint->GetVelocity())/2.);
		
            if (ScintillationRiseTime==0.0) 
            {
                deltaTime = deltaTime - 
                    ScintillationTime * std::log( G4UniformRand() );
            } 
            else 
            {
              deltaTime = deltaTime +
                  sample_time(ScintillationRiseTime, ScintillationTime);
            }		
		
            G4double aSecondaryTime = t0 + deltaTime;
		
            // The sim photon in this case stores its production point and time
            TVector3 PhotonPosition(x0[0],x0[1],x0[2]);
	    
	  float Energy = 9.7*eV;
            float Time = aSecondaryTime;
           	
            // Make a photon object for the collection
	    sim::OnePhoton PhotToAdd;
            PhotToAdd.InitialPosition  = PhotonPosition;
            PhotToAdd.Energy           = Energy;
            PhotToAdd.Time             = Time;
            PhotToAdd.SetInSD          = false;
			
            fst->AddPhoton(itdetphot->first, std::move(PhotToAdd));
        }
      }
      if(pvs->StoreReflected())
      {
        // And then add these to the total collection for the event	    
      for(std::map<int,int>::const_iterator itdetphot = ReflDetectedNum.begin();
	    itdetphot!=ReflDetectedNum.end(); ++itdetphot)
      {
	    for (G4int i = 0; i < itdetphot->second; ++i) 
        {
            G4double deltaTime = aStep.GetStepLength() /
                ((pPreStepPoint->GetVelocity()+
                  pPostStepPoint->GetVelocity())/2.);
		
            if (ScintillationRiseTime==0.0) 
            {
                deltaTime = deltaTime - 
                    ScintillationTime * std::log( G4UniformRand() );
            } 
            else 
            {
              deltaTime = deltaTime +
                  sample_time(ScintillationRiseTime, ScintillationTime);
            }		
		
            G4double aSecondaryTime = t0 + deltaTime;
		
            // The sim photon in this case stores its production point and time
            TVector3 PhotonPosition(x0[0],x0[1],x0[2]);
		
            // We don't know anything about the momentum dir, so set it to be Z		
	    std::map<double,double> tpbemission=art::ServiceHandle<util::LArPropertiesOpLib>()->TpbEm();
	    const int nbins=tpbemission.size();
	    
	    
	    double * parent=new double[nbins];
	   
            int ii=0;
	    for( std::map<double, double>::iterator iter = tpbemission.begin(); iter != tpbemission.end(); ++iter)
	    { parent[ii++]=(*iter).second;
	    }
            
	    CLHEP::RandGeneral rgen0(parent,nbins);
             
            double x0 = rgen0.fire()*((*(--tpbemission.end())).first-(*tpbemission.begin()).first)+(*tpbemission.begin()).first;
	    
	    
            // We don't know anything about the momentum dir, so set it to be Z		
            float Energy = x0*eV;
            float Time = aSecondaryTime;
		
            // Make a photon object for the collection
	    sim::OnePhoton PhotToAdd;
            PhotToAdd.InitialPosition  = PhotonPosition;
            PhotToAdd.Energy           = Energy;
            PhotToAdd.Time             = Time;
            PhotToAdd.SetInSD          = false;
			
            fst->AddPhoton(itdetphot->first, std::move(PhotToAdd));
	    delete [] parent;
        }
      }
      
      }// end storing reflected light
      
      }
    }
    }
  }
  
  return 0;
  }


// BuildThePhysicsTable for the scintillation process
// --------------------------------------------------
//

void OpFastScintillation::BuildThePhysicsTable()
{
        if (theFastIntegralTable && theSlowIntegralTable) return;

        const G4MaterialTable* theMaterialTable = 
                               G4Material::GetMaterialTable();
        G4int numOfMaterials = G4Material::GetNumberOfMaterials();

        // create new physics table
	
        if(!theFastIntegralTable)theFastIntegralTable = new G4PhysicsTable(numOfMaterials);
        if(!theSlowIntegralTable)theSlowIntegralTable = new G4PhysicsTable(numOfMaterials);

        // loop for materials

        for (G4int i=0 ; i < numOfMaterials; i++)
        {
                G4PhysicsOrderedFreeVector* aPhysicsOrderedFreeVector =
					new G4PhysicsOrderedFreeVector();
                G4PhysicsOrderedFreeVector* bPhysicsOrderedFreeVector =
                                        new G4PhysicsOrderedFreeVector();

                // Retrieve vector of scintillation wavelength intensity for
                // the material from the material's optical properties table.

                G4Material* aMaterial = (*theMaterialTable)[i];

                G4MaterialPropertiesTable* aMaterialPropertiesTable =
                                aMaterial->GetMaterialPropertiesTable();

                if (aMaterialPropertiesTable) {

                   G4MaterialPropertyVector* theFastLightVector = 
                   aMaterialPropertiesTable->GetProperty("FASTCOMPONENT");

                   if (theFastLightVector) {

                      // Retrieve the first intensity point in vector
                      // of (photon energy, intensity) pairs 

                      G4double currentIN = (*theFastLightVector)[0];

                      if (currentIN >= 0.0) {

                         // Create first (photon energy, Scintillation 
                         // Integral pair  

                         G4double currentPM = theFastLightVector->Energy(0);

                         G4double currentCII = 0.0;

                         aPhysicsOrderedFreeVector->
                                 InsertValues(currentPM , currentCII);

                         // Set previous values to current ones prior to loop

                         G4double prevPM  = currentPM;
                         G4double prevCII = currentCII;
                         G4double prevIN  = currentIN;

                         // loop over all (photon energy, intensity)
                         // pairs stored for this material  

                         for (size_t i = 1;
                              i < theFastLightVector->GetVectorLength();
                              i++)
                         {
                                currentPM = theFastLightVector->Energy(i);
                                currentIN = (*theFastLightVector)[i];

                                currentCII = 0.5 * (prevIN + currentIN);

                                currentCII = prevCII +
                                             (currentPM - prevPM) * currentCII;

                                aPhysicsOrderedFreeVector->
                                    InsertValues(currentPM, currentCII);

                                prevPM  = currentPM;
                                prevCII = currentCII;
                                prevIN  = currentIN;
                         }

                      }
                   }

                   G4MaterialPropertyVector* theSlowLightVector =
                   aMaterialPropertiesTable->GetProperty("SLOWCOMPONENT");

                   if (theSlowLightVector) {

                      // Retrieve the first intensity point in vector
                      // of (photon energy, intensity) pairs

                      G4double currentIN = (*theSlowLightVector)[0];

                      if (currentIN >= 0.0) {

                         // Create first (photon energy, Scintillation
                         // Integral pair

                         G4double currentPM = theSlowLightVector->Energy(0);

                         G4double currentCII = 0.0;

                         bPhysicsOrderedFreeVector->
                                 InsertValues(currentPM , currentCII);

                         // Set previous values to current ones prior to loop

                         G4double prevPM  = currentPM;
                         G4double prevCII = currentCII;
                         G4double prevIN  = currentIN;

                         // loop over all (photon energy, intensity)
                         // pairs stored for this material

                         for (size_t i = 1;
                              i < theSlowLightVector->GetVectorLength();
                              i++)
                         {
                                currentPM = theSlowLightVector->Energy(i);
                                currentIN = (*theSlowLightVector)[i];

                                currentCII = 0.5 * (prevIN + currentIN);

                                currentCII = prevCII +
                                             (currentPM - prevPM) * currentCII;

                                bPhysicsOrderedFreeVector->
                                    InsertValues(currentPM, currentCII);

                                prevPM  = currentPM;
                                prevCII = currentCII;
                                prevIN  = currentIN;
                         }

                      }
                   }
                }

        // The scintillation integral(s) for a given material
        // will be inserted in the table(s) according to the
        // position of the material in the material table.

        theFastIntegralTable->insertAt(i,aPhysicsOrderedFreeVector);
        theSlowIntegralTable->insertAt(i,bPhysicsOrderedFreeVector);

        }
}

// Called by the user to set the scintillation yield as a function
// of energy deposited by particle type

void OpFastScintillation::SetScintillationByParticleType(const G4bool scintType)
{
        if (emSaturation) {
           G4Exception("OpFastScintillation::SetScintillationByParticleType", "Scint02",
                       JustWarning, "Redefinition: Birks Saturation is replaced by ScintillationByParticleType!");
           RemoveSaturation();
        }
        scintillationByParticleType = scintType;
}

// GetMeanFreePath
// ---------------
//

G4double OpFastScintillation::GetMeanFreePath(const G4Track&,
                                          G4double ,
                                          G4ForceCondition* condition)
{
        *condition = StronglyForced;

        return DBL_MAX;

}

// GetMeanLifeTime
// ---------------
//

G4double OpFastScintillation::GetMeanLifeTime(const G4Track&,
                                          G4ForceCondition* condition)
{
        *condition = Forced;

        return DBL_MAX;

}

G4double OpFastScintillation::sample_time(G4double tau1, G4double tau2)
{
// tau1: rise time and tau2: decay time

        while(1) {
          // two random numbers
          G4double ran1 = G4UniformRand();
          G4double ran2 = G4UniformRand();
          //
          // exponential distribution as envelope function: very efficient
          //
          G4double d = (tau1+tau2)/tau2;
          // make sure the envelope function is 
          // always larger than the bi-exponential
          G4double t = -1.0*tau2*std::log(1-ran1);
          G4double g = d*single_exp(t,tau2);
          if (ran2 <= bi_exp(t,tau1,tau2)/g) return t;
        }
        return -1.0;
}

// Get random number based on parametrization
// ---------------
//
double OpFastScintillation::TimingParam(const double & distance){

		 TF1* fit4 = new TF1("testParams",mixLaga,0,150,6);
		    fit4->SetParameters(exp(6.97332-0.890701*distance),
		                        1.22647,
		                        2.44005 + 2.21248*distance,
		                        exp(6.5268 - 0.56354*distance),
		                        exp(0.0342685+0.310659*distance),
		                        exp(1.67014 + 0.278709*distance));
		
		
		    fit4->FixParameter(1,1.22647-0.2);
		    fit4->FixParameter(2,2.44005+2.21248*distance);
		    fit4->FixParameter(4,exp(-0.2344980+ 0.408152*distance)); 
		    fit4->FixParameter(5,exp(1.67014 + 0.278709*distance));
		
		    if(distance < 4.4){
		        fit4->FixParameter(0,exp( 7.4 - 1*distance));
		        fit4->FixParameter(3,exp(8.69024 - 1.15069*distance));
		        }   
		    else{
		        fit4->FixParameter(0,exp(6.97332-0.890701*distance));
		        fit4->FixParameter(3,exp( 6.5268-0.56354*distance));
		        }   

//      Below is the parametrization for 5x5x5 voxels
//       TF1* fit4 = new TF1("testParams",mixLaga,0,80,6);
//        fit4->SetParameters(exp(11.288-1.34131*distance),
//                            0.301818 + 0.0109287*distance,
//                            0.427641 +1.6214*distance,
//                            exp(9.27407-0.73157*distance),
//                            exp(-1.10106 +0.41167*distance),  
//                            -8.87049 +4.29312*distance);
//
//        fit4->FixParameter(0,exp(11.288-1.34131*distance));
//        fit4->FixParameter(1,0.301818 + 0.0109287*distance);
//        fit4->FixParameter(2,0.427641 +1.6214*distance);
//        fit4->FixParameter(4,exp(-1.10106 +0.41167*distance));    
//
//        if(distance<5.3){
//           fit4->FixParameter(3,exp(12.2459-1.35122*distance));
//           fit4->FixParameter(5,-7.5 +4.29312*distance);
//          }   
//        else if(distance>=5.3 && distance < 9){ 
//           fit4->FixParameter(3,exp(9.78835-0.828446 *distance));
//           fit4->FixParameter(5,-8.87049 +4.29312*distance);
//          }   
//        else{
//           fit4->FixParameter(3,711.937 -172.824*distance +14.0902*distance*distance-0.382049*distance*distance*distance);
//           fit4->FixParameter(5,-8.87049 +4.29312*distance);
//           }   
//
		double paramRand = fit4->GetRandom() ;

        return paramRand ; 
    }

double mixLaga(double *x, double *par) {

 double fmixGaus;
 double sum = 0.0;

 if(x[0] >= par[2] - 3*par[1])
    fmixGaus = par[0]*TMath::Gaus( x[0],par[2], par[1]) + par[3]*TMath::Landau(x[0],par[5],par[4]) ; 
 else 
    fmixGaus= 0;

 sum+=fmixGaus ;

 return sum; 
}

// Get random time (ns) from a simplified parametrized approximation of 
// the direct light distribution, as obtained in a toy MC.
// Input argument units: meters
// ----------
double OpFastScintillation::TimingParamDirect(TVector3 ScintPoint, TVector3 OpDetPoint) {
  
  // This function takes as input the scintillation point and the optical detector
  // point (in m), and returns a direct photon propagation time (with Rayleigh scattering).

  double c_LAr_VUV = 0.13; // meters per second (~group velocity)

  double distance = (ScintPoint - OpDetPoint).Mag(); // this must be in METERS!
  double mpv      = -3.933 + 15.26*distance;
  double width    = 1.81 + 0.214*mpv;

  TF1 *fDirectTiming = new TF1("fDirectTiming",DoubleLandauCutoff,0.,250.,7);
  fDirectTiming->SetNpx(10000);

  fDirectTiming->FixParameter(0.,distance/c_LAr_VUV);
  fDirectTiming->FixParameter(1.,mpv);
  fDirectTiming->FixParameter(2.,width);

  // Set 2nd landau normalization to zero, turning
  // this into a simple 1-landau functino
  fDirectTiming->FixParameter(3,0.);
  fDirectTiming->FixParameter(4,1.); // irrelevant parameter for now
  fDirectTiming->FixParameter(5,1.); // irrelevant parameter for now
  fDirectTiming->FixParameter(6,1.); // overall normalization

  return fDirectTiming->GetRandom();
}



// Get random time (ns) from a parametrized approximation to the reflected
// light distribution, as obtained in a toy MC.
// (UNDER CONSTRUCTION)
// Input argument units: meters
// ---------------
double OpFastScintillation::TimingParamReflected(TVector3 ScintPoint, TVector3 OpDetPoint ) {
  
  // This function takes as input the scintillation point and the optical 
  // detector point (in m), and uses this information to return a photon propagation 
  // time assuming the field cage walls and cathode are covered ~100% uniformly 
  // in TPB reflector foil (100% efficiency for VUV conversion, 95% diffuse 
  // reflectance to blue-visible light).
  //
  // Note:  Modeling photon arrival times as a function of scintillation
  //        position in a fully reflective system is complicated, given there
  //        is no obvious "distance to PMT" handle we can use to parametrize
  //        the distribution.  This parametrization is only an approximation 
  //        to this timing, and should be treated accordingly.
  //       
  //        The arrival time distributions are well-described by the double-
  //        landau function, DoubleLandauCutoff.  However, this has several
  //        free parameters that were not able to be parametrized.  So for 
  //        now, we are zeroing-out the second ("fast") landau in the function
  //        and thus treating it as a single landau with a t0-cutoff.
  //
  //        This parametrization was developed specifically for the SBND
  //        half-TPC (2x4x5m).  It's not yet known if it holds for other 
  //        dimensions.  These dimensions are thus hard-coded in for now.
  //        

  // Convert to center-of-detector origin coordinates, reversing the 
  // x direction to go from APA to cathode (RHR!!)
  ScintPoint[0] = -1.*(ScintPoint[0]-1.);
  OpDetPoint[0] = -1.*(OpDetPoint[0]-1.);
  ScintPoint[2] -= 2.5;
  OpDetPoint[2] -= 2.5;

  // Each plane's position in x,y,z
  //    index 0: APA (x)
  //    index 1: cathode plane (x)
  //    index 2: bottom (y)
  //    index 3: top (y)
  //    index 4: upstream wall (z)
  //    index 5: downstream wall (z)
  double  plane_depth[6] = {-1.,1.,-2.,2.,-2.5,2.5};
  int     dir_index_norm[6] = {0,0,1,1,2,2};

  // Speed of light in LAr, refractive indices
  double n_LAr_VUV = 2.632;             // Effective index due to group vel.
  double n_LAr_vis = 1.23;
  double c_LAr_VUV = 0.12;              // m per s
  double c_LAr_vis = 0.29979/n_LAr_vis; // m per s

  // TVectors to be used later
  TVector3 image(0,0,0);
  TVector3 bounce_point(0,0,0);
  TVector3 hotspot(0,0,0);
  TVector3 v_to_wall(0,0,0);

  // First find shortest 1-bounce path and fill
  // vector of "tAB" paths, each with their weighting:
  double t0   = 99;
  double tAB[5];
  double WAB=0;
  double tAB_sum=0; 
  int counter = 0;
  for(int j = 1; j<6; j++){
    
    v_to_wall[dir_index_norm[j]] = plane_depth[j] - ScintPoint[dir_index_norm[j]]; 
    
    // hotspot is point on wall where TPB is
    // activated most intensely by the scintillation
    hotspot = ScintPoint + v_to_wall;

    // define "image" by reflecting over plane
    image = hotspot + v_to_wall*(n_LAr_vis/n_LAr_VUV);

    // find point of intersection with plane j of ray 
    // from the PMT to the image
    TVector3  tempvec = (OpDetPoint-image).Unit();
    double    tempnorm= ((image-hotspot).Mag())/tempvec[dir_index_norm[j]];
    bounce_point = image + tempvec*tempnorm;

    // find t, check for t0    
    double t1 = (ScintPoint-bounce_point).Mag()/c_LAr_VUV;
    double t2 = (OpDetPoint-bounce_point).Mag()/c_LAr_vis;
    double t  = t1 + t2;
    if( t < t0 ) t0 = t;

    // find "tAB" and its weight
    double dA = (ScintPoint-hotspot).Mag();
    double dB = (OpDetPoint-hotspot).Mag();
    double tA = dA/c_LAr_VUV;
    double tB = dB/c_LAr_vis;
    double hotspot_weight = 1./pow(dA,2.) - 0.0294/pow(dA,3.);
    double tAB_w          = hotspot_weight/(1.+dB*dB);
    tAB[counter]  =  tA + tB;
    tAB_sum       += (tA+tB)*tAB_w;
    WAB           += tAB_w;

    counter++;

  } //<-- end loop over 5 foil-covered planes

  // weighted mean
  double tAB_mean = tAB_sum/WAB;

  // now find tAB_spread
  double tAB_spread = 0;
  for(int jj=0; jj<counter; jj++) tAB_spread += fabs( tAB[jj] - tAB_mean )/counter; 

  // Now we have all the info we need for the parametrization
  double mpv    = 93.695 - 16.572*tAB_spread + 2.064*pow(tAB_spread,2) - 0.1177*pow(tAB_spread,3) + 0.002025*pow(tAB_spread,4); 
  double width  = 14.75 - 0.4295*tAB_spread;  

  TF1 *fReflectedTiming = new TF1("fReflectedTiming",DoubleLandauCutoff,0.,250.,7);
  fReflectedTiming -> SetNpx(10000);
  fReflectedTiming -> FixParameter(0,t0);
  fReflectedTiming -> FixParameter(1,mpv);
  fReflectedTiming -> FixParameter(2,width);
  fReflectedTiming -> FixParameter(3,0);  // ***setting "fast" landau to zero***
  fReflectedTiming -> FixParameter(4,1.); // fast landau mpv (irrelevant for now)
  fReflectedTiming -> FixParameter(5,1.); // fast landau width (irrelevant for now)
  fReflectedTiming -> FixParameter(6,1.); // overall normalization 

  return fReflectedTiming->GetRandom();
}


double DoubleLandauCutoff(double *x, double *par){
  // par0 = t0 cutoff (no causality violation)
  // par1 = main Landau MPV
  // par2 = main Landau width
  // par3 = normalization of "fast" Landau
  //        relative to main
  // par4 = MPV of fast Landau (relative to
  //        the calculated t0)
  // par5 = width of fast Landau
  // par6 = overall function normalization

  double y = 0.;
  if( x[0] > par[0]) {
    y = par[6]*(TMath::Landau(x[0],par[1],par[2])          
      + par[3]*TMath::Landau(x[0],par[0]+par[4],par[5]));
  } else {
    y = 0.;
  }

  return y;
}

} //namespace larg4
