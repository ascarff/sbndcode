***DaqAnalysis***

Code for online analysis of data acquisition electronics for the
Vertical Slice Test (VST). The code is organized into two art 
modules--`DaqDecoder` (a producer) and `SimpleDaqAnalysis` (an 
analyzer). 

`DaqDecoder` takes in an art root
file prepared by e.g. `test_driver` that contains a vector of 
`NevisTPCFragment` stored in the file as `artdaq::Fragment`. It decodes
those fragments and turns them into `raw::RawDigits`. An example fcl
file for running `DaqDecoder` is given in `fcl/decoder.fcl`.

Those `raw::RawDigits` can be sent to a number of larsoft modules. An
example module, `GausHitFinder`, is given in the `pipeline.fcl` file in
the `fcl` sub-directory. 

In addition, the `SimpleDaqAnalysis` module takes `raw::RawDigits` as
input. `SimpleDaqAnalysis` creates metrics for the input waveforms such
as FFT's, peak finding, and noise RMS and cross-channel correlation. It
stores output in a new root file provided by a `TFileService`. An
example fcl file for running `SimpleDaqAnalysis` is given in
`fcl/analysis.fcl`. 

**Building**

After checking out this feature branch, you can build as normal by
running `mrb i`.

**Provided Configuration Files**

-`decoder.fcl`: runs just the `DaqDecoder` module, taking in some art
root file with `artdaq::Fragment` as input
-`analysis.fcl`: runs just the `SimpleDaqAnalysis` module, taking in
some art root file with `raw::RawDigits` as input
-`simple_pipeline.fcl`: runs first the `DaqDecoder` module and then
feeds that into `SimpleDaqAnalysis`
-`pipeline.fcl`: runs first the `DaqDecoder` module and then feeds that
into both `SimpleDaqAnalysis` and the default `sbndcode` `GausHitFinder`
