# Modificar las variables emisor y receptor con el .cc correspondiente
emisor=enviaFile
receptor=recibeFile

all: pct.o CB.o utils.o semaforos.o gbn_receiver.o gbn_sender.o $(emisor) $(receptor)

pct.o: pct.cpp gbn_receiver.cpp gbn_sender.cpp
	g++ -c -pthread pct.cpp

gbn_receiver.o: gbn_receiver.cpp utils.cpp CB.o 
	g++ -c -pthread gbn_receiver.cpp 

gbn_sender.o: gbn_sender.cpp utils.cpp CB.o 
	g++ -c -pthread gbn_sender.cpp

CB.o: CB.cpp utils.cpp
	g++ -c -pthread CB.cpp

utils.o: utils.cpp CB.cpp semaforos.cpp
	g++ -c -pthread utils.cpp

semaforos.o: semaforos.cpp
	g++ -c -pthread semaforos.cpp

$(emisor): $(emisor).cc pct.o gbn_receiver.o gbn_sender.o CB.o utils.o semaforos.o
	g++ -o $(emisor) -pthread $(emisor).cc pct.o gbn_receiver.o gbn_sender.o CB.o utils.o semaforos.o

$(receptor): $(receptor).cc pct.o gbn_receiver.o gbn_sender.o CB.o utils.o semaforos.o
	g++ -o $(receptor) -pthread $(receptor).cc pct.o gbn_receiver.o gbn_sender.o CB.o utils.o semaforos.o

clean:
	rm -f *.o
	rm $(emisor)
	rm $(receptor)

