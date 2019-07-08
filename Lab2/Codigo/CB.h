/* 
 * File:   CB.h
 * Author: bdi-usr
 *
 * Created on 14 de octubre de 2012, 08:21 AM
 */

#ifndef CB_H
#define	CB_H
#include "utils.h"

#define PACKET_BUFFER_SIZE 256 // tamanio de buffer de paquetes
#define DATA_BUFFER_SIZE 10000

class CB {
public:
    
    static CB* Inst();

    virtual ~CB();
    
    void restart();
    
    // PACKET BUFFER
    
    // Poner un paquete { item } de tamanio { item size } en el buffer
    int push_packet(char *item, int item_size);

    // Sacar un paquete del buffer. El paquete se almacen en holder si
    // el tamanio del paquete es menor o igual a holder_size. De lo contrario
    // se retorna 0 y el paquete no se almacena en holder.
    int pop_packet(char *holder, int holder_size);
    
    // Se lee el paquete i contando desde tail, del buffer. 
    int read_i_from_tail(int i, char *holder, int holder_size);
    
    void delete_tail();
    
    int CBcount();
    
    int base();
    
    // head % 256
    int signumsec();
    
    // DATA BUFFER
    
    int push_data(char *item, int item_size);
    
    int pop_data(char *holder, int holder_size);
    
    int numsecesperado();
    
private:
    
    CB();
    
    static CB* instance;
    
    //pthread_mutex_t* mutex;
    
    char packets[PACKET_BUFFER_SIZE][MAX_PACKET_SIZE];
    int packets_size[PACKET_BUFFER_SIZE];
    int count;     // number of items in the buffer
    int head;       // pointer to head ( escritor )
    int tail;    // pointer to tail ( lector )
    
    
    char data[DATA_BUFFER_SIZE];     // data buffer
    int data_end; // end of data buffer
    int data_count;     // number of items in the buffer
    int data_head;       // pointer to head ( escritor )
    int data_tail;       // pointer to tail ( lector )
    int entrys;
    
};

#endif	/* CB_H */

