OBJS = main.o administracion.o clienteservidor.o utils.o semaforos.o
GPP = g++
FLAGS = -pthread

all: proxy

proxy: $(OBJS)
	$(GPP) $(FLAGS) $(OBJS) -o proxy

main.o: main.cpp
	$(GPP) $(FLAGS) -c main.cpp

administracion.o: administracion.cpp
	$(GPP) $(FLAGS) -c administracion.cpp

clienteservidor.o: clienteservidor.cpp
	$(GPP) $(FLAGS) -c clienteservidor.cpp

utils.o: utils.cpp
	$(GPP) $(FLAGS) -c utils.cpp

semaforos.o: semaforos.cpp
	$(GPP) $(FLAGS) -c semaforos.cpp	

clean:
	rm -f *.o proxy