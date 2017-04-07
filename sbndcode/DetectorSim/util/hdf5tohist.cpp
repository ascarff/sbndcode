/**
 * Convert raw ADC test stand data in HDF5 format to a 2D ROOT histogram in
 * ideal & real ADC, for input to sbndcode's SimWireDataADC.
 *
 * Usage:
 *
 *   ./hdf5tohist OUTPUT.root INPUT.hdf5 CHIP_INDEX
 *
 *   CHIP_INDEX is a sequential ID number, to allow running this script in
 *   parallel.
 *
 * Build e.g.:
 *
 *   g++ -o hdf5tohist hdf5tohist.cpp \
 *     `root-config --cflags --libs` -lhdf5 -lhdf5_hl -lHist
 *
 * Revisions:
 *   2017/03/29 ATM - Small hists of distributions around mean, plus offset
 *
 * A. Mastbaum <mastbaum@uchicago.edu>, 2017/03/12
 */

#include <cassert>
#include <iostream>
#include <TFile.h>
#include <TH2S.h>
#include <hdf5.h>
#include <hdf5_hl.h>

int main(int argc, char* argv[]) {
  assert(argc == 4);

  hid_t file_id;
  herr_t status;

  // Open HDF5 file
  file_id = H5Fopen(argv[2], H5F_ACC_RDONLY, H5P_DEFAULT);
  std::cout << argv[2] << " " << file_id << std::endl;
  assert(file_id >= 0);

  hsize_t num_obj;
  status = H5Gget_num_objs(file_id, &num_obj);

  size_t chip_id = atoi(argv[3]);

  TFile* f = TFile::Open(argv[1], "recreate");
  f->Close();

  for (hsize_t ids=0; ids<num_obj; ids++) {
    char name[100];
    size_t size = H5Gget_objname_by_idx(file_id, ids, name, 100);
    std::string dataset = name;
    std::cout << name << std::endl;

    std::vector<unsigned int> rank;
    std::vector<uint16_t> data;

    int ndims;
    std::vector<hsize_t> dims;
    unsigned int nelements = 1;

    // Get number of dimensions in array
    status = H5LTget_dataset_ndims(file_id, name, &ndims);
    assert(status >= 0);

    // Get rank of each dimension
    dims.resize(ndims);
    status = H5LTget_dataset_info(file_id, name, &dims.front(), NULL, NULL);
    assert(status >= 0);

    rank.resize(ndims, 0);
    rank[0] += dims[0];
    nelements *= dims[0];

    for (unsigned int i=1; i<rank.size(); i++) {
      if (rank[i] != 0 && rank[i] != dims[i]) {
        H5Fclose(file_id);
        assert(false);
      }
      nelements *= dims[i];
    }

    // Read the data
    data.resize(nelements);
    status = H5LTread_dataset(file_id, name, H5T_NATIVE_SHORT, &data[0]);

    // Get input voltages
    float vlo, vhi, slope, offset;
    
    status  = H5LTget_attribute_float(file_id, name, "low_level_volts", &vlo);
    status |= H5LTget_attribute_float(file_id, name, "high_level_volts", &vhi);
    status |= H5LTget_attribute_float(file_id, name, "slope", &slope);
    status |= H5LTget_attribute_float(file_id, name, "offset", &offset);
    assert(status >= 0);

    f = TFile::Open(argv[1], "update");
    char hname[50];
    snprintf(hname, 50, "ht_%05llu", 16 * chip_id + ids);
    TH2S ht(hname, ";ADC in;ADC out", 4095, 0, 4095, 4095, 0, 4095);

    float vin, ain;
    for (hsize_t i=0; i<rank[0]; i++) {
      vin = vlo + (vhi - vlo) / rank[0] * i;
      ain = (vin * 1000 - offset) / slope;
      ht.Fill(ain, data[i]-1);
    }

    // Shrink using offsets
    snprintf(hname, 50, "hs_%05llu", 16 * chip_id + ids);
    TH2S hs(hname, ";ADC in;#Delta ADC", 4095, 0, 4095, 400, -200, 200);

    snprintf(hname, 50, "ho_%05llu", 16 * chip_id + ids);
    TH1S ho(hname, ";ADC in;Offset", 4095, 0, 4095);

    for (int i=0; i<ht.GetNbinsX()+1; i++) {
      TH1D* htemp = ht.ProjectionY("htemp", i, i);
      int maxbin = htemp->GetMaximumBin() - 1;
      ho.SetBinContent(i, maxbin);
      for (int j=0; j<400; j++) {
        hs.SetBinContent(i, j, ht.GetBinContent(i, maxbin - 200 + j));
      }
    }

    ht.Write();
    hs.Write();
    ho.Write();

    f->Flush();
    f->Close();
  }

  return 0;
}

