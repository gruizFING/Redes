
#include "pct.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>


#define MAX_LINE_SIZE	1024
#define TRUE 1
#define FALSE 0

int main( int argc, const char* argv[] )
{
    int IdPCT,result;
    int puerto, puertoOrig,puertoDest;
    unsigned char puertoO,puertoD;

    
    struct in_addr dirOrig;
    struct in_addr dirDest;


    if (argc != 5){
        printf( "Error, ejecutar %s <puerto_local> <direccionIP_local> <puerto_remoto>  <direccionIP_remoto>\n",argv[0]);
        exit(-1);
    }

    // cargo variables de la conexion
    puerto=atoi( argv[1] );
    puertoOrig=(atoi( argv[1] ) % 256);
    puertoO = (unsigned char) puertoOrig;
    puertoDest=(atoi( argv[3] ) % 256);
    puertoD = (unsigned char) puertoDest;


    // configuro direccion del equipo local
    inet_aton(argv[2], &dirOrig);
    IdPCT = crearPCT(dirOrig);
    if ( IdPCT  == -1 ){
        printf( "Error al crear el Socket\n");
        exit(-1);
    }
    printf( "Conexion creada\n");

    // Configuro direccion remota y conecto
    inet_aton(argv[4], &dirDest);
    result = conectarPCT(puertoO,puertoD,dirDest);
    if ( result == -1 ) {
        printf( "Error al conectar Socket\n");
        exit(-1);
    }
    printf( "Equipo conectado\n");
	
    // comienzo a leer de stdin
    int final = FALSE;
    char string [MAX_LINE_SIZE];
    char ult[3];
    char aux;
    int cant;
    ult[0]=ult[1]=ult[2]='A';
    printf ("Escriba lineas y finalice con <enter>.<enter>: \n");
    while ( ! final ) {
	    cant = 0;
	    aux = fgetc(stdin);
	    while (aux != '\n'){
		string[cant] = (char)aux;
    		ult[0]=ult[1];
    		ult[1]=ult[2];
    		ult[2]=string[cant];
		cant++;
		aux = fgetc(stdin);
	    }
  	    string[cant] = (char)aux;
    	    ult[0]=ult[1];
    	    ult[1]=ult[2];
    	    ult[2]=string[cant];
	    string[cant+1] = NULL;
	    //fgets(string,MAX_LINE_SIZE,stdin);
	    result = escribirPCT(&string[0], cant+1);
	    printf ("Enviando (%d caracteres): >%s<\n",result,string);
	    if (ult[0]== '\n' && ult[1]=='.' && ult[2]=='\n') final = TRUE;
    }
    printf("Transmisión finalizada\n");

    // cierro conexion
    cerrarPCT();
    printf("Cerrando sesion PCT\n");
    sleep(5);
    printf("Envio Finalizado\n");
    
}


