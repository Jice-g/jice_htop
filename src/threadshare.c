/**
 * @file threadshare.c
 * @brief Cycle de vie de t_shared et du thread collecteur en arrière‑plan.
 *
 * Le thread collecteur fonctionne indépendamment de la boucle d’affichage.
 * Il possède le côté écriture de t_shared 
 * c’est le seul thread qui modifie listProc, nb, capacity et les métriques de RAM.  
 * Le thread principal lit ces champs en prenant une copie protégée par le mutex
 * (voir main.c).
 */

#include <stdlib.h>
#include <unistd.h>
#include "threadshare.h"
#include "sysproc.h"

/** Durée de sommeil du collecteur entre deux scans de /proc, en microsecondes (= 200 miliseconde). */
#define COLLECT_INTERVAL 200000


/* =========================================================================
 * Cycle de vie
 * ========================================================================= */

void init_shared(t_shared *s)
{
    s->proc_list    = NULL;
    s->nb          = 0;
    s->capacity    = 0;
    s->total_ram   = 0;
    s->avail_ram   = 0;
    s->ram_used    = 0;
    s->ram_percent = 0.0f;
    s->self_use    = 0;
    s->running     = 1;

    /*
     * pthread_mutex_init() doit être appelé avant toute tentative de lock/unlock.
     * Des attributs NULL sélectionnent le type de mutex par défaut (rapide, non récursif -> voir la documentation de la fonction pour comprendre).
     */
    pthread_mutex_init(&s->mutex, NULL);
}


void free_shared(t_shared *s)
{
    free(s->proc_list);
    s->proc_list = NULL;
    pthread_mutex_destroy(&s->mutex);
}


/* =========================================================================
 * Thread collecteur
 * ========================================================================= */

void *collector_thread(void *arg)
{
    t_shared *s = (t_shared *)arg;
    DIR*  d;
    int   nb;
    int   new_capa;

    while (1) {

       /*
        * Vérifie le flag d’arrêt (running) sous protection du mutex.
        *
        * s->running est écrit une seule fois par le thread principal (mis à 0
        * quand l’utilisateur appuie sur 'q', dans uiwin.c get_keypressed()) et
        * cette écriture se fait sous s->mutex. Le lire ici sous le même mutex
        * garde les accès symétriques, au lieu de compter sur l’atomicité
        * matérielle d’un int aligné sur x86.
        *
        * Cette section critique est volontairement minuscule et de durée fixe
        * (une seule lecture d’int), donc elle ne concurrence jamais réellement
        * l’utilisation du mutex par le thread d’affichage.
        */
        pthread_mutex_lock(&s->mutex);
        int still_running = s->running;
        pthread_mutex_unlock(&s->mutex);

        if (!still_running)
            break;

        d = opendir("/proc");

        if (!d) {
            usleep(COLLECT_INTERVAL);
            continue;
        }

        nb = count_processes(d);
        // Roadmap : il y a ici un comptage qui s'effectue avant le scan.
        // il est possible de compter en enregistrant : c'est une amélioration future à implémenter
        // Car il peut y avoir une latence entre le comptage et l'enregistrement.

        /* -----------------------------------------------------------------
         * Section critique — écriture des données partagées
         * ----------------------------------------------------------------- */
        pthread_mutex_lock(&s->mutex);

        if (nb > s->capacity) {
            /*
             * Augmenter la capacité avec une marge de 20 entrées pour réduire
             * les futures reallocations lorsque le nombre de processus varie
             * autour de la capacité actuelle.
             */
            new_capa = nb + 20;
            t_process *tmp = realloc(s->proc_list, new_capa * sizeof(t_process));
            
            if (tmp) {
                s->proc_list = tmp;
                s->capacity = new_capa;
            } else {
                /*
                 * realloc() a échoué : relâcher le mutex et ignorer ce cycle.
                 * La boucle d’affichage continue de montrer l’ancienne liste
                 * jusqu’au prochain cycle réussi.
                 * N.B : Echecs répétés peu probables, mais penser à le gérer
                 */
                pthread_mutex_unlock(&s->mutex);
                closedir(d);
                usleep(COLLECT_INTERVAL);
                continue;  // on recommence du début
            }
        }

        fill_process_list(d, s->proc_list, nb);
        s->nb = nb;
        update_ram_info(&s->total_ram, &s->avail_ram, &s->ram_used, &s->ram_percent, &s->self_use);

        pthread_mutex_unlock(&s->mutex);
        /* Fin de la section critique */

        /*
         * closedir() ne touche pas aux données partagées, donc il est exécuté
         * en dehors de la section critique pour réduire au minimum le temps
         * pendant lequel le mutex est verrouillé.
         */
        closedir(d);
        usleep(COLLECT_INTERVAL);
    }

    /* Convention pthread : retourner NULL pour signaler une fin propre. */
    return NULL;
}

