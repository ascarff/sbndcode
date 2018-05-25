#include "Purity.hh"

// Function to calculate the electron lifetime from a collection of hits
double daqAnalysis::CalculateLifetime(std::vector<art::Ptr<recob::Hit>> rawhits) {
  // You have a vector of recob::Hit objects (you can google recob::Hit to find 
  // out what info is stored in them)
  // Apply a minimum hit cut
  // Plot the WireID().Wire against PeakTime()
  // Do a linear fit, cut hits a certain distance from the fit
  // Count up the number of unique hit wires, apply a cut on the minimum number
  // Plot the Integral() against PeakTime()
  // The charge loss of a minimum ionizing particle looks like a landau gaussian 
  // convolution.
  // Electronegative impurities (e.g. oxygen, water) will absorb charge, the 
  // further charge has to travel the more of it will be absorbed (the electrons
  // have a shorter "lifetime"). This effect is exponential.
  // ==> The charge vs time graph will take the form of a landau-gaussian 
  // convolution in fixed time slices and the peak charge will decay exponentially.
  // To measure the exponential decay with a single track you are probably going to
  // want to do a maximum likelihood estimation, let me know when you get to this
  // point and we can talk about it more.
  
  // Return the lifetime here
  return 1;
}
