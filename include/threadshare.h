#ifndef THREADSHARE_H
#define THREADSHARE_H

#include <pthread.h>
#include "sysproc.h"

// typedef struct s_shared {...} t_shared centralise :
// - les données système ;
// - l’état du programme ;
// - les mécanismes de synchronisation (running).
// Contexte partagé multithread.

typedef struct s_shared
{
    t_process         *listProc;
    int               nb;
    int               capacite;
    unsigned long     total_ram;
    unsigned long     avail_ram;
    unsigned long     ram_used;
    float             ram_percent;
    unsigned long     self_use;
    int               running;    // 1 = thread actif, 0 = demande d'arrêt
    pthread_mutex_t   mutex;
    
} t_shared;


// Cycle de vie
void    init_shared(t_shared *s);
void    free_shared(t_shared *s);

// Fonction du thread collecte
void    *thread_collecte(void *arg);

#endif

