#include <stdlib.h>
#include <unistd.h>
#include "threadshare.h"
#include "sysproc.h"

#define COLLECT_INTERVAL 200000  // 200ms en microsecondes

void init_shared(t_shared *s)
{
    s->listProc   = NULL;
    s->nb        = 0;
    s->capacite  = 0;
    s->total_ram = 0;
    s->avail_ram = 0;
    s->ram_used  = 0;
    s->ram_percent = 0.0f;
    s->self_use  = 0;
    s->running   = 1;
    
    // initialisation le mutex — obligatoire avant tout lock/unlock :
    pthread_mutex_init(&s->mutex, NULL);    // Le NULL signifie attributs par défaut.    
}

void free_shared(t_shared *s)
{
    free(s->listProc);
    s->listProc = NULL;
    pthread_mutex_destroy(&s->mutex);
}


void *thread_collecte(void *arg)
{
    t_shared *s = (t_shared *)arg;
    DIR      *d;
    int      nb;
    int icapa_t_process;

    while (1)
    {
        // Vérifier le signal d'arrêt
        // Sans mutex (running est écrit une seule fois par main : lecture safe)
        if (!s->running)
            break;

        d = opendir("/proc");
        if (!d)
        {
            usleep(COLLECT_INTERVAL); // On attend 200 ms avant de recommencer
            continue;
        }

        nb = compter_processus(d);
        

        // ==== SECTION CRITIQUE MUTEX: on écrit dans les données partagées ========
        //
        pthread_mutex_lock(&s->mutex);

        if (nb > s->capacite)
        {
            icapa_t_process = nb + 20; // On garde une marge de 20 en réallouant l'espace mémoire
            t_process *tmp = realloc(s->listProc, icapa_t_process * sizeof(t_process));
            if (tmp)
            {
                s->listProc    = tmp;
                s->capacite = icapa_t_process;
            }
            else
            {
                pthread_mutex_unlock(&s->mutex);
		// On libère le mutex le plus tôt possible
                closedir(d);
                usleep(COLLECT_INTERVAL);
                continue;  // Si realloc échoue, on a déjà fait unlock et on saute ce cycle.
                // Les données partagées restent dans leur état précédent — l'UI continue d'afficher l'ancienne liste en attendant.
                // Voir ensuite comment alerter si l'erreur persiste cycle après cycle, ce qui est peu probable.
            }
        }

        remplir_liste_processus(d, s->listProc, nb);
        s->nb = nb;
        
        //  RAM globale  :  Lecture de "/proc/meminfo" :
        // Mise à jour des données RAM
        update_ram_info(&s->total_ram, &s->avail_ram, &s->ram_used, &s->ram_percent, &s->self_use);

        pthread_mutex_unlock(&s->mutex);
        //
        // === FIN SECTION CRITIQUE MUTEX ================================================
        
       // Comme en cas d'échec du realloc, on libère le mutex le plus tôt possible (le thread UI n'attend pas qu'on ferme le dossier).
       // closedir(d) ne touche pas aux données partagées, elle peut se faire dehors.
        closedir(d);
        usleep(COLLECT_INTERVAL);
    }

    return NULL;
    // La signature imposée par pthread est void *(*)(void *). Le retour NULL est la convention pour "terminé sans erreur".
}

