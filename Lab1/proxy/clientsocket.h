// Definition of the ClientSocket class

#ifndef ClientSocket_class
#define ClientSocket_class

#include "socket.h"


class ClientSocket : private Socket
{
 public:

  ClientSocket ( int host, int port );
  virtual ~ClientSocket(){};

  const ClientSocket& operator << ( const std::string& ) const;
  const ClientSocket& operator >> ( std::string& ) const;
  
  void close();
  int getFD();

};


#endif
