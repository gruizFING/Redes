#include "pct.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <netdb.h>
#include <stddef.h>
#include <iostream>

using namespace std;

#define MAX_INTENTOS	1000
#define MAX_FILE_SIZE	1024

int main( int argc, const char* argv[] )
{
    int IdPCT,result,i;
    int cantInt,puerto, puertoOrig,puertoDest, tam_archivo;
    char puertoO,puertoD;
    
    struct in_addr direccion;

    FILE* archivo;

    if (argc != 4){
        printf( "Error, ejecutar %s <puerto_orig> <direccionIP_local> <nom_archivo_salida>\n",argv[0]);
        exit(-1);
    }

    // cargo variables para conexion
    puertoOrig=(atoi( argv[1] ) % 256);
    puertoO = (char) puertoOrig;
    unsigned char buf[MAX_FILE_SIZE];
    inet_aton(argv[2], &direccion);
    IdPCT = crearPCT(direccion);
    if ( IdPCT  == -1 ){
        printf( "Error al crear el Socket\n");
        exit(-1);
    }
    printf( "Esperando Conexion\n");

    result = aceptarPCT(puertoO);
    if ( result == -1 ) {
        printf( "Error al aceptar conexion\n");
        exit(-1);
    }
    printf( "Equipo conectado\n");
    sleep(10);


    // Espero llegada de datos
    // voy grabando lo recibido
    cantInt = 0;
    int leido = 0;
    result = 0;
    while (result >= 0) {
        result = leerPCT(&buf[0], MAX_FILE_SIZE);
	//printf("Recibidos %d bytes\n",result);
	if (result > 0 ) {
	    FILE* archigroso = fopen(argv[3], "a+");
	    fwrite (buf , 1 , result, archigroso);
	    fclose(archigroso);
   	}
	if (result > 0 ) {
        	leido = leido + result;
	}
    }
    sleep(10);

    // cierro conexion
    cerrarPCT();
    printf("Cerrando sesion PCT\n");
    sleep(5);
    printf("Recepcion Finalizada\n");
}
