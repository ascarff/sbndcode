#include <cassert>
#include "sbndcode/FluxReader/NTupleInterface.h"
#include "TFile.h"
#include "TTree.h"

namespace sbnd {

NTupleInterface::NTupleInterface() {}

NTupleInterface::~NTupleInterface() {}

void NTupleInterface::SetRootFile(TFile* inputFile) {
  fFluxTree=dynamic_cast<TTree*>(inputFile->Get("testmod/flux"));
  
  fFluxTree->SetBranchAddress("vtxx" ,&fvtxx);
  fFluxTree->SetBranchAddress("vtxy" ,&fvtxy);
  fFluxTree->SetBranchAddress("vtxz" ,&fvtxz);
  fFluxTree->SetBranchAddress("px"   ,&fpx);
  fFluxTree->SetBranchAddress("py"   ,&fpy);
  fFluxTree->SetBranchAddress("pz"   ,&fpz);
  fFluxTree->SetBranchAddress("E"    ,&fE);
  fFluxTree->SetBranchAddress("pdg"  ,&fpdg);
  fFluxTree->SetBranchAddress("ptype",&fptype);
  fFluxTree->SetBranchAddress("wgt"  ,&fwgt);
  fFluxTree->SetBranchAddress("dist" ,&fdist);
  fFluxTree->SetBranchAddress("evtno"  ,&frun);
  fFluxTree->SetBranchAddress("nenergyn", &fnenergyn);

  fNEntries = fFluxTree->GetEntries();
  assert(fNEntries > 0);
  fFluxTree->GetEntry(0);
  fRun = frun;
}

bool NTupleInterface::FillMCFlux(Long64_t ientry, simb::MCFlux& flux) {
  if (!fFluxTree->GetEntry(ientry))
    return false;

  fNuPos=TLorentzVector(fvtxx, fvtxy, fvtxz, 0);
  fNuMom=TLorentzVector(fpx  , fpy  , fpz  , fE);

  flux.fptype = fptype;
  flux.fntype  = fpdg;
  flux.fnimpwt = fwgt;
  flux.fdk2gen = fdist;
  flux.fnenergyn = fnenergyn;

  std::cout << ">> vtxx: " << fvtxx << std::endl;
  std::cout << ">> vtxy: " << fvtxy << std::endl;
  std::cout << ">> vtxz: " << fvtxz << std::endl;
  std::cout << ">> px  : " << fpx << std::endl;
  std::cout << ">> py  : " << fpy << std::endl;
  std::cout << ">> pz  : " << fpz << std::endl;
  std::cout << ">> E   : " << fE << std::endl;
  std::cout << ">> ptype:" << fptype << std::endl;
  std::cout << ">> ntype : " << fpdg << std::endl;
  std::cout << ">> wgt : " << fwgt << std::endl;
  std::cout << ">> dist: " << fdist << std::endl;
  std::cout << ">> nenergyn: " << fnenergyn << std::endl;

  return true;
}

}  // namespace sbnd

