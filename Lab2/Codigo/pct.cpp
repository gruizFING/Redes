/* 
 * File:   main.cpp
 * Author: bdi-usr
 *
 * Created on 5 de octubre de 2012, 01:46 AM
 */
#include "pct.h"
#include "utils.h"
#include "CB.h"
#include "gbn_receiver.h"
#include "gbn_sender.h"
#include "iostream"

using namespace std;
//Variables globales
int socket_desc;
Estado estado;
bool send_window;

struct in_addr localIP;
struct in_addr peerIP;
unsigned char localPort;
unsigned char peerPort;

pthread_t sender_thread;
int pipes[4][2];  // Pipes para despertar a los threads

pthread_mutex_t* buff_mutex;



int crearPCT(struct in_addr localIPaddr)
{
    //Creacion del raw socket
    socket_desc = socket(AF_INET, SOCK_RAW, 0xFF);
    if (socket_desc < 0) 
    {
        perror("Error al crear socket");
        return -1;
    }
    localIP = localIPaddr;
    estado = NOCONECTADO;
    send_window = false;

    inicializar_semaforos();
    CB::Inst()->restart();
    
    // Inicializo Pipes
    pipe(pipes[0]); pipe(pipes[1]); pipe(pipes[2]); pipe(pipes[3]);
    
    return 0;
}

int aceptarPCT(unsigned char localPCTport) {

    struct sockaddr_in saddr;
    to_sockaddr(saddr, localIP);


    struct timeval time_out;
            int con_close = 0;

   while (!con_close) {
        printf("Esperando SYN\n");
        // Esperar SYN        
        int recibiPKT = 0;

        unsigned int rec = 0;
        int ack = 0;
        int seq = 0;

        struct sockaddr_in src_addr;

       
	    recibiPKT = 0;
            //Les reservo memoria a las estructuras que van a ser seteadas en esperarSYN_ACK
            struct pct_header *pkt_pct = (struct pct_header*) malloc(sizeof (struct pct_header));
            struct iphdr *pkt_ip = (struct iphdr*) malloc(sizeof (struct iphdr));


            rec = esperarSYN_ACK(socket_desc, SYN_FLAG, localPCTport, src_addr, localIP, pkt_pct, pkt_ip, recibiPKT);

            if (rec > 0) {
                if (recibiPKT) {
                    //Me Guardo nro de seq en ack
                    ack = pkt_pct->nro_SEC_pct;
                    peerPort = pkt_pct->srcPort;
                    peerIP.s_addr = pkt_ip->saddr;


                    printf("Recibi SYN\n");
                } else
                    printf("Recibi basura\n");

            } else if (rec == 0)
                //Se debe cerra la conecion
                con_close = 1;
            else
                return -1;
        



        if (recibiPKT) {

            seq = ack;
            char packet[MAX_PACKET_SIZE];

            //Creo el paquete SYN ACK
            int syn_size = make_pkt(localPCTport, peerPort, localIP, peerIP, NULL, 0,
                    SYN_FLAG | ACK_FLAG, seq, ack, packet);


            to_sockaddr(src_addr, peerIP);



            //Envio el SYN,ACK
            if (sendto(socket_desc, packet, syn_size, 0, (struct sockaddr *) &src_addr, (socklen_t)sizeof (src_addr)) < 0) {
                perror("Error al enviar SYN,ACK");
                return -1;
            }
            printf("Mande SYN,ACK\n");

            //Se usa select para saber cuando el socket esta listo para recibiPKTr el ack.
            fd_set read_set;

            //Inicializo el file descritpor read_set para que tenga 0 bits.
            FD_ZERO(&read_set);

            //Seteo el bit para el socket descriptor socket_desck en el file descriptor set read_set.
            FD_SET(socket_desc, &read_set);

            time_out.tv_sec = TIMEOUT;
            time_out.tv_usec = 0;

            do {
                // Select, espero hasta que este listo para leer o paso TIME_OUT      
                printf("Esperando ACK...\n");

                //Quiero saber cuando esta listo para leer asi que solo me importa el segundo parametro que es el de lectura, el 3ro y 4to son para chequear scritura y exepciones.
                //Si se pasa el time out devuelvo 0, si hay error -1 y si esta listo para leer retorna  el numero de ready descriptors que estan contenidos en la bit masks.
                if (select(socket_desc + 1, &read_set, NULL, NULL, &time_out) < 0) {
                    perror("Error esperando ACK");
                    return -1;
                }
                recibiPKT = 0;
                con_close = 0;
                // Esperar ACK

                //Retorna 0 si el bit para el socket descriptor socket_desc esta seteado en el file descriptor set fdset.
                if (FD_ISSET(socket_desc, &read_set)) {

                    struct pct_header *pkt_pct = (struct pct_header*) malloc(sizeof (struct pct_header));
                    struct iphdr *pkt_ip = (struct iphdr*) malloc(sizeof (struct iphdr));

                    rec = esperarSYN_ACK(socket_desc, ACK_FLAG, localPCTport, src_addr, localIP, pkt_pct, pkt_ip, recibiPKT);

                    if (rec > 0) {



                        //Chequeo que los puerto y las direcciones IP como tambien el tipo de protocolo que debe ser 0xFF
                        if (pkt_ip->saddr == peerIP.s_addr && src_addr.sin_addr.s_addr == peerIP.s_addr) {




                            //Chequeo que el ack corresponda al paquete que fue enviado, tambien chequeo puertos y flag ACK                          
                            recibiPKT = recibiPKT && (pkt_pct->nro_ACK_pct == ack && pkt_pct->nro_SEC_pct == seq);

                            if (recibiPKT) {
                                printf("Recibi ACK\n");
                            } else
                                printf("Recibi basura\n");
                        }
                    } else if (rec == 0)
                        con_close = 1;
                    else
                        return -1;
                }
                if (recibiPKT) {
                    localPort = localPCTport;

                    estado = ACEPTADO;

                    //Creo el thread para recibir
                    pthread_t receiver_thread;
                    if (pthread_create(&receiver_thread, NULL, gbn_receiver, NULL) < 0) {
                        perror("Error en pthread_create");
                        return -1;
                    }

                    return 0;
                }

            } while ((time_out.tv_sec > 0 || time_out.tv_usec > 0) && !recibiPKT && !con_close); // Vuelvo a esperar ACK

        } // if (recibiPKT)


      }
    return -1;
    
}

int conectarPCT(unsigned char localPCTport, unsigned char peerPCTport, struct in_addr peerIPaddr) 
{


    //Inicializo Estructuras
    struct sockaddr_in saddr;
    struct sockaddr_in daddr;

    int Envio_PKT;
    struct timeval time_out;
    to_sockaddr(saddr, localIP);
    to_sockaddr(daddr, peerIPaddr);
    //Se usa select para saber cuando el socket esta listo para recibiPKTr el ack.
    fd_set read_set;

    localPort = localPCTport;
    peerPort = peerPCTport;
    peerIP = peerIPaddr;
    int seq = rand() % 2;
    int recibiOK = 0;
    char packet[MAX_PACKET_SIZE];

    
   while ( 1) {
    //Creo el paquete SYN
    int syn_size = make_pkt(localPCTport, peerPCTport, localIP, peerIPaddr, NULL, 0, SYN_FLAG, seq, 0x00, packet);
    Envio_PKT = sendto(socket_desc, packet, syn_size, 0, (struct sockaddr *) &daddr, (socklen_t)sizeof (daddr));

    if (Envio_PKT < 0) {
        perror("Error al intentar enviar SYN\n");
        return -1;
    }
    printf("Mande SYN\n");

    time_out.tv_sec = TIMEOUT;
    time_out.tv_usec = 0;


    //Inicializo el file descritpor read_set para que tenga 0 bits.
    FD_ZERO(&read_set);

    //Seteo el bit para el socket descriptor socket_desck en el file descriptor set read_set.
    FD_SET(socket_desc, &read_set);

 do {
    recibiOK = 0;
    int con_close = 0;

 
        printf("Esperando SYN,ACK...\n");

        //Espero hasya el el socket este listo para escribir o se pase el time out
        if (select(socket_desc + 1, &read_set, NULL, NULL, &time_out) < 0) {
            perror("Error esperando SYN,ACK");
            return -1;
        }


        int rec = 0;
        int ack = 0;

        //Retorna 0 si el bit para el socket descriptor socket_desc esta seteado en el file descriptor set fdset.
        if (FD_ISSET(socket_desc, &read_set)) {

            struct sockaddr_in src_addr;

            struct pct_header *pkt_pct = (struct pct_header*) malloc(sizeof (struct pct_header));
            struct iphdr *pkt_ip = (struct iphdr*) malloc(sizeof (struct iphdr));
            rec = esperarSYN_ACK(socket_desc, (SYN_FLAG | ACK_FLAG), localPCTport, src_addr, localIP, pkt_pct, pkt_ip, recibiOK);

            if (rec > 0) {

                //Controlo secuencia direccionamiento
                if (pkt_ip->saddr == peerIP.s_addr && src_addr.sin_addr.s_addr == peerIP.s_addr) {

                    //Controlo secuencia
                    recibiOK = recibiOK && (pkt_pct->nro_ACK_pct == seq);

                    if (recibiOK) {
                        //Guardo ack y seq  
                        ack = pkt_pct->nro_SEC_pct;
                        seq = pkt_pct->nro_ACK_pct;
                        printf("Recibi SYN,ACK\n");
                    } else {
                        printf("Recibi basura\n");

                    }
                } else recibiOK = 0;
            }
            else if (rec == 0)
                con_close = 1;
            else
                return -1;

        }

        if (recibiOK) {
            // Crear paquete ACK
            int ack_size = make_pkt(localPCTport, peerPCTport, localIP, peerIPaddr, NULL, 0, ACK_FLAG, seq, ack, packet);


            // Enviar paquete ACK
            if (sendto(socket_desc, packet, ack_size, 0, (struct sockaddr *) &daddr, (socklen_t)sizeof (daddr)) < 0) {
                perror("Error al enviar ACK");
                return -1;
            }

            printf("Envie ACK\n");

            estado = CONECTADO;

            //Creo el thread para enviar
            if (pthread_create(&sender_thread, NULL, gbn_sender, NULL) < 0) {
                perror("Error en pthread_create");
                return -1;
            }

            return 0;
        }
    } while ((time_out.tv_sec > 0 || time_out.tv_usec > 0) && !recibiOK);
}
    return -1;
}

int escribirPCT(const void *buf, size_t len)
{    
    if (estado != CONECTADO || len <= 0)
        return -1;

    int data_size = ( len <= MAX_PCT_DATA_SIZE ) ? len : MAX_PCT_DATA_SIZE;
    
    // Para enviar paquete
    char packet[MAX_PACKET_SIZE];
    int packet_size = MAX_PACKET_SIZE;
    struct sockaddr_in daddr;
    to_sockaddr(daddr, peerIP);
    
    P(buff_mutex);
    
    // ARMO PAQUETE CON LO QUE QUEPA DE DATOS
    packet_size = make_pkt(localPort, peerPort, localIP, peerIP, (char*)buf, data_size, DATA_FLAG, CB::Inst()->signumsec(), 0x00, packet);
    
    // Lo meto en el buffer
    CB::Inst()->push_packet(packet, packet_size);
    
    if(CB::Inst()->CBcount() <= GBN_WINDOW) // Si el paquete entro en la ventana
    {

        if(CB::Inst()->CBcount() == 1) // Si es el primer paquete de la ventana
        {
            send_window = true; // Inicio el temporizador
        }
        
        V(buff_mutex);
        
        // ENVIO PAQUETE
        if (sendto(socket_desc, (char *) packet, packet_size, 0, (struct sockaddr *) &daddr, (socklen_t)sizeof (daddr)) < 0)
                perror("escribirPCT:: Error al enviar paquete.");
        
    }
    else
    {
        V(buff_mutex);
    }
    
    return data_size;
}

int leerPCT(void *buf, size_t len)
{
    if (estado == NOCONECTADO)
        return -1;
    
    int data_size;
    P(buff_mutex);
    
    // Leo datos del buffer de datos
    data_size = CB::Inst()->pop_data((char*)buf, len);

    V(buff_mutex);
    
    return data_size;
}

int cerrarPCT() 
{
    if (estado == ACEPTADO)
        return -1;
    else if (estado == CONECTADO)
    {
        //Cambiar el estado a vaciando buffer
        estado = VACIANDOBUFFER;
        
        // Despierto al thread gbn_sender
        write(pipes[1][1], "X", 1);
        
        //Espero que termine de vaciar el buffer y termine
        pthread_join(sender_thread, NULL);
        perror("El thread gbn_sender termino/n");

        estado = NOCONECTADO;
        
        //Libero memoria y cierro conexion
        close(socket_desc);
        pthread_mutex_destroy(buff_mutex);
        //CB::Inst()->~CB();
        
        return 0;
    }
    return -1;
}


