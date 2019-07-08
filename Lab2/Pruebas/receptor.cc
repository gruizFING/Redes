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

#define MAX_LINE_SIZE	1024

int main( int argc, const char* argv[] )
{
    int IdPCT,result,i;
    int cantInt,puerto, puertoOrig,puertoDest, tam_archivo;
    char puertoO,puertoD;
    
    struct in_addr direccion;


    if (argc != 3){
        printf( "Error, ejecutar %s <puerto_orig> <direccionIP_local>\n",argv[0]);
        exit(-1);
    }

    // cargo variables para conexion
    puertoOrig=(atoi( argv[1] ) % 256);
    puertoO = (char) puertoOrig;
    unsigned char buf[MAX_LINE_SIZE];
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

    // Espero llegada de datos
    result = 0;
    while (result >= 0) {
        result = leerPCT(&buf[0], MAX_LINE_SIZE);
	buf[result] = NULL;
	if ( result > 0 ){
             printf("Recibidos (%d caracteres): >%s<\n",result,buf);
	}
        sleep(1);
   }
    sleep(10);

    // cierro conexion
    cerrarPCT();
    printf("Cerrando sesion PCT\n");
    sleep(5);
    printf("Recepcion Finalizada\n");

    
}
