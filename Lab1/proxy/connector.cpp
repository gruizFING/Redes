/***************************************************************************
                          connector.cpp  -  description
                             -------------------
    begin                : Mon Nov 11 2002
    copyright            : (C) 2002 by Truls Tveøy
    email                : trulstveoy@home.no
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* Written and copyright 1997 Anonymous Coders and Junkbusters Corporation.
 * Distributed under the GNU General Public License; see the README file.
 * This code comes with NO WARRANTY. http://www.junkbusters.com/ht/en/gpl.html
*/

#include <sstream>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "connector.h"
#include "helper.h"


char *debugfile = NULL;
FILE *debugfp;

const int DEBUG=1;
const int DEBUG2=0;

Connector::Connector()
{
    
    FILE *tdebug = fopen("debug", "a");
    debugfp = tdebug;
    setbuf(debugfp, NULL);
        
    cout << "KProx started...\n";
    
    // Create the socket
    ServerSocket server ( 8080 );
    browserSocket=new ServerSocket();
        

    //do this for the rest of the program run
    for(;;)
    {
	
	if(DEBUG)
	    cout << "\nWaiting for browser to connect...\n";
	server.accept(*browserSocket);
	
	if(DEBUG)
	    cout << "Browser connected...\n";
	    
	Talk(browserSocket, webSocket);

	server.close( *browserSocket );

    }

}
	    

Connector::~Connector()
{

}

void Connector::Talk(ServerSocket *browserSocket,  ClientSocket *webSocket )
{
    string buffer; //temporary container for data
    string data=""; //the complete piece of data
        
    
    //Read the header from the browser
    if(DEBUG)
	cout << "\nReading header from browser...\n";
    for(;;)
    {
	try
	{
	    //stream the content of the socket into buffer
	    *browserSocket >> buffer;
	}
	catch ( SocketException& e )
	{
	    if(DEBUG)
		cout << "Exception was caught:" << e.description() << endl;
	    return;
	}
	
	data += buffer;
	
	if( buffer.find("\r\n", buffer.length() - 2) != string::npos)
	    break;
	
    }
    if(DEBUG2)
    {
	cout << "The browser says this:\n" << data << endl;
	fprintf(debugfp, "The browser says this:\n%s\n", data.c_str());
    }

    //Fetch the host from the header
    string host = GetHost(&data);
    if(DEBUG2)
    {	
	cout << "\nThis is the host: " << host << endl;
	fprintf(debugfp, "%s\n\n\n\n\n", data.c_str());    
    }
    
    //Convert the header
    data = ConvertHeader(&data);
    if(DEBUG)
    {
	cout << "\nConverted header:\n" << data << endl;
	fprintf(debugfp, "\nConverted header:\n%s\n", data.c_str());    
    }

    //Try to connect to the webserver..
    if(DEBUG)
    {
    	cout << "Connecting to web server...\n";
	fprintf(debugfp, "\nConnecting to web server...\n");
    }
    webSocket = new ClientSocket(HostLookup((char *)host.c_str()), 80);
    if(DEBUG)    
    {
	cout << "Connection accepted...\n";
	fprintf(debugfp, "Connection accepted...\n");
    }
    
    if(DEBUG)
    {
	cout << "Writing browser header to webserver...\n";
	fprintf(debugfp, "Writing browser header to webserver...\n");
    }
    *webSocket << data;
    
    //decide who wants to talk
    int n, maxfd;
    fd_set readfds;
    int fd1, fd2;
          
    fd1 = browserSocket->getFD();
    fd2 = webSocket->getFD();
    maxfd = ( fd1  > fd2 ) ? fd1 : fd2;    
       
    int counter=1;
    int isbody=false;
    for(;;)
    {	
	buffer="";
	if(DEBUG)
	{
	    fprintf(debugfp, "\n\nEntering loop no %d\n\n", counter);
	    cout << "\n\nEntering loop no " << counter++ << endl << endl;
	}
	FD_ZERO(&readfds);
	FD_SET(fd1, &readfds);
	FD_SET(fd2, &readfds);
    
	
	n = select(maxfd+1, &readfds, NULL, NULL, NULL);
	if( n < 0 )
	{
	    cout << "Could not select...\n";
	    break;
	}


	if(FD_ISSET(fd1, &readfds ) )
	{
	    if(DEBUG)
	    {
		cout << "\nThe browser wants to talk...\n";
		fprintf( debugfp, "\nThe browser wants to talk...\n");
	    }
	    try
	    {
		*browserSocket >> buffer;
		
	    }
	    catch ( SocketException& e)
	    {
		if(DEBUG)
		{
		    cout << "Exception was caught:" << e.description() << endl;
		    fprintf(debugfp, "Exception was caught: %s\n", e.description().c_str() );
		}
		break;
	    }	    
	    
	    if(DEBUG)
	    {
		if(DEBUG2)
		    cout << "\nSays this (" << buffer.length() << "):\n" << buffer << endl;
		fprintf( debugfp, "\nSays this (%d):\n%s\n", buffer.length(), buffer.c_str() );
	    }

	    *webSocket << data;
	    
	    if(DEBUG)
	    {
		fprintf(debugfp, "Wrote to web server...\n");
		cout << "Wrote to web server...\n";
	    }

	    continue;
 
	}

	if(FD_ISSET( fd2, &readfds ) )
	{
	    
	    if(DEBUG)
	    {
		cout << "\nThe web server wants to talk...\n";
		fprintf(debugfp, "\nThe web server wants to talk...\n");
	    }
	    
	    try
	    {
		*webSocket >> buffer;
	    }
	    catch ( SocketException& e )
	    {
		if(DEBUG)
		{
		    cout << "Exception was caught:" << e.description() << endl;
		    fprintf(debugfp, "Exception was caught: %s\n", e.description().c_str() );
		}

		break;
	    }
	    
	 
	    
	    if(isbody) //we're dealing with the server's body - send it to the browser
	    {
	    
		if(DEBUG)
		{
		    if(DEBUG2)
			cout << "\nSays this (" << buffer.length() << "):\n" << buffer << endl;
		    fprintf(debugfp, "\nSays this (%d):\n%s\n", buffer.length(), buffer.c_str());    
		}
		
		*browserSocket << buffer;
				
		if(DEBUG)
		{
		    cout << "Wrote to browser...\n";
		    fprintf(debugfp, "Wrote to browser...\n");
		}
	    }
	    else //find the header int the message
	    {
		string header;
		if( ParseHeader(&buffer, &header) )
		{
		    isbody=true;
		    //found the header, write to browser
		    if(DEBUG)
		    {
			if(DEBUG2)
			    cout << "\nSays this (" << buffer.length() << "):\n" << buffer << endl;
			fprintf(debugfp, "\nSays this (%d):\n%s\n", buffer.length(), buffer.c_str());    
		    }		    
		    
		    *browserSocket << header;
		    
		    if(buffer.length() != 0)
		    {
			*browserSocket << buffer;
		    }

		    if(DEBUG)
		    {
			cout << "Wrote to browser...\n";
			fprintf(debugfp, "Wrote to browser...\n");
		    }
		    continue;
		}
		
		//Should never get here...
		return;
	    }
	}
    }
    webSocket->close();
}
