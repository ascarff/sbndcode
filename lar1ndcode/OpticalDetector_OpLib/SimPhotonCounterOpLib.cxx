#include "SimPhotonCounterOpLib.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <functional>

opdet::SimPhotonCounterOpLib::SimPhotonCounterOpLib(size_t s,
					  float t_p1, float t_p2,
					  float t_l1, float t_l2,
					  float min_w, float max_w,
					  float e)
{

  SetWavelengthRanges(min_w,max_w);
  SetTimeRanges(t_p1,t_p2,t_l1,t_l2);

  _photonVector_prompt=std::vector<float>(s);
  _photonVector_late=std::vector<float>(s);
  _qeVector = std::vector<float>(s,e);

}

opdet::SimPhotonCounterOpLib::SimPhotonCounterOpLib(float t_p1, float t_p2,
					  float t_l1, float t_l2,
					  float min_w, float max_w,
					  const std::vector<float>& eV)
{
  SetWavelengthRanges(min_w,max_w);
  SetTimeRanges(t_p1,t_p2,t_l1,t_l2);

  _photonVector_prompt=std::vector<float>(eV.size());
  _photonVector_late=std::vector<float>(eV.size());
  _qeVector = eV;

}

void opdet::SimPhotonCounterOpLib::SetWavelengthRanges(float min_w, float max_w)
{
  if(min_w >= max_w)
    throw std::runtime_error("ERROR in SimPhotonCounterOpLib: bad wavelength range");

  _min_wavelength = min_w;
  _max_wavelength = max_w;
}

float opdet::SimPhotonCounterOpLib::Wavelength(const sim::OnePhoton& ph)
{
  if(ph.Energy < std::numeric_limits<float>::epsilon())
    throw std::runtime_error("ERROR in SimPhotonCounterOpLib: photon energy is zero.");

  return 0.00124 / ph.Energy;
}

void opdet::SimPhotonCounterOpLib::SetTimeRanges(float t_p1, float t_p2, float t_l1, float t_l2)
{
  if(t_p2<t_p1 || t_l2<t_l1 || t_p2>t_l1 )
    throw std::runtime_error("ERROR in SimPhotonCounterOpLib: bad time ranges");

  _min_prompt_time = t_p1; _max_prompt_time = t_p2;
  _min_late_time = t_l1; _max_late_time = t_l2;
}

void opdet::SimPhotonCounterOpLib::AddOnePhoton(size_t i_opdet, const sim::OnePhoton& photon)
{
  if(i_opdet > GetVectorSize())
    throw std::runtime_error("ERROR in SimPhotonCounterOpLib: Opdet requested out of range!");

  if(Wavelength(photon) < _min_wavelength || Wavelength(photon) > _max_wavelength) return;
  
  if(photon.Time > _min_prompt_time && photon.Time <= _max_prompt_time)
    _photonVector_prompt[i_opdet] += _qeVector[i_opdet];
  else if(photon.Time > _min_late_time && photon.Time < _max_late_time)
    _photonVector_late[i_opdet] += _qeVector[i_opdet];
    
}

void opdet::SimPhotonCounterOpLib::AddSimPhotons(const sim::SimPhotons& photons)
{
  for(size_t i_ph=0; i_ph < photons.size(); i_ph++)
    AddOnePhoton(photons.OpChannel(),photons[i_ph]);
}

void opdet::SimPhotonCounterOpLib::ClearVectors()
{
  for(size_t i=0; i<GetVectorSize(); i++){
    _photonVector_prompt[i]=0.0;
    _photonVector_late[i]=0.0;
  }
}

std::vector<float> opdet::SimPhotonCounterOpLib::TotalPhotonVector() const{
  
  std::vector<float> totalPhotonVector(GetVectorSize());
  std::transform(PromptPhotonVector().begin(),PromptPhotonVector().end(),
		 LatePhotonVector().begin(),
		 totalPhotonVector.begin(),
		 std::plus<float>());
  return totalPhotonVector;
}

void opdet::SimPhotonCounterOpLib::Print()
{
  std::cout << "Vector size: " << GetVectorSize() << std::endl;
  std::cout << "Time cut ranges: ("
	    << MinPromptTime() << "," << MaxPromptTime() << ") , ("
	    << MinLateTime() << "," << MaxLateTime() << ")" << std::endl;
  std::cout << "\t" << "i : QE / Prompt / Late / Total" << std::endl;
  for(size_t i=0; i<GetVectorSize(); i++)
    std::cout << "\t" << i << ": " << _qeVector[i] << " / " << _photonVector_prompt[i] << " / " << _photonVector_late[i] << " / " << TotalPhotonVector(i) << std::endl;
  
}
