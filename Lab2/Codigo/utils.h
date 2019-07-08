/* 
 * File:   utils.h
 * Author: bdi-usr
 *
 * Created on 5 de octubre de 2012, 02:31 AM
 */

#ifndef UTILS_H
#define	UTILS_H

#include <netinet/in.h>
#include <stdio.h> //for printf
#include <string.h> //memset
#include <sys/socket.h>    //for socket ofcourse
#include <stdlib.h> //for exit(0);
#include <errno.h> //For errno - the error number
#include <netinet/tcp.h>   //Provides declarations for tcp header
#include <netinet/ip.h>    //Provides declarations for ip header
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>   /* gethostbyname() necesita esta cabecera */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include "pct.h"
#include "semaforos.h"
#define MAX_PACKET_SIZE (sizeof(struct iphdr) + sizeof(struct pct_header) + MAX_PCT_DATA_SIZE)



enum Estado {NOCREADO, NOCONECTADO, CONECTADO, ACEPTADO, VACIANDOBUFFER};

/*
* Crea un paquete con los parametros dados.
* Retorna el largo del paquete resultado.
*/
int make_pkt(unsigned char localPCTport, unsigned char peerPCTport, struct in_addr localIP,
	     struct in_addr peerIP, char *payload, int payl_size,
	     unsigned char flags_pct, unsigned char nro_SEC_pct,
	     unsigned char nro_ACK_pct, char *packet);

void to_sockaddr(struct sockaddr_in &addr, struct in_addr &ip);

int esperarSYN_ACK(int socket_desc,unsigned char flag_chk,unsigned char localPCTport,struct sockaddr_in &src_addr, 
               struct in_addr  localIP ,struct pct_header *pkt_pct, struct iphdr * pkt_ip,int &recibiPKT);

void* timeout_thread(void*);

int inicializar_semaforos();


#endif	/* UTILS_H */

