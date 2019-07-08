/* 
 * File:   CB.cpp
 * Author: bdi-usr
 * 
 * Created on 14 de octubre de 2012, 08:21 AM
 */

#include "CB.h"
#include <stdlib.h>
#include <string.h>

CB* CB::instance(0);

CB* CB::Inst() {
    if (!instance)
        instance = new CB();

    return instance;
}

CB::CB() {
    count = 0;
    head = 0;
    tail = 0;
    
    data_end = DATA_BUFFER_SIZE;
    data_count = 0;
    data_head = 0; 
    data_tail = 0;
    entrys = 0;
}

void CB::restart()
{
    count = 0;
    head = 0;
    tail = 0;
    
    data_end = DATA_BUFFER_SIZE;
    data_count = 0;
    data_head = 0; 
    data_tail = 0;
    entrys = 0;
}

 int CB::push_packet(char *item, int item_size)
 {
     if(count == PACKET_BUFFER_SIZE) // Si esta lleno no se copia nada
        return 0;
     
     if(item_size > MAX_PACKET_SIZE) // No se ingresa un paquete cuyo tamanio exceda el maximo
         return 0;
     
     memcpy(packets[head], item, item_size);
     packets_size[head] = item_size;
     head++;
     if(head == PACKET_BUFFER_SIZE)
        head = 0;
     count++;

     return item_size;
 }

   
int CB::pop_packet(char *holder, int holder_size)
{
    if(count == 0 || holder_size < 0) // Si esta vacio no se lee nada
        return 0;
    
    if(holder_size < packets_size[tail]) // No se quita un paquete si no cabe en el holder
        return 0;

    memcpy(holder, packets[tail], packets_size[tail]);
    tail++;
    if(tail == PACKET_BUFFER_SIZE)
        tail = 0;
    count--;
    return packets_size[tail];
}

int CB::read_i_from_tail(int i, char *holder, int holder_size)
{
    if(i >= count || i < 0) // No esta ese elemento
        return 0;
    
    int j = (tail + i) % PACKET_BUFFER_SIZE;

    if(holder_size < packets_size[j]) // No se devuelve un paquete si no cabe completamente en holder
        return 0;
    
    memcpy(holder, packets[j], packets_size[j]);
    
    return packets_size[j];
}

void CB::delete_tail()
{
    if(count > 0){
        tail++;
        if(tail == PACKET_BUFFER_SIZE)
            tail = 0;
        count--;
    }
}

int CB::CBcount()
{
    return count;
}

int CB::base()
{
    return tail % 256;
}
    
int CB::signumsec()
{
    return head % 256;
}

// DATA BUFFER

int CB::push_data(char *item, int item_size)
{
    if(data_count == DATA_BUFFER_SIZE) // Si esta lleno no se copia nada
        return 0;
    
    int copy_size; // Tamanio a copiar
    
    // condition ? first_expression : second_expression;
    // Determino el minimo entre el espacio en el buffer y el tamanio del item
    copy_size = ( item_size <= (DATA_BUFFER_SIZE - data_count) ) ? item_size : DATA_BUFFER_SIZE - data_count;
    
    int to_end_size; // Tamanio hasta el final del buffer
    to_end_size = data_end - data_head;
    
    if(to_end_size >= copy_size) // Si hay suficiente espacio antes de llegar al final
    {
        memcpy(&data[data_head], item, copy_size);
        data_head += copy_size;
    }
    else // Si tengo que dar la vuelta
    {
        memcpy(&data[data_head], item, to_end_size);
        memcpy(data, item+to_end_size, copy_size - to_end_size);
        data_head = (copy_size - to_end_size);
    }
    data_count += item_size;
    entrys++;
    return copy_size;
}
    
int CB::pop_data(char *holder, int holder_size)
{
    if(data_count == 0) // Si esta vacio no se lee nada
        return 0;
    
    int read_size; // Tamanio a leer
    
    // Determino el minimo entre lo disponible para leer y el tamanio del item (buffer de salida)
    read_size = ( holder_size <= data_count ) ? holder_size : data_count;
    
    int to_end_size; // Tamanio hasta el final del buffer
    to_end_size = data_end - data_tail;
    
    if(to_end_size >= read_size) // Si los datos a leer se almacenan antes del fin del buffer
    {
        memcpy(holder, &data[data_tail], read_size);
        data_tail += read_size;
    }
    else // Tengo que dar la vuelta
    {
        memcpy(holder, &data[data_tail], to_end_size);
        memcpy(holder+to_end_size, data, read_size - to_end_size);
        data_tail = (read_size - to_end_size);
    }
    data_count -= read_size;
    return read_size;
}

int CB::numsecesperado()
{
    return entrys % 256;
}


CB::~CB() {
}

