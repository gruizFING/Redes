#include "utils.h"
#include "CB.h"

extern struct in_addr localIP;
extern struct in_addr peerIP;
extern unsigned char localPort;
extern unsigned char peerPort;
extern int socket_desc;
extern Estado estado;

extern int pipes[4][2]; // Se debe inicializar con pipe

extern bool send_window; // Se debe inicializar en false
int window_moved;

// MULTIEXCLUSION BUFFER
extern pthread_mutex_t* buff_mutex;

void* gbn_sender_listener(void* params);

// THREAD QUE ENVIA VENTANA

void* gbn_sender(void* params) 
{
    pthread_t thread_gbn_sender_listener;
    if (pthread_create(&thread_gbn_sender_listener, NULL, gbn_sender_listener, NULL) != 0)
        perror("gbn_sender:: ERROR: falla al crear thread thread_gbn_sender_listener");
    
    pthread_t timer_thread;
    if (pthread_create(&timer_thread, NULL, timeout_thread, NULL) != 0)
        perror("gbn_sender:: ERROR: falla al crear thread timeout_thread");
    
    bool fin = false;

    bool finack = false;    

    // Para enviar ventana
    char holder[MAX_PACKET_SIZE];
    int holder_size = MAX_PACKET_SIZE;
    int i = 0, packet_size;
    struct sockaddr_in daddr;
    to_sockaddr(daddr, peerIP);
    bool fin_envio;
    
    // Parametros Select
    fd_set descriptoresLectura;
    struct timeval time_out;
    
    // Para recibir paquete
    struct sockaddr_in saddr;
    int fromlen = sizeof (saddr);
    char packet[MAX_PACKET_SIZE];

    while (!fin) 
    {
        FD_ZERO(&descriptoresLectura);
        FD_SET(pipes[1][0], &descriptoresLectura);
        
        time_out.tv_sec = TIMEOUT/10;
        time_out.tv_usec = 0;

        if(estado != VACIANDOBUFFER)
        {
                select(pipes[1][0]+1, &descriptoresLectura, NULL, NULL, &time_out);
        }
        
        /*********** CERRANDO CONEXION ******************/
        if(estado == VACIANDOBUFFER)
        {
            // Despierto a los threads
            write(pipes[0][1], "X", 1);
            write(pipes[2][1], "X", 1);

            // Espero que los threads terminen
            pthread_join(timer_thread, NULL);
            pthread_join(thread_gbn_sender_listener, NULL);
            
            // Envio todo lo del buffer
            for(i = 0; i < CB::Inst()->CBcount(); i++)
            {
                packet_size = CB::Inst()->read_i_from_tail(i, holder, holder_size);
                if (sendto(socket_desc, (char *) holder, holder_size, 0, (struct sockaddr *) &daddr, (socklen_t)sizeof (daddr)) < 0)
                        perror("gbn_sender:: Error al enviar paquete.");
            }
            
            CB::Inst()->restart();
            
            // Armo paquete FIN
            holder_size = make_pkt(localPort, peerPort, localIP, peerIP, NULL, 0,FIN_FLAG, 0x00, 0x00, holder);
            
            // ENVIO FIN
            if (sendto(socket_desc, (char *) holder, holder_size, 0, (struct sockaddr *) &daddr, (socklen_t)sizeof (daddr)) < 0)
                perror("gbn_sender:: Error al enviar paquete FIN.");
            
            // ESPERO FIN ACK. SINO LLEGA SALGO POR TIMEOUT
            FD_ZERO(&descriptoresLectura);
            FD_SET(socket_desc, &descriptoresLectura);
            time_out.tv_sec = TIMEOUT;
            time_out.tv_usec = 0;
            while( time_out.tv_sec > 0 &&  time_out.tv_usec > 0 && !finack)
            {
                if (select(socket_desc + 1, &descriptoresLectura, NULL, NULL, &time_out) < 0) 
                    perror("gbn_sender_listener:: Error en select.");
            
                if (FD_ISSET(socket_desc, &descriptoresLectura)) // Paquete recibido
                {
                    packet_size = recvfrom(socket_desc, (char *) &packet, sizeof (packet), 0, (struct sockaddr *) &saddr, (socklen_t *) & fromlen);
                    if (packet_size < 0) 
                    {
                        perror("gbn_sender_listener:: Error al recibir paquete.");
                    } 
                    else 
                    {
                        struct iphdr *ipr = (struct iphdr *) packet;
                        if (ipr->saddr == peerIP.s_addr && ipr->daddr == localIP.s_addr && saddr.sin_addr.s_addr == peerIP.s_addr && ipr->protocol == 0xFF) 
                        {
                            struct pct_header* pkt_pct = (struct pct_header *) (packet + sizeof (struct iphdr));
                            if (pkt_pct->destPort == localPort && pkt_pct->srcPort == peerPort) 
                            {
                                /***************** RECIBI FINACK *********************/
                                if (pkt_pct->flags_pct == ACK_FLAG | FIN_FLAG) 
                                {
                                    finack = true;
                                }
                                /*--------------------- RECIBI FINACK -------------------------*/
                            }
                        }
                    }
                }
            }
            
            fin = true; // FIN TOTAL
            
        } /*---------------- CERRANDO CONEXION --------------------*/
        else if( time_out.tv_sec == 0 &&  time_out.tv_usec == 0)
        {
            P(buff_mutex);
            if (send_window) 
            {
                
                V(buff_mutex);
                fin_envio = false;
                i = 0;
                while(!fin_envio)
                {
                    P(buff_mutex);

                    // Me ajusto al movimiento de la ventana
                    i -= window_moved;
                    window_moved = 0;        
                    if(i < 0) i = 0;
                  
                    packet_size = CB::Inst()->read_i_from_tail(i, holder, holder_size);

                    V(buff_mutex);

                    if((packet_size > 0) && i < GBN_WINDOW) 
                    {
                        // ENVIO PAQUETE
                        if (sendto(socket_desc, (char *) holder, packet_size, 0, (struct sockaddr *) &daddr, (socklen_t)sizeof (daddr)) < 0)
                                perror("gbn_sender:: Error al enviar paquete.");
                    }
                    else
                    {
                        fin_envio = true;
                    }
                    
                    i++;
                }
            }
            else
            {
                V(buff_mutex);
            }
            
        }
    } /********* WHILE PRINCIPAL **********/
    
    pthread_exit(NULL);
}

bool ack_valido(int ack)
{
    int count = CB::Inst()->CBcount() < GBN_WINDOW ? CB::Inst()->CBcount() : GBN_WINDOW;
    int x = 0;
    int base = CB::Inst()->base();
    int res = ( base + x ) % 256;
    while(res != ack && x < count)
    {
        x++;
        res = ( base + x ) % 256;
    }
    if(res != ack) return false;
    else return true;
}

// THREAD QUE RECIBE ACK'S y MUEVE VENTANA

void* gbn_sender_listener(void* params) 
{
    bool fin = false;
    
    // Parametros Select
    struct timeval time_out;
    time_out.tv_sec = 2*TIMEOUT;
    time_out.tv_usec = 0;
    fd_set descriptoresLectura;

    // Para recibir paquete
    struct sockaddr_in saddr;
    int fromlen = sizeof (saddr);
    char packet[MAX_PACKET_SIZE];
    int packet_size;

    // Para enviar paquete
    char holder[MAX_PACKET_SIZE];
    int holder_size = MAX_PACKET_SIZE;
    struct sockaddr_in daddr;
    to_sockaddr(daddr, peerIP);

    // Este thread mueve la ventana
    // Debe avisar a gbn_sender si la mueve;
    window_moved = 0;
    
    while (!fin) 
    {

        FD_ZERO(&descriptoresLectura);
        FD_SET(socket_desc, &descriptoresLectura);
        FD_SET(pipes[2][0], &descriptoresLectura);

        /************ SELECT *************/
        if(estado != VACIANDOBUFFER)
        {
                if (select(socket_desc > pipes[2][0] ? socket_desc+1 : pipes[2][0]+1, &descriptoresLectura, NULL, NULL, &time_out) < 0) 
                        perror("gbn_sender_listener:: Error en select.");
        }
        /**********************************/
        
        if(estado == VACIANDOBUFFER)
        {
            fin = true;
        }
        
        /* Se comprueba si se ha enviado algo */
        else if (FD_ISSET(socket_desc, &descriptoresLectura)) // Paquete recibido
        {

            packet_size = recvfrom(socket_desc, (char *) &packet, sizeof (packet), 0, (struct sockaddr *) &saddr, (socklen_t *) & fromlen);
            if (packet_size < 0) 
            {
                perror("gbn_sender_listener:: Error al recibir paquete.");
            } 
            else 
            {
                struct iphdr *ipr = (struct iphdr *) packet;

                if (ipr->saddr == peerIP.s_addr && ipr->daddr == localIP.s_addr && saddr.sin_addr.s_addr == peerIP.s_addr && ipr->protocol == 0xFF) 
                {
                    struct pct_header* pkt_pct = (struct pct_header *) (packet + sizeof (struct iphdr));
                    if (pkt_pct->destPort == localPort && pkt_pct->srcPort == peerPort) 
                    {
                        
                        // Reinicio el tiempo solo si recibi un paquete del otro extremo
                        time_out.tv_sec = 2*TIMEOUT;
                        time_out.tv_usec = 0;
                        
                        /***************** RECIBI ACK *********************/
                        if (pkt_pct->flags_pct == ACK_FLAG) 
                        {
                            // Tengo que eliminar del buffer hasta el ack obtenido y tengo que envíar aquellos paquetes que entren en la ventana
                            P(buff_mutex);
                             if(ack_valido(pkt_pct->nro_ACK_pct))
                            { 
                            while (CB::Inst()->base() != ((pkt_pct->nro_ACK_pct + 1) % 256))
                            {
                                CB::Inst()->delete_tail();

                                if (CB::Inst()->read_i_from_tail(GBN_WINDOW - 1, holder, holder_size) > 0) // al correr la ventana entra un paquete
                                {
                                        // ENVIO PAQUETE
                                        if (sendto(socket_desc, (char *) holder, holder_size, 0, (struct sockaddr *) &daddr, (socklen_t)sizeof (daddr)) < 0)
                                                perror("gbn_sender:: Error al enviar paquete.");
                                 }
                                // Aviso del movimiento de la ventana;
                                window_moved++;
                            }
                            
                            if (CB::Inst()->base() == CB::Inst()->signumsec())
                            {
                                    send_window = false;
                            }
                            else
                            {
                                 send_window = true;
                            }
                             }  
                            
                            V(buff_mutex);
                         }
                        /***************** ************* *********************/
                    }

                }
                else if(time_out.tv_sec == 0 && time_out.tv_usec == 0)
                {
                    //Pasaron 2*TIMEOUT sin recibir paquetes, se termina la conexion 
                    estado = VACIANDOBUFFER;
                    write(pipes[1][1], "X", 1); // Despierto al otro thread
                    fin = true;
                }
            }
        }  
    } /********* WHILE PRINCIPAL **********/
    pthread_exit(NULL);
}
