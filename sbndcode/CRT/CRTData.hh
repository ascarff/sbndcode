#ifndef CRTData_hh_
#define CRTData_hh_

#include <stdint.h> //uint32_t
#include <TVector3.h>

namespace sbnd {
namespace crt {

  class CRTData {
    uint32_t fChannel;
    uint32_t fT0;
    uint32_t fT1;
    uint32_t fADC;
    double fTime;  // Truth!
    TVector3 fHitPos;  // Truth!
    TVector3 fMomentum; //trajectory
   public:
    CRTData();
    CRTData(uint32_t channel, uint32_t t0, uint32_t t1, uint32_t adc, double t, TVector3 p, TVector3 l);
    virtual ~CRTData();
    uint32_t Channel() const;
    uint32_t T0() const;
    uint32_t T1() const;
    uint32_t ADC() const;
    double Time() const;
    TVector3 HitPos() const;
    TVector3 Momentum() const;
  };

} // namespace crt
} // namespace sbnd

#endif
