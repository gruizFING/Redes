//needed by Tokenize(...) & GetHost(..)
#include <string>
#include <vector>

//needed by HostLookup(...)
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

#include <iostream>

using namespace std;

void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " ")
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

int HostLookup(char *host)
{
  struct sockaddr_in inaddr;
  struct hostent *hostp;

  if ((host == NULL) || (*host == '\0'))
    return(INADDR_ANY);

  memset ((char * ) &inaddr, 0, sizeof inaddr);
  if ((int)(inaddr.sin_addr.s_addr = inet_addr(host)) == -1)
  {
    if ((hostp = gethostbyname(host)) == NULL)
    {
  	errno = EINVAL;
	return(-1);
    }
    if (hostp->h_addrtype != AF_INET)
    {
      errno = EPROTOTYPE;
      return(-1);
    }
    memcpy((char * ) &inaddr.sin_addr, (char * ) hostp->h_addr, sizeof(inaddr.sin_addr));
  }
  return(inaddr.sin_addr.s_addr);
}

string GetHost(string *hdr)
{
  vector <string> tokens;
  
  //split up the header
  Tokenize(*hdr, tokens, "\r\n");
	    
  vector<string>::iterator it;
  //step through the tokenized header
  for (it = tokens.begin(); it != tokens.end(); ++it)
  {
    //if we find the host-tag at the start of a token
    if( it->find( "Host:", 0 ) == 0 )
    {
      //return the hostname minus the host-tag
      return it->substr(6);
    }
  }

  //couldn't find the host, return error (-1)
  return "-1";
}

string ConvertHeader(string *hdr)
{

  string ret="";
  string host;
  vector <string> tokens;
  Tokenize(*hdr, tokens, "\r\n");

  vector<string>::iterator it;
  for( it = tokens.begin(); it != tokens.end(); ++it)
  {

    if( it->find("Host:", 0) == 0 )
    {
      host = it->substr(6);
    }
    
  }
  
  

  for( it = tokens.begin(); it != tokens.end(); ++it)
  {
    if( it->find( "GET", 0) == 0)
    {

      string temp = *it;
      host = "http://" + host;
      int location = temp.find(host,0);
      int length = host.length();
      temp = temp.replace( location, length, "" );
      ret += temp + "\r\n";

    }
    else if( it->find( "Proxy", 0) != 0 )
    {
      ret += *it + "\r\n";
    }
  }
  ret += "\r\n";
  
  return ret;
}

bool ParseHeader(string *buf, string *hdr)
{
  
  string ret = "";
  
  int position = buf->find("\r\n\r\n", 0);
  if( position  == (int)string::npos )
    return false;
  
  //if we got here, we found the end point
  
  //take the header
  string header = buf->substr(0, position+4);
  
  
  vector <string> tokens;
  
  //split up the header
  Tokenize(header, tokens, "\r\n");
	    
  vector<string>::iterator it;
  //step through the tokenized header
  for (it = tokens.begin(); it != tokens.end(); ++it)
  {
    //if we find the host-tag at the start of a token
    if( it->find( "Connection:", 0 ) != 0 )
    {
      ret += *it + "\r\n";
    }
  }
  
  ret+= "\r\n";
  
  *hdr = ret;
  *buf = buf->substr(position+4);
  


  
  return true;
}




    
