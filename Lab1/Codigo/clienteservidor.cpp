#include "clienteservidor.h"


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

char* errorTamanio = "HTTP/1.1 403 Forbidden\r\nDate: Sun, 16 Sep 2012 02:46:30 GMT\r\nVary: Accept-Encoding\r\n"
                     "Content-Length: 313\r\nConnection: close\r\nContent-Type: text/html; charset=iso-8859-1\r\n\r\n"
                     "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                     "<html><head>\r\n"
                     "<title>403 Forbidden</title>\r\n"
                     "</head><body>\r\n"
                     "<h1>Forbidden</h1>\r\n"
                     "<p>El recurso es muy grande para su transferencia, denegado por el administrador<br />\r\n"
                     "</p>\r\n"
                     "<hr>\r\n"
                     "</body></html>\r\n";


typedef struct {
    int socket_to_cliente;
    char data[MAX_MSG_SIZE];
} thr_param;

void* data_cliente(void* params) 
{

    thr_param* parametros = (thr_param*) params;
    int socket_to_client = parametros->socket_to_cliente;
    char* data = new char[MAX_MSG_SIZE];
    memcpy(data, parametros->data, MAX_MSG_SIZE);


    bool get = esGet(data);
    bool post = esPost(data);
    char* data_response = NULL;
    bool enCache = false;
    int data_size = 0;
    if (get || post) 
    {
        char* url = parseURL(data);
        entrada* entrada_cache = NULL;
        bool denegado = false;
        if (get) 
        {
            P(cache_mutex);
            entrada_cache = buscar_objeto_cache(url); //Devuelve una copia de la entrada
            V(cache_mutex);
            if (entrada_cache != NULL) 
            {
                enCache = true;
                data_size = entrada_cache->tamanio;
                P(max_object_size_mutex);
                if (data_size > max_object_size)
                    denegado = true;
                V(max_object_size_mutex);
            }
        } 
        else //Post
        {
            int tamanioObjetoPost = parseTamanio(data);
            P(max_object_size_mutex);
            if (tamanioObjetoPost > max_object_size)
                denegado = true;
            V(max_object_size_mutex);
        }


        if (!denegado) 
        {
            //Conectarse al servidor externo, mandarle el request y esperar respuesta
            //Socket para conectarse con el servidor externo
            printf("[%d] Creando socket para el servidor externo...\n", socket_to_client);
            int socket_to_server = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_to_server == -1) 
            {
                perror("Error al crear socket");
                pthread_exit(NULL);
            }
            printf("[%d] Socket para servidor creado: [%d]\n", socket_to_client, socket_to_server);

            //Primero obterner la direccion del servidor
            printf("[%d] [%d] Obteniendo la direccion del servidor...\n", socket_to_client, socket_to_server);
            char* addr = parseAddress(url);
            char* port = parsePort(url);
            struct addrinfo hints, *res;
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            if (getaddrinfo(addr, port, &hints, &res) != 0) 
            {
                perror("Error al obtener la direccion del servidor solicitado por el cliente");
                send(socket_to_client,
                        "HTTP/1.0 503 Service Unavailable\r\nContent-Type: text/html\r\nContent-Length: 17\r\n\r\nURL no encontrada`",
                        strlen("HTTP/1.0 503 Service Unavailable\r\nContent-Type: text/html\r\nContent-Length: 17\r\n\r\nURL no encontrada"),
                        0);
                close(socket_to_server);
                delete [] data;
                delete [] url;
                delete [] addr;
                delete [] port;
                pthread_exit(NULL);
            }

            //primitiva CONNECT
            printf("[%d] [%d] Intentando conectarse con el servidor...\n", socket_to_client, socket_to_server);
            if (connect(socket_to_server, res->ai_addr, res->ai_addrlen) != 0) 
            {
                perror("Error al conectarse al servidor externo");

                close(socket_to_server);
                delete [] data;
                delete [] url;
                delete [] addr;
                delete [] port;
                pthread_exit(NULL);
            }
            printf("[%d] [%d] Conexion realizada con exito\n", socket_to_client, socket_to_server);

            //Modificar request para el server
            char* data_request;
            if (enCache) //Se encuentra en cache
                data_request = modificarRequest(data,entrada_cache->fecha);
            else
                data_request = modificarRequest(data, NULL);

            //primitiva SEND
            int msg_size = strlen(data_request);
            int sent_msg_size = send(socket_to_server, data_request, msg_size, 0);
            if (sent_msg_size == -1) 
            {
                perror("Error al enviar datos al servidor");

                close(socket_to_server);
                pthread_exit(NULL);
            }
            
            //printf("[%d] [%d] Enviado al servidor (%d bytes)\n%s\n", socket_to_client, socket_to_server, sent_msg_size,data);

            //printf("[%d] [%d] Enviado al servidor (%d bytes)\n%s\n", socket_to_client, socket_to_server, sent_msg_size,data_request);

            //Esperar respuesta
            char* data_servidor = new char[MAX_MSG_SIZE];
            printf("[%d] [%d] Esperando respuesta del servidor...\n", socket_to_client, socket_to_server);
            int recibido_servidor = recv(socket_to_server, data_servidor, MAX_MSG_SIZE, 0);

            if (recibido_servidor > 0) 
            {
                //printf("[%d] [%d] Recibido del servidor (%d bytes)\n%s\n", socket_to_client, socket_to_server, recibido_servidor,data_servidor);
                
                bool noModificada = false;
                if (enCache)
                {
                     noModificada = noEstaModificada(data_servidor);
                     if (noModificada)
                     {
                         P(objetos_cacheados_entregados_mutex);
                         objetos_cacheados_entregados++;
                         V(objetos_cacheados_entregados_mutex);
                         P(cache_mutex);
                         updateExpires(entrada_cache->sig, parseExpires(data_servidor));
                         V(cache_mutex);
                         data_response = entrada_cache->objeto;
                         data_size = entrada_cache->tamanio;
                         printf("[%d] Respuesta en cache (%d bytes)\n", socket_to_client, data_size);
                     }
                     else
                     {
                         P(cache_mutex);
                         borrar_elemento_cache(entrada_cache->sig); //En sig esta la entrada original en cache
                         object_count--;
                         V(cache_mutex);
                         enCache = false;
                     }
                }
      
                if (!enCache)
                {
                    int tamanio = parseTamanio(data_servidor);
                    int tam = tamanio;
                    if (tamanio != -1) 
                    {
                        P(max_object_size_mutex);
                        if (tamanio > max_object_size)
                            denegado = true;
                        V(max_object_size_mutex);
                    }

                    if (!denegado) 
                    {
                        char* buffaux = new char[0];
                        int buffaux_size = recibido_servidor;
                        data_response = new char[0];
                        int aux;

                        while (buffaux_size > 0) 
                        {
                            delete [] buffaux;
                            buffaux = new char[data_size];
                            memcpy(buffaux, data_response, data_size);
                            delete [] data_response;
                            data_response = new char[data_size + buffaux_size];
                            aux = data_size;
                            data_size += buffaux_size;

                            P(max_object_size_mutex);
                            if (data_size > max_object_size) 
                            {
                                V(max_object_size_mutex);
                                denegado = true;
                                break;
                            } else
                            {
                                V(max_object_size_mutex);
                            }

                            memcpy(data_response, buffaux, aux);
                            memcpy(data_response + aux, data_servidor, buffaux_size);
                            delete [] data_servidor;
                            data_servidor = new char[MAX_MSG_SIZE];
                            buffaux_size = recv(socket_to_server, data_servidor, MAX_MSG_SIZE, 0);
                        }

                        printf("[%d] [%d] Recibido del servidor (%d bytes)\n", socket_to_client, socket_to_server, data_size);

                        if (!denegado) 
                        {
                            P(max_cached_object_size_mutex);
                            if (data_size <= max_cached_object_size) 
                            {
                                V(max_cached_object_size_mutex);
                                P(cache_mutex);
                                if (object_count == max_object_count) //Borro el ultimo
                                {
                                    borrar_n_elementos_final_cache(1);
                                    save_in_cache(data_response, data_size, url);
                                }
                                else
                                {
                                    save_in_cache(data_response, data_size, url);
                                    object_count++;
                                }
                                V(cache_mutex);
                                
                                P(memoria_utilizada_mutex);
                                memoria_utilizada += data_size;
                                V(memoria_utilizada_mutex);
                            }
                            else
                            {
                                V(max_cached_object_size_mutex);
                            }
                        }
                    }
                }
            }
            else //Error
            {
                perror("Error al recibir respuesta del servidor");
                close(socket_to_server);
                pthread_exit(NULL);
            }

            freeaddrinfo(res);
            close(socket_to_server);
            delete [] data_servidor;
            delete [] addr;
            delete [] port;
            
        }

        if (denegado) 
        {
            // Se devuelve el mensaje de error al cliente indicando que el  tamanio excede el permitido por el admin
            if (data_response == NULL)
            {
                data_response = new char[MAX_MSG_SIZE];
            }
            else 
            {
                delete [] data_response;
                data_response = new char[MAX_MSG_SIZE];
            }
            strcpy(data_response, "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\nContent-Length: 78\r\n\r\nEl recurso es muy grande para su transferencia, denegado por el administrador.");
            data_size = strlen("HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\nContent-Length: 78\r\n\r\nEl recurso es muy grande para su transferencia, denegado por el administrador.");
            //strcpy(data_response,errorTamanio);
            //data_size = strlen(errorTamanio);
        }

    } 
    else //Metodo no permitido
    {
        data_response = new char[MAX_MSG_SIZE];
        strcpy(data_response, "HTTP/1.0 405 Method Not Allowed\r\nContent-Type: text/html\r\nContent-Length: 19\r\n\r\nMetodo no permitido");
        data_size = strlen("HTTP/1.0 405 Method Not Allowed\r\nContent-Type: text/html\r\nContent-Length: 19\r\n\r\nMetodo no permitido");
    }

    //Mandar data_response al browser
    printf("[%d] Enviando resultado al cliente... (%d bytes)\n", socket_to_client, data_size);
    int enviado = send(socket_to_client, data_response, data_size, 0);
    if (enviado < 0) 
    {
        perror("Error al enviar resultado al cliente");
        pthread_exit(NULL);
    }
    printf("[%d] Enviado al cliente %d bytes\n", socket_to_client, enviado);

    P(requests_atendidos_mutex);
    requests_atendidos++;
    V(requests_atendidos_mutex);

    delete [] data_response;
    delete [] data;
    printf("[%d] Terminando thread \n", socket_to_client);
    pthread_exit(NULL);

}

int continuarCliente(int socketCliente, int *numeroClientes) 
{
    thr_param* parametros = new thr_param();
    parametros->socket_to_cliente = socketCliente;

    int data_largo = MAX_MSG_SIZE;

    //primitiva RECEIVE
    int received_data_size = recv(socketCliente, parametros->data, data_largo, 0);

    if (received_data_size <= 0) 
    {
        /* El cliente cerro la conexión */
        close(socketCliente);
        *numeroClientes--;
        return received_data_size;
    }

    pthread_t thread_data;
    if (pthread_create(&thread_data, NULL, data_cliente, (void*) parametros) != 0) 
    {
        perror("ERROR: falla al crear thread para atender al cliente");
        exit(0);
    }
}

/*
 * Crea un nuevo socket cliente.
 * Se le pasa el socket servidor y el array de clientes, con el número de
 * clientes ya conectados.
 */
void nuevoCliente(int servidor, int *clientes, int *nClientes) 
{
    if ((*nClientes) < 10) {
        //primitiva ACCEPT
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        clientes[*nClientes] = accept(
                servidor,
                (struct sockaddr *) &client_addr, &client_addr_size
                );
        if (clientes[*nClientes] == -1) 
        {
            perror("Error en accept");
            close(servidor);
            exit(-1);
        }

        /* Acepta la conexión con el cliente, guardándola en el array */
        (*nClientes)++;


        if (continuarCliente(clientes[*nClientes], nClientes) <= 0) 
        {
            clientes[*nClientes] = -1;
        }
    }
}

/*
 * Función que devuelve el valor máximo en la tabla.
 * Supone que los valores válidos de la tabla son positivos y mayores que 0.
 * Devuelve 0 si n es 0 o la tabla es NULL */
int dameMaximo(int *tabla, int n) 
{
    int i;
    int max;

    if ((tabla == NULL) || (n < 1))
        return 0;

    max = tabla[0];
    for (i = 0; i < n; i++)
        if (tabla[i] > max)
            max = tabla[i];

    return max;
}

/*
 * Busca en array todas las posiciones con -1 y las elimina, copiando encima
 * las posiciones siguientes.
 * Ejemplo, si la entrada es (3, -1, 2, -1, 4) con *n=5
 * a la salida tendremos (3, 2, 4) con *n=3
 */
void compactaClaves(int *tabla, int *n) 
{
    int i, j;

    if ((tabla == NULL) || ((*n) == 0))
        return;

    j = 0;
    for (i = 0; i < (*n); i++) {
        if (tabla[i] != -1) {
            tabla[j] = tabla[i];
            j++;
        }
    }

    *n = j;
}

void* clienteservidor(void* data_socket) 
{

    int socketServidor = *(int*) data_socket;
    int socketCliente[10]; /* Descriptores de sockets con clientes */
    int numeroClientes = 0; /* Número clientes conectados */
    fd_set descriptoresLectura; /* Descriptores de interes para select() */
    int maximo; /* Número de descriptor más grande */
    int i; /* Para bubles */
    int *res;
    while (1) 
    {
        /* Cuando un cliente cierre la conexión, se pondrá un -1 en su descriptor
         * de socket dentro del array socketCliente. La función compactaClaves()
         * eliminará dichos -1 de la tabla, haciéndola más pequeña.
         * 
         * Se eliminan todos los clientes que hayan cerrado la conexión */
        compactaClaves(socketCliente, &numeroClientes);

        /* Se inicializa descriptoresLectura */
        FD_ZERO(&descriptoresLectura);

        /* Se añade para select() el socket servidor */
        FD_SET(socketServidor, &descriptoresLectura);

        /* Se añaden para select() los sockets con los clientes ya conectados */
        for (i = 0; i < numeroClientes; i++)
            FD_SET(socketCliente[i], &descriptoresLectura);

        /* Se el valor del descriptor más grande. Si no hay ningún cliente,
         * devolverá 0 */
        maximo = dameMaximo(socketCliente, numeroClientes);

        if (maximo < socketServidor)
            maximo = socketServidor;

        /* Espera indefinida hasta que alguno de los descriptores tenga algo
         * que decir: un nuevo cliente o un cliente ya conectado que envía un
         * mensaje */
        select(maximo + 1, &descriptoresLectura, NULL, NULL, NULL);

        /* Se comprueba si algún cliente ya conectado ha enviado algo */
        for (i = 0; i < numeroClientes; i++) 
        {
            if (FD_ISSET(socketCliente[i], &descriptoresLectura)) 
            {
                if (continuarCliente(socketCliente[i], &numeroClientes) <= 0) {
                    socketCliente[i] = -1;
                }
            }
        }
        /* Se comprueba si algún cliente nuevo desea conectarse y se le
         * admite */
        if (FD_ISSET(socketServidor, &descriptoresLectura))
            nuevoCliente(socketServidor, socketCliente, &numeroClientes);
    }

}
