//
//  lariat-online/daq/src/RawBuffer.cc    (W.Badgett
//

#include "RawBuffer.h"
#include <cstring>
#include <algorithm>

RawBuffer::RawBuffer(char *data, unsigned int len)
{
  setg(data,data,(data+len));
}

std::streampos RawBuffer::seekoff(std::streamoff     off,
				  std::ios::seekdir  dir,
				  std::ios::openmode which) 
{
  std::streamsize actual = 0;
  if ( which == std::ios_base::in ) 
  { 
    actual = std::min<std::streamsize>(off,showmanyc());
    setg(eback(),(gptr()+actual),egptr());
  } 
  return((std::streampos)(gptr()-eback()));
}

std::streampos RawBuffer::seekpos(std::streampos pos,
				  std::ios_base::openmode which) 
{
  std::streamsize actual = 0;
  if ( which == std::ios_base::in ) 
  { 
    actual = std::min<std::streamsize>((std::streamoff)pos,(egptr()-eback()));
    setg(eback(),(eback()+actual),egptr());
  }
  return((std::streampos)(gptr()-eback()));
}

std::streamsize RawBuffer::showmanyc() 
{ 
  std::streamsize size =  ( egptr() - gptr() );
  if ( size <= 0 ) size = -1;
  return(size);
} 

std::streamsize RawBuffer::xsgetn(char * data, std::streamsize size) 
{
  std::streamsize actual = std::min<std::streamsize>(size,showmanyc());
  if ( actual > 0 )
  {
    memcpy(data,gptr(),actual);
    setg(eback(),(gptr()+actual),egptr());
    return(actual);
  }
  return(EOF);
}

int RawBuffer::underflow() 
{
  if ( showmanyc() > 0 ) { return ( (int)(*gptr()) ); }
  else { return(EOF);}
}

int RawBuffer::uflow() 
{
  if ( showmanyc() > 0 )
  {
    int reply = (int)(*gptr());
    setg(eback(),(gptr()+reply),egptr());
    return ( reply );
  }
  else { return(EOF);}
}

// Don't let user alter input stream contents
int RawBuffer::pbackfail(int c) 
{
  if ( gptr() > eback() )
  {
    char * ptr = gptr() - 1;
    setg(eback(),ptr,egptr());
    return( (int)(*gptr())); // Return actual contents, which may vary
  }
  else { return(EOF); }
}
