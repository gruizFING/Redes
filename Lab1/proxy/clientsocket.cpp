// Implementation of the ClientSocket class

#include "clientsocket.h"
#include "socketexception.h"


ClientSocket::ClientSocket ( int host, int port )
{
  if ( ! Socket::create() )
  {
	throw SocketException ( "Could not create client socket." );
  }

  if ( ! Socket::connect ( host, port ) )
  {
	throw SocketException ( "Could not bind to port." );
  }

}

const ClientSocket& ClientSocket::operator << ( const std::string& s ) const
{
  if ( ! Socket::send ( s ) )
  {
	throw SocketException ( "Could not write to socket." );
  }

  return *this;

}


const ClientSocket& ClientSocket::operator >> ( std::string& s ) const
{
  if ( ! Socket::recv ( s ) )
  {
	throw SocketException ( "Could not read from socket." );
  }

  return *this;
}

void ClientSocket::close ()
{
    if( !Socket::close(*this))
    {
	throw SocketException ( "Could not close socket.");
    }
}

int ClientSocket::getFD()
{
    return Socket::getfd();
}
