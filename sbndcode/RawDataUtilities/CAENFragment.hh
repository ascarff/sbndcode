//
//  Definition of the raw data format for SBND light readout wave forms
//  CAEN  wave form digitizers.     (W.Badgett)
//

#ifndef _sbnddaq_readout_Generators_CAENFragment
#define _sbnddaq_readout_Generators_CAENFragment

#include <iosfwd>
#include <vector>

// Fragment overlay class for SBND PMT wave form data.
// NB:  Needs to go into separate package for offline+online use
//
// Structure of an event record is:
//   One struct Header
//   Header.nChannels number of struct ChannelData
//

#include <vector>
#include <iostream>

namespace sbnddaq
{
class CAENFragment 
{
 public:
  CAENFragment(const char * buffer, size_t size);

  enum
  {
    MAX_BOARDS = 8
  };

#pragma pack(push,1)

  typedef struct CaenHeader 
  {
    uint32_t fragmentSize;
    uint32_t fragmentType;
    uint32_t eventCounter;
    uint32_t eventSize;
    uint32_t boardId;
    uint32_t pattern;
    uint32_t triggerTimeTag;
    uint32_t channelMask;
    uint32_t nChannels;
    uint32_t nSamples;
    uint32_t timeStampSec;
    uint32_t timeStampNSec;
  } CaenHeader_t;

  typedef struct CaenChannelData
  {
    uint16_t *data;
  } CaenChannelData_t;

#pragma pack(pop)

  typedef std::vector<uint16_t> WaveFormData_t;
  class WaveForm{
  public:
    WaveFormData_t data;
  };
  
  CaenHeader_t header;
  std::vector<WaveForm> waveForms;
  
  /**
     * stream the data from this object into a buffer.
     * Returns the number of characters written as a convenience.
     * This function sets fragmentSize and fragmentType.
     **/
  uint32_t streamToBuffer(std::ostream& buffer);
  
  /**
   * Fill in this object, assuming that the current position
   * in the buffer coincides with the start of this object.
   * Returns the number of characters read as a convenience.
   * Obviously streamToBuffer() and fillFromBuffer() must agree on
   * the structure of the object in the buffer!
   **/
  uint32_t fillFromBuffer(std::istream& buffer);

  /**
   * print this object in a user readable format
   **/
  void print(std::ostream& os = std::cout);

  /**
   * print this object in a user readable format
   **/
  void printWaves(int* chans=0, int nchan=0, std::ostream& os=std::cout);


};
}

std::ostream& operator<<(std::ostream& os, const sbnddaq::CAENFragment& e);
std::ostream& operator<<(std::ostream& os, 
			 const sbnddaq::CAENFragment::CaenHeader_t& e);

#endif
