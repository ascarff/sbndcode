//
// File:  lariat-online/daq/include/RawBuffer.h   (W.Badgett)
//

#ifndef RAW_BUFFER_H_
#define RAW_BUFFER_H_

#include <iostream>

class RawBuffer: public std::streambuf
{
  public:
    RawBuffer(char *data, unsigned int len);

  protected:
    std::streampos seekoff(std::streamoff     off,
			   std::ios::seekdir  dir,
			   std::ios::openmode which) override;
    std::streampos seekpos(std::streampos pos,
			   std::ios_base::openmode which) override;
    std::streamsize showmanyc() override;
    std::streamsize xsgetn(char * data, std::streamsize size) override;
    int underflow() override;
    int uflow() override;
    int pbackfail(int c) override;

};

#endif
