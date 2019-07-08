/* 
 * File:   main.cpp
 * Author: bdi-usr
 *
 * Created on 1 de septiembre de 2012, 01:00 AM
 */

#include "administracion.h"
#include "clienteservidor.h"
#include <signal.h>

using namespace std;


//Memoria compartida por threads
unsigned int memoria_utilizada;
unsigned int requests_atendidos;
unsigned int objetos_cacheados_entregados;
unsigned int max_object_size;
unsigned int max_cached_object_size;
unsigned int max_object_count; // se multi excluye con cache_mutex
unsigned int object_count; // se multi excluye con cache_mutex

int admin_socket, data_socket;

cache* Cache; // se multi excluye con cache_mutex

//Semaforos utilizados por threads
pthread_mutex_t* memoria_utilizada_mutex;
pthread_mutex_t* objetos_cacheados_entregados_mutex;
pthread_mutex_t* requests_atendidos_mutex;
pthread_mutex_t* max_object_size_mutex;
pthread_mutex_t* max_cached_object_size_mutex;
pthread_mutex_t* cache_mutex;

int inicializarSemaforos()
{
    memoria_utilizada_mutex = new pthread_mutex_t();
    memoria_utilizada_mutex= new pthread_mutex_t();
    objetos_cacheados_entregados_mutex= new pthread_mutex_t();
    requests_atendidos_mutex= new pthread_mutex_t();
    max_object_size_mutex= new pthread_mutex_t();
    max_cached_object_size_mutex= new pthread_mutex_t();
    cache_mutex= new pthread_mutex_t();
    pthread_mutex_init(memoria_utilizada_mutex,NULL);
    pthread_mutex_init(objetos_cacheados_entregados_mutex,NULL);
    pthread_mutex_init(requests_atendidos_mutex,NULL);
    pthread_mutex_init(max_object_size_mutex,NULL);
    pthread_mutex_init(max_cached_object_size_mutex,NULL);
    pthread_mutex_init(cache_mutex,NULL);
}

int inicializarMemoriaCompartida() 
{
    memoria_utilizada = 0;
    object_count = 0; // se multi excluye con cache_mutex
    requests_atendidos = 0;
    objetos_cacheados_entregados = 0;
    max_object_size = 10*1024*1024;
    max_cached_object_size = 10*1024;
    max_object_count = 200;
    
    Cache = new cache();
    Cache->final = NULL;
    Cache->ppio = NULL;
}

void cerrar_sockets(int sig_num) // handler para la señal SIGCHLD
{
    close(data_socket);
    close(admin_socket);
}

int main(int argc, char** argv) 
{    
    if (argc > 3) {
        cout << "Uso: proxyserver [IP | IP PUERTO_DATOS]" << endl;
        exit(0);
    }

    const char* ip;
    int puerto_datos;

    switch (argc) {
        case 1:
        {
            ip = DEFAULT_IP;
            puerto_datos = DEFAULT_DATA_PORT;
            break;
        }
        case 2:
        {
            ip = argv[1];
            puerto_datos = DEFAULT_DATA_PORT;
            break;
        }
        case 3:
        {
            ip = argv[1];
            puerto_datos = atoi(argv[2]);
            break;
        }
    }

    cout << "Iniciado servidor...>" << endl;

    if(inicializarSemaforos() == -1)
    {
        perror("<ERROR: falla en la inicializacion de semaforos>");
        exit(1);
    }
    if (inicializarMemoriaCompartida() == -1) {
        perror("ERROR: falla en la inicializacion de memoria compartida");
        exit(1);
    }
    
    
    
    //Se crea el socket para escuchar a los admins
    admin_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (admin_socket < 0) {
        perror("ERROR: falla al crear admin_socket");
        exit(1);
    }
    printf("Creado el socket para escuchar a los administradores: %d\n", admin_socket);

    //Se crea el socket para escuchar a los clientes de datos
    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket < 0) {
        perror("ERROR: falla al crear data_socket");
        close(admin_socket);
        exit(1);
    }
    printf("Creado el socket para escuchar a los clientes: %d\n", data_socket);

    
    //Bind del socket de administradores
    struct sockaddr_in admin_addr;
    socklen_t admin_addr_size = sizeof admin_addr;
    admin_addr.sin_family = AF_INET;
    admin_addr.sin_port = htons(ADMIN_PORT);
    admin_addr.sin_addr.s_addr = inet_addr(ip);
    if (bind(admin_socket, (struct sockaddr*) &admin_addr, admin_addr_size) < 0) {
        perror("ERROR: falla en bind de admin_socket");
        close(admin_socket);
        close(data_socket);
        exit(1);
    }
    //setsockopt(admin_socket, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
    
    
    

    //Bind del socket de datos
    struct sockaddr_in data_addr;
    socklen_t data_addr_size = sizeof data_addr;
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(puerto_datos);
    data_addr.sin_addr.s_addr = inet_addr(ip);
    if (bind(data_socket, (struct sockaddr*) &data_addr, data_addr_size) < 0) {
        perror("ERROR: falla en bind de data_socket");
        close(admin_socket);
        close(data_socket);
        exit(1);
    }
    //setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
     

    // Listen del admin_socket
    if (listen(admin_socket, MAX_QUEUE) < 0) {
        perror("ERROR: falla en listen de admin_socket");
        close(admin_socket);
        close(data_socket);
        exit(1);
    }

    // Listen del data_socket
    if (listen(data_socket, MAX_QUEUE) < 0) {
        perror("ERROR: falla en listen de data_socket");
        close(admin_socket);
        close(data_socket);
        exit(1);
    }

    
    signal(SIGABRT, cerrar_sockets);
    signal(SIGTERM, cerrar_sockets);
    signal(SIGKILL, cerrar_sockets);

    //Creacion de hilo para atender a los administracion
    pthread_t thread_admin;
    if (pthread_create(&thread_admin, NULL, administracion, (void*) &admin_socket) != 0) {
        perror("ERROR: falla al crear thread administracion");
        close(admin_socket);
        close(data_socket);
        exit(1);
    }

    //Creacion de hilo para atender navegadores
    pthread_t thread_nav;

    if (pthread_create(&thread_nav, NULL, clienteservidor, (void*) &data_socket) != 0) {
        perror("ERROR: falla al crear thread para navegadores");
        close(admin_socket);
        close(data_socket);
        exit(1);
    }
    
    printf("Esperando conexiones...\n");


    pthread_join(thread_admin, NULL);
    perror("El thread para administracion ha terminado");
    pthread_join(thread_nav, NULL);
    perror("El thread para navegadores ha terminado");

    //Cerrando los sockets
    close(data_socket);
    close(admin_socket);


    return EXIT_SUCCESS;

}

