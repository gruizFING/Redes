
#ifndef PCT_H_
#define PCT_H_

#include <sys/types.h>
#include <netinet/in.h>


#define DATA_FLAG		0x80		/* Mascara para deteccion de DATA */
#define SYN_FLAG		0x01		/* Mascara para deteccion de SYN */
#define ACK_FLAG		0x02		/* Mascara para deteccion de ACK */
#define FIN_FLAG		0x04		/* Mascara para deteccion de FIN */

#define TIMEOUT			30		/* Mascara para deteccion de FIN */
#define GBN_WINDOW		25		/* Mascara para deteccion de FIN */

#define MAX_PCT_DATA_SIZE	256		/* contenido maximo de datos en segmento PCT */

struct pct_header{ 
  unsigned char        srcPort;
  unsigned char        destPort;
  unsigned char        flags_pct;         /* Flags para control de pct TIPo -> 0x80, SYN -> 0x01, ACK ->0x02, FIN ->0x04 */
  unsigned char        nro_SEC_pct;       /* nro de secuencia de pct*/
  unsigned char        nro_ACK_pct;       /* nro de ACK de pct*/
};


int crearPCT(struct in_addr localIPaddr);
int aceptarPCT(unsigned char localPCTport);
int conectarPCT(unsigned char localPCTport,unsigned char peerPCTPport, struct in_addr peerIPaddr);
int escribirPCT(const void *buf, size_t len);
int leerPCT(void *buf, size_t len);
int cerrarPCT();


#endif /* PCT_H_ */

