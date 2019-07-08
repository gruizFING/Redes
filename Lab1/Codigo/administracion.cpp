#include "administracion.h"

extern unsigned int memoria_utilizada;
extern unsigned int object_count;
extern unsigned int requests_atendidos;
extern unsigned int objetos_cacheados_entregados;
extern unsigned int max_object_size;
extern unsigned int max_cached_object_size;
extern unsigned int max_object_count;

extern cache* Cache;

extern pthread_mutex_t* memoria_utilizada_mutex;
extern pthread_mutex_t* objetos_cacheados_entregados_mutex;
extern pthread_mutex_t* requests_atendidos_mutex;
extern pthread_mutex_t* max_object_size_mutex;
extern pthread_mutex_t* max_cached_object_size_mutex;
extern pthread_mutex_t* cache_mutex;


void enviar_cadena(int &socket_to_client, const char* cadena)
{
    send(socket_to_client, cadena, strlen(cadena), 0);
}

void show_run(int &socket_to_client)
{
    enviar_cadena(socket_to_client, "Valores actuales:\n");
    int int_aux;
    P(memoria_utilizada_mutex);
    int_aux = memoria_utilizada;
    V(memoria_utilizada_mutex);
    char buffer[256];
    sprintf(buffer, "Memoria total utilizada: %d\n",int_aux);
    enviar_cadena(socket_to_client, buffer);
    
    P(cache_mutex);
    int_aux = object_count;
    V(cache_mutex);
    sprintf(buffer, "Cantidad de objetos: %d\n",int_aux);
    enviar_cadena(socket_to_client, buffer);
    
    P(requests_atendidos_mutex);
    int_aux = requests_atendidos;
    V(requests_atendidos_mutex);
    sprintf(buffer, "Cantidad de requests atendidos: %d\n",int_aux);
    enviar_cadena(socket_to_client, buffer);
    
    P(objetos_cacheados_entregados_mutex);
    int_aux = objetos_cacheados_entregados;
    V(objetos_cacheados_entregados_mutex);
    sprintf(buffer, "Cantidad de objetos cacheados entregados: %d\n",int_aux);
    enviar_cadena(socket_to_client, buffer);
    enviar_cadena(socket_to_client, "---\n");
}

void purge(int &socket_to_client)
{
    int aux;
    P(cache_mutex);
    borrar_n_elementos_final_cache(object_count);
    aux = object_count;
    object_count = 0;
    V(cache_mutex);
    enviar_cadena(socket_to_client, "Cache limpiada.\n");
    char buffer[256];
    sprintf(buffer, "Cantidad de objetos cacheados eliminados: %d.\n",aux);
    enviar_cadena(socket_to_client, buffer);
}

void set_max_object_size(int &socket_to_client, int size)
{
    P(max_object_size_mutex);
    max_object_size = size*1024;
    V(max_object_size_mutex);
    char buffer[256];
    sprintf(buffer, "Tamanio maximo de objeto transferible ajustado a: %d kb.\n",size);
    enviar_cadena(socket_to_client, buffer);
}

void set_max_cached_object_size(int &socket_to_client, int size)
{
    P(max_cached_object_size_mutex);
    max_cached_object_size = size*1024;
    V(max_cached_object_size_mutex);
    char buffer[256];
    sprintf(buffer, "Tamanio maximo de objeto cacheable ajustado a: %d kb.\n",size);
    enviar_cadena(socket_to_client, buffer);
}

void set_max_object_count(int &socket_to_client, int size)
{
    P(cache_mutex);
    if(object_count > size)
    {
       borrar_n_elementos_final_cache(object_count - size);
       object_count = max_object_count = size;
    }
    else
    {
        max_object_count = size;
    }
    V(cache_mutex);
    char buffer[256];
    sprintf(buffer, "Cantidad maxima de objetos ajustada a: %d.\n",size);
    enviar_cadena(socket_to_client, buffer);
}

bool string_to_int(char* arg, int &param)
{
    try
    {
        param = atoi(arg);
        if(param >= 0)
            return true;
        else
            return false;
    }
    catch(exception e)
    {
        return false;
    }
}

void* adminconsola(void* socket_to_client) 
{
    bool quit = false, comando_invalido;
    int argc;
    int param;
    char* argv[3];
    char data[MAX_MSG_SIZE];
    int socket_to_cliente = *(int*)socket_to_client;
    printf("[%d] Un administrador inicio conexion.\n", socket_to_cliente);
    while (!quit){
        
        comando_invalido = false;
        //primitiva RECEIVE
        int received_data_size = recv(socket_to_cliente, data, MAX_MSG_SIZE, 0);
        if(received_data_size <= 0)
        {
            quit = true;
            printf("[%d] El administrador cerro la conexion.\n", socket_to_cliente);
        }
        else
        {
            if(!procesarComando(data,argc,argv))
            {
                comando_invalido = true;
            }
            else if((argc == 2) && (!strcmp(argv[0],"show")) && (!strcmp(argv[1],"run")))
            {
                show_run(socket_to_cliente);                
            }
            else if((argc == 1) && (!strcmp(argv[0],"purge")))
            {
                purge(socket_to_cliente);
            }
            else if((argc == 2 || argc == 3) && !strcmp(argv[0],"set"))
            {
                if(!strcmp(argv[1],"max_object_size"))
                {
                    // set max_object_size XXX(KB)
                    if(argc == 3)
                    {
                        if(!string_to_int(argv[2],param))
                        {
                            enviar_cadena(socket_to_cliente, "Comando invalido.\n");
                            enviar_cadena(socket_to_cliente, "Debe ingresar un numero entero mayor a 0.\n");
                        }
                        else
                            set_max_object_size(socket_to_cliente, param);
                    }
                    else if(argc == 2)
                        set_max_object_size(socket_to_cliente, 10240);
                }
                else if(!strcmp(argv[1],"max_cached_object_size"))
                {
                    // set max_cached_object_size YYY(KB)
                    if(argc == 3)
                    {
                        if(!string_to_int(argv[2],param))
                        {
                            enviar_cadena(socket_to_cliente, "Comando invalido.\n");
                            enviar_cadena(socket_to_cliente, "Debe ingresar un numero entero mayor a 0.\n");
                        }
                        else
                            set_max_cached_object_size(socket_to_cliente, param);
                    }
                    else if(argc == 2)
                        set_max_cached_object_size(socket_to_cliente, 100);
                }
                else if(!strcmp(argv[1],"max_object_count"))
                {
                    if(argc == 3)
                    {
                        if(!string_to_int(argv[2],param))
                        {
                            enviar_cadena(socket_to_cliente, "Comando invalido.\n");
                            enviar_cadena(socket_to_cliente, "Debe ingresar un numero entero mayor a 0.\n");
                        }
                        else
                            set_max_object_count(socket_to_cliente, atoi(argv[2]));
                    }
                    else if(argc == 2)
                        set_max_object_count(socket_to_cliente, 200);
                }
                else
                    comando_invalido = true;
            }
            else if((argc == 1) && !strcmp(argv[0],"quit"))
            {
                enviar_cadena(socket_to_cliente, "Fin de sesion.\n");
                quit = true;
            }
            else
            {
                comando_invalido = true;
            }
                
            
            if(comando_invalido)
            {
                enviar_cadena(socket_to_cliente, "Comando invalido.\n");
            }
            
            
        }
    }
    
    
    close(socket_to_cliente);

}

void* administracion(void* admin_socket) {


    while (1) {
        //primitiva ACCEPT
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        int socket_to_client = accept(
                *(int*) admin_socket,
                (struct sockaddr *) &client_addr, &client_addr_size
                );

        pthread_t thread_admin_consola;
        if (pthread_create(&thread_admin_consola, NULL, adminconsola, (void*) &socket_to_client) != 0) {
            perror("ERROR: falla al crear thread para atender consola de administracon");
            close(socket_to_client);
        }

    }
}


