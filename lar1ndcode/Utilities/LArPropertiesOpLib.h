////////////////////////////////////////////////////////////////////////
// LArPropertiesOpLib.h
//
// Utility LAr functions
//
// maddalena.antonello@lngs.infn.it
// ornella.palamara@lngs.infn.it
// msoderbe@syr.edu
// joshua.spitz@yale.edu
//
// Optical Properties:
// bjpjones@mit.edu
//
////////////////////////////////////////////////////////////////////////
#ifndef LARPROPERTIES_H
#define LARPROPERTIES_H

#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "string.h"



///General LArSoft Utilities
namespace util{
    class LArPropertiesOpLib {
    public:
      LArPropertiesOpLib(fhicl::ParameterSet const& pset, art::ActivityRegistry& reg);
      ~LArPropertiesOpLib();

      void   reconfigure(fhicl::ParameterSet const& pset);

      double Density(double temperature=0.) const;                          ///< g/cm^3
      double DriftVelocity(double efield=0., double temperature=0.) const;  ///< cm/us

      /// dQ/dX in electrons/cm, returns dE/dX in MeV/cm.
      double BirksCorrection(double dQdX) const;
      double ModBoxCorrection(double dQdX) const;

      double Efield(unsigned int planegap=0) const;                             ///< kV/cm
      double Temperature()                   const; 				///< kelvin	    
      double ElectronLifetime() 	     const; 				///< microseconds
      double RadiationLength()  	     const { return fRadiationLength; } ///< g/cm^2      

      double Argon39DecayRate()              const { return fArgon39DecayRate; }  // decays per cm^3 per second

      /// Restricted mean dE/dx energy loss (MeV/cm).
      double Eloss(double mom, double mass, double tcut) const;

      /// Energy loss fluctuation (sigma_E^2 / length in MeV^2/cm).
      double ElossVar(double mom, double mass) const;


      double ScintResolutionScale() const { return fScintResolutionScale; }
      double ScintFastTimeConst()   const { return fScintFastTimeConst;   } 
      double ScintSlowTimeConst()   const { return fScintSlowTimeConst;   }
      double ScintBirksConstant()   const { return fScintBirksConstant;   }

      bool ScintByParticleType()    const { return fScintByParticleType;  }

      double ScintYield()           const { return fScintYield;           }
      double ScintYieldRatio()      const { return fScintYieldRatio;      }

      double ProtonScintYield()       const { return fProtonScintYield;         }
      double ProtonScintYieldRatio()  const { return fProtonScintYieldRatio;    }
      double MuonScintYield()         const { return fMuonScintYield;           }
      double MuonScintYieldRatio()    const { return fMuonScintYieldRatio;      }
      double KaonScintYield()         const { return fKaonScintYield;           }
      double KaonScintYieldRatio()    const { return fKaonScintYieldRatio;      }
      double PionScintYield()         const { return fPionScintYield;           }
      double PionScintYieldRatio()    const { return fPionScintYieldRatio;      }
      double ElectronScintYield()     const { return fElectronScintYield;       }
      double ElectronScintYieldRatio() const { return fElectronScintYieldRatio;  }
      double AlphaScintYield()        const { return fAlphaScintYield;          }
      double AlphaScintYieldRatio()   const { return fAlphaScintYieldRatio;     }

      bool CerenkovLightEnabled()     const { return fEnableCerenkovLight;      }

      bool ExtraMatProperties() const { return fExtraMatProperties; }
      double TpbTimeConstant()  const { return fTpbTimeConstant;     }
      bool SimpleBoundary()     const { return fSimpleBoundary;      }
      bool SimpleScint()    const { return fSimpleScint;      }


      
      std::map<double, double>  SlowScintSpectrum();   
      std::map<double, double>  FastScintSpectrum();
      std::map<double, double>  TpbAbs();   
      std::map<double, double>  TpbEm();
      std::map<double, double>  RIndexSpectrum();
      std::map<double, double>  AbsLengthSpectrum();
      std::map<double, double>  RayleighSpectrum();

      std::map<std::string, std::map<double, double> > SurfaceReflectances();
      std::map<std::string, std::map<double, double> > SurfaceReflectanceDiffuseFractions();
      std::map<std::string, std::map<double, double> > SurfaceTpbReflectances();
      std::map<std::string, std::map<double, double> > SurfaceReflectanceTpbDiffuseFractions();

    private:

      void preBeginRun(art::Run const& run);
      void checkDBstatus() 	const;
      
      
      std::vector< double >          fEfield;           ///< kV/cm
      double                         fTemperature;      ///< kelvin
      double                         fElectronlifetime; ///< microseconds
      double                         fRadiationLength;  ///< g/cm^2

      double                         fArgon39DecayRate; ///<  decays per cm^3 per second

      bool			     fAlreadyReadFromDB; ///< tests whether the values have alread been picked up from the Database
      
      // Following parameters are for use in Bethe-Bloch formula for dE/dx.

      double fZ;                ///< Ar atomic number
      double fA;                ///< Ar atomic mass (g/mol)
      double fI;                ///< Ar mean excitation energy (eV)
      double fSa;               ///< Sternheimer parameter a
      double fSk;               ///< Sternheimer parameter k
      double fSx0;              ///< Sternheimer parameter x0
      double fSx1;              ///< Sternheimer parameter x1
      double fScbar;            ///< Sternheimer parameter Cbar


      // Optical parameters for LAr

      std::vector<double> fFastScintSpectrum;
      std::vector<double> fFastScintEnergies;
      std::vector<double> fSlowScintSpectrum;
      std::vector<double> fSlowScintEnergies;
      std::vector<double> fRIndexSpectrum;
      std::vector<double> fRIndexEnergies;
      std::vector<double> fAbsLengthSpectrum;
      std::vector<double> fAbsLengthEnergies;
      std::vector<double> fRayleighSpectrum;
      std::vector<double> fRayleighEnergies;

      bool fScintByParticleType;

      double fProtonScintYield;
      double fProtonScintYieldRatio; 
      double fMuonScintYield;
      double fMuonScintYieldRatio; 
      double fPionScintYield;
      double fPionScintYieldRatio; 
      double fKaonScintYield;
      double fKaonScintYieldRatio; 
      double fElectronScintYield;
      double fElectronScintYieldRatio; 
      double fAlphaScintYield;
      double fAlphaScintYieldRatio; 
	double fTpbTimeConstant;
      double fScintYield;
      double fScintResolutionScale;
      double fScintFastTimeConst;
      double fScintSlowTimeConst;
      double fScintYieldRatio;
      double fScintBirksConstant;

      bool fEnableCerenkovLight;
      bool fExtraMatProperties;
      bool fSimpleBoundary;
      bool fSimpleScint;


      std::vector<std::string>          fReflectiveSurfaceNames;
      std::vector<double>               fReflectiveSurfaceEnergies;
      std::vector<std::vector<double> > fReflectiveSurfaceReflectances;
      std::vector<std::vector<double> > fReflectiveSurfaceDiffuseFractions;
      std::vector<double>               fTpbEmmisionEnergies;
      std::vector<double>               fTpbEmmisionSpectrum;
      std::vector<double>               fTpbAbsorptionEnergies;
      std::vector<double>               fTpbAbsorptionSpectrum;
      std::vector<std::string>          fReflectiveSurfaceTpbNames;
      std::vector<double>               fReflectiveSurfaceTpbEnergies;
      std::vector<std::vector<double> > fReflectiveSurfaceTpbReflectances;
      std::vector<std::vector<double> > fReflectiveSurfaceTpbDiffuseFractions;
      
    }; // class LArPropertiesOpLib
} //namespace utils
DECLARE_ART_SERVICE(util::LArPropertiesOpLib, LEGACY)
#endif // LARPROPERTIES_H
