#include "utils.h"
#include "CB.h"

extern struct in_addr localIP;
extern struct in_addr peerIP;
extern unsigned char localPort;
extern unsigned char peerPort;
extern int socket_desc;
extern Estado estado;

extern int pipes[4][2]; // Se debe inicializar con pipe

// MULTIEXCLUSION BUFFER
extern pthread_mutex_t* buff_mutex;

void* gbn_receiver(void*)
{
  pthread_t thread_timer_sender;
  if (pthread_create(&thread_timer_sender, NULL, timeout_thread, NULL) != 0)
        perror("gbn_receiver:: ERROR: falla al crear thread thread_timer_sender");  
       
  struct sockaddr_in src_addr;
  int fromlen = sizeof(src_addr);
  
  // Para quedarse con el select esperando para leer del socket
  fd_set reader;
  // Para manejar el timeout
  struct timeval timeout;
  timeout.tv_sec = 2*TIMEOUT;
  timeout.tv_usec = 0;
  
  struct iphdr* packet_ip;
  struct pct_header* packet_pct;
  
  unsigned char ultNum = 0; 
  bool mandar = false;
  bool entro;
   
  while (1)
  {   
      FD_ZERO(&reader);
      FD_SET(socket_desc, &reader);
      FD_SET(pipes[3][0], &reader);
      
      if (select(socket_desc > pipes[3][0] ? socket_desc+1 : pipes[3][0]+1, &reader, NULL, NULL, &timeout) < 0) {
          perror("Error en select esperando recibir...");
      }
      else 
      {
          if (FD_ISSET(socket_desc, &reader)) 
          { // Llegaron datos al socket           
            char packet[MAX_PACKET_SIZE];
            if (recvfrom(socket_desc, (char *)packet, sizeof(packet), 0, (struct sockaddr*)&src_addr, (socklen_t *)&fromlen) < 0) 
            {
               perror("packet receive error:"); 
            }
            else
            { 
              // Extraigo el header ip del packet recibido
              packet_ip = (struct iphdr *)packet;
              // Check de las direcciones y el protocolo 0xFF de PCT
              if (packet_ip->saddr == peerIP.s_addr && packet_ip->daddr == localIP.s_addr && src_addr.sin_addr.s_addr == peerIP.s_addr && packet_ip->protocol == 0xFF) 
              {
                // Extraigo el header pct del packet recibido
                packet_pct = (struct pct_header *)(packet + sizeof(struct iphdr));
                // Check del puerto
                if (packet_pct->destPort == localPort && packet_pct->srcPort == peerPort)
                {
                    //Recibi algo del otro extremo, reinicio timeout
                    timeout.tv_sec = 2*TIMEOUT;
                    timeout.tv_usec = 0;
                    
                    // Ahora checkear si el segmento es de DATA o de FIN con las flags_pct
                    if (packet_pct->flags_pct == DATA_FLAG)
                    {
                        unsigned char num = CB::Inst()->numsecesperado();
                        printf("Recibiendo DATA con secnum: %d Esperado: %d\n", packet_pct->nro_SEC_pct, num);                                               
                        entro = false;
						// SecNum ok?
                        if (packet_pct->nro_SEC_pct == num)
                        {
                            // Se extrae la data del paquete y se guarda en el buffer de la aplicacion
                            int data_size = ntohs(packet_ip->tot_len) - sizeof(struct iphdr) - sizeof(struct pct_header);
                            printf("Recibido: %d bytes\n", data_size);
                            char* data = (char*) packet_pct + sizeof(struct pct_header); 
                            P(buff_mutex);
                            entro = CB::Inst()->push_data(data,data_size) > 0;
                            V(buff_mutex);
                            if (!entro)
                                printf("No hay espacio en el buffer\n");
                        }
                        
                        // Solo aumenta el secnum si esta ok y si entro, lo hace internamente el buffer
                        unsigned char secNumAck;
                        if (entro)
                        {
                            ultNum = num;
                            secNumAck = num;
                            mandar = true;
                        }
                        else
                        {
                            secNumAck = ultNum;
                        }    
                            
                        if (mandar)
                        {
                            char ACK_Packet[MAX_PACKET_SIZE];
                            int packet_size = make_pkt(localPort, peerPort, localIP, peerIP, NULL, 0,
                                                       ACK_FLAG, 0, secNumAck, ACK_Packet);
                                  
                            printf("Enviando ACK packet con num: %d\n", secNumAck);
                            if (sendto(socket_desc, ACK_Packet, packet_size, 0, (struct sockaddr *)&src_addr, (socklen_t)sizeof(src_addr)) < 0)
                                perror("packet send error:");
                        }
                    }
                    else if (packet_pct->flags_pct == FIN_FLAG)
                    {
                        printf("Recibiendo FIN\n");
                        // Mandar FIN,ACK y terminar la conexion
                        char FINACK_Packet[MAX_PACKET_SIZE];
                        int packet_size = make_pkt(localPort, peerPort, localIP, peerIP, NULL, 0,
                                                 FIN_FLAG | ACK_FLAG, 0, packet_pct->nro_SEC_pct, FINACK_Packet);
                        printf("Enviando FIN,ACK packet\n");
                        if (sendto(socket_desc, FINACK_Packet, packet_size, 0, (struct sockaddr *)&src_addr, (socklen_t)sizeof(src_addr)) < 0)
                              perror("packet send error:");
                        
                        estado = NOCONECTADO;
                        break; //Salgo del while
                    }                
                }
              }              
            }            
          }
          else if (timeout.tv_sec == 0 && timeout.tv_usec == 0) //Se paso el TIMEOUT sin recibir
          {
              printf("Nada recibido en %d segundos --> 2 TIMEOUT\n", 2*TIMEOUT);
              printf("Conexion perdida...\n");
              estado = NOCONECTADO;              
              break;
          }
        }     
  }
  
  // Despierto al thread que envia
  write(pipes[0][1], "X", 1);
  
  //Espero al hilo que envia
  pthread_join(thread_timer_sender, NULL);
  
  pthread_exit(NULL);       
}
      
