#include "sbndcode/CRT/CRTData.hh"

namespace sbnd{
namespace crt{

  CRTData::CRTData(): fChannel(0), fT0(0), fT1(0) { }
  CRTData::CRTData(uint32_t channel, uint32_t t0, 
    uint32_t t1, uint32_t adc, double t, TVector3 p, TVector3 l):
    fChannel(channel),
    fT0(t0),
    fT1(t1),
    fADC(adc),
    fTime(t),
    fHitPos(p),
    fMomentum(l) { }
  CRTData::~CRTData(){
  }
  uint32_t CRTData::Channel() const {
    return fChannel;
  }
  uint32_t CRTData::T0() const {
    return fT0;
  }
  uint32_t CRTData::T1() const {
    return fT1;
  }
  uint32_t CRTData::ADC() const {
    return fADC;
  }
  double CRTData::Time() const {
    return fTime;
  }
  TVector3 CRTData::HitPos() const {
    return fHitPos;
  }
  TVector3 CRTData::Momentum() const {
    return fMomentum;
 }
} // namespace crt
} // namespace sbnd
