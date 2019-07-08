#include "utils.h"

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



void insertar_elemento_cache(entrada* entrada)
{
    if(Cache != NULL)
    {
        entrada->sig = Cache->ppio;
        entrada->ant = NULL;
        Cache->ppio = entrada;
        
        if(entrada->sig == NULL) //0 elementos
        {
            Cache->final = entrada;
        }
        else
        {
            entrada->sig->ant = entrada;
        }
    }
}

void liberar_memoria_entrada(entrada* entrada)
{
    if(entrada != NULL)
    {
        delete [] entrada->objeto;
        delete [] entrada->url;
        delete [] entrada->fecha;
        P(memoria_utilizada_mutex);
        memoria_utilizada -= entrada->tamanio;
        V(memoria_utilizada_mutex);
        delete entrada;
        entrada = NULL;
    }
}

//PRE: la cache tiene n elementos
void borrar_n_elementos_final_cache(int n)
{
    if(Cache != NULL)
    {
        int i = 0;
        entrada* aux;
        entrada* aux2;
        aux = Cache->final;
        while(i < n && aux != NULL)
        {
//            if (aux != NULL)
//            {
//                Cache->final = aux->ant;
//                liberar_memoria_entrada(aux); 
//                aux = Cache->final;
//            }
            aux2 = aux->ant; 
            borrar_elemento_cache(aux);
            aux = aux2;
            i++;
        }
//        if(aux == NULL) // 0 objetos
//        {
//            Cache->ppio = NULL;
//        }
      }
}

//Metodos para clienteservidor

bool esGet(char* str)
{
    return strstr(str, "GET") != NULL;
}

bool esPost(char* str)
{
    return strstr(str, "POST") != NULL;   
}
void procesarCadena(char* cadena, int &argc, char** argv) {
    char * pch;
    pch = strtok(cadena, " ");
    int pchSize;
    int i = 0;

    while (pch != NULL) {
        pchSize = strlen(pch);
        argv[i] = strcpy(new char[pchSize + 1], pch);
        pch = strtok(NULL, " ");
        i++;
    }
    
    argc = i;
}

bool procesarComando(char* cadena, int &argc, char** argv) {
    char * pch;
    pch = strtok(cadena, " ");
    int pchSize;
    int i = 0;

    while (pch != NULL && i < 4) {
        pchSize = strlen(pch);
        argv[i] = strcpy(new char[pchSize + 1], pch);
        pch = strtok(NULL, " ");
        i++;
    }
    
    int j;
    for(j = 0; argv[i-1][j] != '\r' && argv[i-1][j] != '\n' && argv[i-1][j] != '\0'; j++);
    argv[i-1][j] = '\0';
    
    argc = i;
    if(pch != NULL)
        return false;
    else
        return true;
}

char* parseURL(char* str)
{
    char* ppio = strstr(str, "http://");
    int largo = strstr(ppio, " ") - ppio;
    char* url = new char[largo+1];
    strncpy(url,ppio,largo);
    url[largo] = '\0';
    return url;
}

char* parseAddress(char* url)
{
    char* copy = new char[strlen(url)+1];
    strcpy(copy,url);
    char* aux = copy + 7; //Salteando "http://"
    char* address = strtok(aux, ":/");
    char* addr = new char[strlen(address)+1];
    strcpy(addr,address);
    delete [] copy;
    return addr;
}

char* parsePort(char* url)
{
    char* copy = new char[strlen(url)+1];
    strcpy(copy,url);
    char* aux = copy + 7; //Salteando "http://"
    aux = strstr(aux, ":");
    char* port;
    if (aux != NULL)
        port = strtok(aux+1,"/");
    else
        port = strtok(copy,":");
    char* p = new char[strlen(port)+1];
    strcpy(p,port);
    delete [] copy;
    return p;        
}

long int parseTamanio(char* data)
{
    long int tamanio = -1;
    char* pos = strstr(data, "Content-Length: ");
    if (pos != NULL)
    {
        pos += strlen("Content-Length: ");
        int largo = strstr(pos, "\r\n") - pos; // \r\n == CRLF, en Unix el \n es solo LF 
        char* s = new char[largo+1];
        strncpy(s,pos,largo);
        s[largo] = '\0';
        tamanio = atol(s);
    }   

    return tamanio;        
}

char* parseExpires(char* data)
{
    char* pos = strstr(data, "Expires: ");
    if (pos != NULL)
    {
        pos +=  strlen("Expires: ");
        int largo = strstr(pos, "\r\n") - pos;
        char* fecha = new char[largo+1];
        strncpy(fecha,pos,largo);
        fecha[largo] = '\0';
        return fecha;
    }   
    return NULL;        
}

void updateExpires(entrada* ent, char* exp)
{
    if (ent != NULL)
    {
        ent->fechaExpiracion = parseFechaT(exp);
    }
}

char* parseFecha(char* data)
{
    char* pos = strstr(data, "Date: ");
    if (pos != NULL)
    {
        pos +=  strlen("Date: ");
        int largo = strstr(pos, "\r\n") - pos;
        char* fecha = new char[largo+1];
        strncpy(fecha,pos,largo);
        fecha[largo] = '\0';
        return fecha;
    }   
    return NULL;        
}

bool noEstaModificada(char* data)
{
    return strstr(data, "HTTP/1.1 304 Not Modified") != NULL;
}

char* modificarRequest(char* data, char* fecha)
{
    char* pos = strstr(data, "HTTP/1.1");
    if (pos != NULL)
    	strncpy(pos, "HTTP/1.0", 8);
    
    if (fecha != NULL) //Agregar campo if-last-modified
    {
        pos = strstr(data, "If-Modified-Since: ");
        if (pos != NULL)
	{
		pos += strlen("If-Modified-Since: ");
	        strncpy(pos, fecha, strlen(fecha));
	}
    }
    return data;
}

//Se copia solo lo necesario
entrada* copia(entrada* ent)
{
    entrada* copia = new entrada;
    copia->ant = NULL;
    copia->sig = ent; //Sirve para luego borrarla utilizando el metodo borrar_elemento_cache
    copia->tamanio = ent->tamanio;
    copia->fecha = new char[strlen(ent->fecha)+1];
    strcpy(copia->fecha,ent->fecha);
    copia->objeto = new char[ent->tamanio];
    memcpy(copia->objeto,ent->objeto,ent->tamanio);
    copia->fechaExpiracion = -1;
    copia->url = NULL;
    return copia;
}

time_t parseFechaT(char* fecha)
{
    if (fecha != NULL)
    {
        struct tm* tiempo = new tm;
        int argc;
        char** argv;
        procesarCadena(fecha,argc,argv);
        char* diaSemana = argv[0];
        int diaS;
        switch (diaSemana[0])
        {
            case ('M'):
            {
                diaS = 1;
                break;
            }
            case ('T'):
            {
                if (diaSemana[1] == 'u')
                    diaS = 2;
                else 
                    diaS = 4;
                break;
            }
            case ('W'):
            {
                diaS = 3;
                break;
            }
            case ('F'):
            {
                diaS = 5;
                break;
            }
            case('S'):
            {
                if (diaSemana[1] == 'a')
                    diaS = 6;
                else
                    diaS = 0;
                break;
            }
        }
        int diaMes = atoi(argv[1]);
        char* mes = argv[2];
        int mesI;
        switch (mes[0])
        {
            case ('J'):
            {
                if (mes[1] == 'a')
                    mesI = 0;
                else if (mes[2] == 'n')
                    mesI = 5;
                else
                    mesI = 6;
                break;
            }
            case('F'):
            {
                mesI = 1;
                break;
            }
            case('M'):
            {
                if (mes[2] == 'r')
                    mesI = 2;
                else
                    mesI = 4;
                break;
            }
            case('A'):
            {
                if (mes[1] == 'p')
                    mesI = 3;
                else
                    mesI = 7;
                break;
            }
            case('S'):
            {
                mesI = 8;
                break;
            }
            case('O'):
            {
                mesI = 9;
                break;
            }
            case('N'):
            {
                mesI = 10;
                break;
            }
            case('D'):
            {
                mesI = 11;
                break;
            }
        }
        int anio = atoi(argv[3]) - 1900;
        char* hora = argv[4];
        char* h = new char[3];
        h[0] = hora[0]; h[1] = hora[1]; h[2] = '\0';
        int horaI = atoi(h);
        char* m = new char[3];
        m[0] = hora[2]; m[1] = hora[3]; m[2] = '\0';
        int minI = atoi(m);
        char* s = new char[3];
        s[0] = hora[5]; s[1] = hora[6]; s[2] = '\0';
        int secI = atoi(s);
        char* zona = argv[5];

        tiempo->tm_sec = secI;
        tiempo->tm_hour = minI;
        tiempo->tm_mday = horaI;
        tiempo->tm_mon = mesI;
        tiempo->tm_wday = diaS;
        tiempo->tm_year = anio;
    //    tiempo->tm_gmtoff = 0;
    //    tiempo->tm_isdst = 0;
        tiempo->tm_zone = zona;

        return mktime(tiempo);
    }
    return -1;
}

bool expiro_entrada(entrada* entrada_cache)
{
    if (entrada_cache->fechaExpiracion != -1)
    {
        time_t tiempo = time(0);
        if (difftime(tiempo,entrada_cache->fechaExpiracion) >= 0.0)
            return true;
    }
    return false;
}

void borrar_elemento_cache(entrada* entrada_cache)
{
    if (entrada_cache != NULL)
    {
        if (entrada_cache->ant == NULL) //Primero
        {
            Cache->ppio = entrada_cache->sig;
            if (Cache->ppio == NULL) // 1 solo
                Cache->final = NULL;
            else
                Cache->ppio->ant = NULL;
        }
        else if (entrada_cache->sig == NULL) //Ultimo
        {
            Cache->final = entrada_cache->ant;
            Cache->final->sig = NULL;
        }
        else
        {
            entrada_cache->ant->sig = entrada_cache->sig;
            entrada_cache->sig->ant = entrada_cache->ant;
        }
        liberar_memoria_entrada(entrada_cache);
    }
}

void mover_al_ppio(entrada* ent)
{
    if (ent->ant != NULL)
    {
        if (ent->sig == NULL)
        {
            ent->ant->sig = NULL;
            Cache->final = ent->ant;
        }
        else
        {
            ent->ant->sig = ent->sig;
            ent->sig->ant = ent->ant;
        }
        ent->ant = NULL;
        ent->sig = Cache->ppio;
        Cache->ppio->ant = ent;
        Cache->ppio = ent;
    }
}


entrada* buscar_objeto_cache(char* url)
{
    entrada* res = NULL;
    if (Cache != NULL)
    {
        entrada* pos = Cache->ppio;
        while (pos != NULL && res == NULL)
        {
            if (strcmp(pos->url,url) == 0)
            {
                if (expiro_entrada(pos))
                {
                    borrar_elemento_cache(pos);
                    object_count--;            
                }
                else
                {
                    res = copia(pos);
                    mover_al_ppio(pos);
                }
            }
            else
                pos = pos->sig;
        }
    }
    return res;
}


void save_in_cache(char* data_response, long int tam, char* url)
{
    entrada* ent = new entrada;
    ent->ant = NULL;
    ent->fecha = parseFecha(data_response);
    ent->fechaExpiracion = parseFechaT(parseExpires(data_response));
    ent->tamanio = tam;
    ent->url = new char[strlen(url)+1];
    strcpy(ent->url,url);
    ent->objeto = new char[tam];
    memcpy(ent->objeto,data_response,tam);
    
    ent->sig = Cache->ppio;
    if (ent->sig == NULL)
        Cache->final = ent;
    else
        ent->sig->ant = ent;
    Cache->ppio = ent;
}

