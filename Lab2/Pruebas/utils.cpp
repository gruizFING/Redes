#include "utils.h"


extern struct in_addr localIP;
extern struct in_addr peerIP;
extern unsigned char localPort;
extern unsigned char peerPort;
extern int socket_desc;
extern Estado estado;

extern int pipes[4][2]; // Se debe inicializar con pipe

// MULTIEXCLUSION BUFFER
extern pthread_mutex_t* buff_mutex;

int make_pkt(unsigned char localPCTport, unsigned char peerPCTport, struct in_addr localIP,
	     struct in_addr peerIP, char *payload, int payl_size,
	     unsigned char flags_pct, unsigned char nro_SEC_pct,
	     unsigned char nro_ACK_pct, char *packet) 
{
    // Tamanio del cabezal IP

    size_t ihl = sizeof (struct iphdr);
    // Tamanio del cabezal IP mas el cabezal PCT
    size_t hl = ihl + sizeof (struct pct_header);

    //packet = (char *) malloc(hl + payl_size);

    // Cabezal IP al comienzo del paquete
    struct iphdr *ip = (struct iphdr *) packet;
    // Cabezal PCT inmediatamente despues del cabezal IP
    struct pct_header *pct = (struct pct_header *) (packet + ihl);


    struct sockaddr_in localIPaddr;
    to_sockaddr(localIPaddr, localIP);

    struct sockaddr_in peerIPaddr;
    to_sockaddr(peerIPaddr, peerIP);

    // Seteo cabezal ip
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(hl + payl_size);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = 0xFF; // Protocolo PCT
    ip->check = 0;
    ip->saddr = localIP.s_addr;
    ip->daddr = peerIP.s_addr;


    // Seteo cabezal PCT
    pct->srcPort = localPCTport;
    pct->destPort = peerPCTport;
    pct->flags_pct = flags_pct;
    pct->nro_SEC_pct = nro_SEC_pct;
    pct->nro_ACK_pct = nro_ACK_pct;




    // Copio el payload en el paquete luego de los cabezales
    memcpy(packet + hl, payload, payl_size);


    // Retorno el largo del paquete (cabezal + payload)
    return (hl + payl_size);

}

int esperarSYN_ACK(int socket_desc,unsigned char flag_chk,unsigned char localPCTport,struct sockaddr_in &src_addr,
               struct in_addr  localIP ,struct pct_header *pkt_pct, struct iphdr * pkt_ip,int &recibiPKT){
    

    int fromlen = sizeof (src_addr);
    char buffer [MAX_PACKET_SIZE];
    int rec;

    rec = (recvfrom(socket_desc, (char *) buffer, sizeof (buffer), 0, (struct sockaddr *) &src_addr, (socklen_t *) & fromlen));
    if (rec > 0) {

        //Casteo la primera parte del paquete obtenido
        struct iphdr *pkt_ipAux = (struct iphdr *) buffer;

        //Copia Limpia
        pkt_ip->ihl = pkt_ipAux->ihl;
        pkt_ip->version = pkt_ipAux->version;
        pkt_ip->tos = pkt_ipAux->tos;
        pkt_ip->tot_len = pkt_ipAux->tot_len;
        pkt_ip->frag_off = pkt_ipAux->frag_off;
        pkt_ip->ttl = pkt_ipAux->ttl;
        pkt_ip->protocol = pkt_ipAux->protocol; // Protocolo PCT
        pkt_ip->check = pkt_ipAux->check;
        pkt_ip->saddr = pkt_ipAux->saddr;
        pkt_ip->daddr = pkt_ipAux->daddr;

        //Controlo direccionamiento y protocolo

        if (pkt_ip->daddr == localIP.s_addr && pkt_ip->protocol == 0xFF) {

            //Casteo la segunda parte del paquete recibiPKTdo 

            struct pct_header * pkt_pctAux = (struct pct_header *) ((buffer) + pkt_ip->ihl * 4);


            pkt_pct->srcPort = pkt_pctAux->srcPort;
            pkt_pct->destPort = pkt_pctAux->destPort;
            pkt_pct->flags_pct = pkt_pctAux->flags_pct;
            pkt_pct->nro_SEC_pct = pkt_pctAux->nro_SEC_pct;
            pkt_pct->nro_ACK_pct = pkt_pctAux->nro_ACK_pct;

            //Controlo puertos, secuencia y flag

            recibiPKT = (pkt_pct->destPort == localPCTport && pkt_pct->flags_pct == flag_chk);

            return rec;


        }
    }
}

void to_sockaddr(struct sockaddr_in &addr, struct in_addr &ip) {           
  addr.sin_family = AF_INET;   
  addr.sin_port = 0;  
  addr.sin_addr.s_addr = ip.s_addr;    
  memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
}


void* timeout_thread(void*)
{
    char packet[MAX_PACKET_SIZE];
    int packet_size = make_pkt(localPort, peerPort, localIP, peerIP, NULL, 0, DATA_FLAG | SYN_FLAG, 0, 0, packet);
    struct sockaddr_in peerIPaddr;
    to_sockaddr(peerIPaddr,peerIP);
    
    fd_set desc;
    struct timeval timeout;
   
    while (1)
    {
         FD_ZERO(&desc);
         FD_SET(pipes[0][0], &desc);
         timeout.tv_sec = TIMEOUT;
         timeout.tv_usec = 0;
         
         if (estado == CONECTADO || estado == ACEPTADO)
                select(pipes[0][0]+1, NULL, NULL, NULL, &timeout);
         
         if (estado != CONECTADO && estado != ACEPTADO)
             break;
         else //Envio algo para ver si el otro extremo sigue
         {           
            //printf("Enviando packet\n");
            if (sendto(socket_desc, packet, packet_size, 0, (struct sockaddr *)&peerIPaddr, (socklen_t)sizeof(peerIPaddr)) < 0)
                   perror("packet send error:");
         }
     }
    
     pthread_exit(NULL);
}

int inicializar_semaforos()
{
    buff_mutex = new pthread_mutex_t();
    pthread_mutex_init(buff_mutex,NULL);
}
