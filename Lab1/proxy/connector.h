/***************************************************************************
                          connector.h  -  description
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

#ifndef CONNECTOR_H
#define CONNECTOR_H


#include <iostream>
#include <string>
#include <fstream>

#include "serversocket.h"
#include "clientsocket.h"
#include "socketexception.h"

using namespace std;

/**
  *@author Truls Tveøy
  */

class Connector
{
 public: 
	Connector();
	~Connector();

 private:
	ServerSocket *browserSocket;
	ClientSocket *webSocket;
	void Talk(ServerSocket *, ClientSocket * );

            





};

#endif
