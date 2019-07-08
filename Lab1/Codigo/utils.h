#ifndef CONSTCOM_H
#define CONSTCOM_H

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctime>

#include "semaforos.h"


//constantes de letra

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_DATA_PORT 5555
#define ADMIN_PORT 6666
#define MAX_QUEUE 2000
#define MAX_MSG_SIZE 4096

using namespace std;


//estructuras que se usan en varios archivos
typedef struct entrada
{
    char* objeto;
    char* url; //Clave
    char* fecha; //De envio
    time_t fechaExpiracion;
    long int tamanio;
    entrada* sig;
    entrada* ant;
} entrada;

typedef struct cache
{
    entrada* ppio;
    entrada* final;
} cache;

void insertar_elemento_cache(entrada* entrada);

void liberar_memoria_entrada(entrada* entrada);

void borrar_n_elementos_final_cache(int n);

bool esGet(char* str);

bool esPost(char* str);

void procesarCadena(char* cadena, int &argc, char** argv);

bool procesarComando(char* cadena, int &argc, char** argv);

char* parseURL(char* str);

char* parseAddress(char* url);

char* parsePort(char* url);

long int parseTamanio(char* data);

bool noEstaModificada(char* data);

char* parseFecha(char* data);

time_t parseFechaT(char* fecha);

char* parseExpires(char* data);

void updateExpires(entrada* url, char* exp);

char* modificarRequest(char* data, char* fecha);

entrada* copia(entrada* ent);

bool expiro_entrada(entrada* entrada_cache);

void borrar_elemento_cache(entrada* entrada_cache);

void mover_al_ppio(entrada* ent);

entrada* buscar_objeto_cache(char* url);

void save_in_cache(char* data_response, long int tam, char* url);





#endif // CONSTCOM_H
