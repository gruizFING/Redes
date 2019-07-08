PROJECT			= kprox
TEMPLATE 		= app
CONFIG 			= qt warn_on debug

OBJECTS_DIR		= obj
MOC_DIR			= moc


HEADERS 		= connector.h \
			  socket.h \
			  serversocket.h \
			  clientsocket.h \
			  socketexception.h \
			  helper.h

SOURCES 		= connector.cpp \
			  serversocket.cpp \
			  clientsocket.cpp \
			  socket.cpp \
			  main.cpp
