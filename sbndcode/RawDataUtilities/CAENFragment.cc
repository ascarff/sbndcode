//
//  Close to encode and decode the CAENFragment data structures
//

#include "CAENFragment.hh"
#include "RawBuffer.h"
#include <stdexcept>
#include <string>
#include <vector>

#include <cassert>
sbnddaq::CAENFragment::CAENFragment(const char * buffer, size_t size)
{
  char * ptr = (char *)buffer;
  RawBuffer rawBuffer(ptr,size);
  std::istream in(&rawBuffer);
  fillFromBuffer(in);
}

uint32_t sbnddaq::CAENFragment::streamToBuffer(std::ostream& buffer)
{
  uint32_t nBytes=0;
    
  //  header.fragmentType = CAENFragment::FRAGMENT_TYPE_CAEN_ADC;
  header.fragmentType = 6;

  using std::streamsize;
  const streamsize headersize=sizeof(CaenHeader_t);

  
  header.fragmentSize = headersize + 
    sizeof(WaveFormData_t::value_type)*header.nChannels*header.nSamples;
  
  // write the header
  
  buffer.write(reinterpret_cast<const char*>(&header),headersize);
  nBytes+=headersize;
  
  if(header.nChannels!=waveForms.size()) 
  {
    throw std::length_error("CAENFragment::streamToBuffer : header.nChannels!=waveForms.size()");
  }
  
  for(size_t i=0; i<header.nChannels; ++i)
  {
    const WaveForm& wave=waveForms[i];
    
    if(wave.data.size()!=header.nSamples)
    {
      throw std::length_error("CAENFragment::streamToBuffer : header.nSamples!=wave.data.size()");
    }
    // compute the size of the waveform data in bytes
    const streamsize waveSize=
	   wave.data.size()*sizeof(WaveFormData_t::value_type);
    // now write the waveform data
    // by ISO standard the elements of a vector are required to be contiguous
    // so, this ought to work!
    buffer.write(reinterpret_cast<const char*>(& (wave.data)[0]),waveSize);
    nBytes+=waveSize;
  }
  return nBytes;
}
  

uint32_t sbnddaq::CAENFragment::fillFromBuffer(std::istream& buffer)
{
  uint32_t nBytes=0;
  
  using std::streamsize;
  const streamsize headersize=sizeof(CaenHeader_t);  

  // read the header
  buffer.read(reinterpret_cast<char*>(&header),headersize);
  nBytes+=headersize;
  
  // read the channel data
  for(size_t i=0; i<header.nChannels; ++i)
  {
    WaveForm wave;
    // compute the size of the waveform data in bytes
    const streamsize waveSize = 
      header.nSamples*sizeof(WaveFormData_t::value_type);    

    // create a block of memory 
    WaveFormData_t::value_type* wave_array = 
      new WaveFormData_t::value_type[header.nSamples]; 

    // read into the memory block
    buffer.read(reinterpret_cast<char*>(wave_array),waveSize);

    std::cout << "READ " << buffer.gcount() << "/" << waveSize << std::endl;
    std::cout << (buffer ? "OK" : "FAIL") << std::endl;

    nBytes+=waveSize;
    std::cout << "wave_array[0] = " << wave_array[0] << std::endl;
    
    // push the array data into the vector inside of wave
    wave.data.assign(wave_array, wave_array+header.nSamples);    
    std::cout << "wave.data[0] = " << wave.data[0] << std::endl;


    //assert(buffer);

    waveForms.push_back(wave);
    delete[] wave_array;
  }

  return nBytes;
}

  
void sbnddaq::CAENFragment::print(std::ostream& os)
{  
  os << *this;
}

void sbnddaq::CAENFragment::printWaves(int* channels, 
				       int nChan, std::ostream& os)
{
  std::vector<int> all_channels(header.nChannels, 0);
  for(size_t i=0; i<header.nChannels; ++i) all_channels[i]=i;

  if(channels==0) {channels = all_channels.data(); nChan=header.nChannels;}
  
  // print header
  os<<"    ";
  for(int i=0; i<nChan; i++)
  {
    int channel=channels[i];
    os.width(1); os.precision(1);
    os<<"|";
    os.width(4); os.precision(4);
    os<<std::dec<<channel;
  }
  os<<'\n';

  os.width(1);
  os<<"-----";
  for(int i=0; i<nChan; i++)
  {
    os<<"-----";
  }
  os<<'\n';

  for(size_t is=0; is<header.nSamples; ++is)
  {
    os.width(4); os.precision(4); os<<std::dec<<is;
    for(int ic=0; ic<nChan; ic++)
    {
      int channel=channels[ic];
      os.width(1); os.precision(1);
      os<<"|";
      os.width(4); os.precision(4);
      os<<std::hex<<waveForms[channel].data[is];
    }
    os<<'\n';
  }
}

std::ostream& operator<<(std::ostream& os, 
			 const sbnddaq::CAENFragment::CaenHeader_t& e)
{
  os << "CAENFragment Header contents" << std::endl
     << "  FragmentSize            "<< e.fragmentSize << std::endl
     << "  FragmentType            "<< e.fragmentType << std::endl
     << "  EventCounter            "<< e.eventCounter << std::endl
     << "  EventSize               "<< e.eventSize << std::endl
     << "  BoardId                 "<< e.boardId << std::endl
     << "  Pattern                 "<< e.pattern << std::endl
     << "  TriggerTimeTag          "<< e.triggerTimeTag << std::endl
     << "  ChannelMask             "<< e.channelMask << std::endl
     << "  nChannels               "<< e.nChannels << std::endl
     << "  nSamples                "<< e.nSamples << std::endl
     << "  TimeStampSec            "<< e.timeStampSec << std::endl
     << "  TimeStampNSec           "<< e.timeStampNSec;
  return(os);
}

std::ostream& operator<<(std::ostream& os, const sbnddaq::CAENFragment& e)
{
  os << e.header;
  return(os);
}
