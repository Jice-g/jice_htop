/**
 * @file threadshare.h
 * @brief Structure de données partagée et interface du thread collecteur.
 *
 * t_shared est la source unique de vérité échangée entre les threads
 * collecteur en arrière‑plan (thread_collector) et la boucle principale d’affichage (dans main.c).  
 * Tous les accès aux champs modifiables doivent être protégés par le mutex intégré.
 * On utilisera un snap pour raccourcir la zone critique mutex au minimum.
 */

#ifndef THREADSHARE_H
#define THREADSHARE_H

#include <pthread.h>
#include "sysproc.h"


/**
 * @brief Données partagées entre le thread collecteur et la boucle d’affichage.
 *
 * Le thread collecteur écrit dans cette structure,
 * la boucle d’affichage la lit en prenant une copie privée sous protection du mutex (voir main.c).
 *
 * Règles de propriété :
 *   - @c proc_list est alloué (et le cas échéant réallouer) sur le tas par le collecteur et libéré par free_shared().
 *   - @c running est le seul champ écrit par le thread principal,
 *        il indique au collecteur quand terminer proprement (quand l'utilisateur décide de quitter le programme).
 */
typedef struct s_shared
{
    t_process       *proc_list;     /**< Tableau de processus alloué sur le tas (capacité : @c capacity) */
    int              nb;            /**< Nombre d’entrées valides dans @c proc_list                      */
    int              capacity;      /**< Capacité allouée de @c proc_list, en nombre d’entrées           */
    unsigned long    total_ram;     /**< Quantité totale de RAM installée, en kB                         */
    unsigned long    avail_ram;     /**< RAM disponible, en kB                                           */
    unsigned long    ram_used;      /**< RAM utilisée (total - disponible), en kB                        */
    float            ram_percent;   /**< Pourcentage de RAM utilisée                                     */
    unsigned long    self_use;      /**< Pic de RSS de ce processus, en kB                               */
    int              running;       /**< Contrôle du collecteur : 1 = actif, 0 = arrêt                   */
    pthread_mutex_t  mutex;         /**< Protège tous les champs ci‑dessus                               */
} t_shared;


/* ---------------------------------------------------------------------------
 * Cycle de vie
 * ------------------------------------------------------------------------- */

/**
 * @brief Initialise tous les champs de @p s avec des valeurs sûres et crée le mutex.
 *
 * Doit être appelé avant de lancer le thread collecteur.
 *
 * @param s  Pointeur vers l’instance t_shared à initialiser. Ne doit pas être NULL.
 */
void init_shared(t_shared *s);


/**
 * @brief Libère les ressources détenues par @p s (tableau de processus et mutex).
 *
 * Ne doit être appelé qu’après la terminaison du thread collecteur.
 *
 * @param s  Pointeur vers l’instance t_shared à détruire. Ne doit pas être NULL.
 */
void free_shared(t_shared *s);


/* ---------------------------------------------------------------------------
 * Thread collecteur
 * ------------------------------------------------------------------------- */

/**
 * @brief Point d’entrée du thread collecteur en arrière‑plan.
 *
 * Fonctionne en boucle à intervalles de COLLECT_INTERVAL microsecondes :
 *   1. Vérifie s->running ; quitte si 0. Lecture sécurisée par mutex.
 *   2. Ouvre /proc et compte les processus actifs en les lisant.
 *   3. Prend le mutex et met à jour s->proc_list, s->nb et les métriques RAM.
 *   4. Relâche le mutex et ferme /proc.
 *
 * En cas d’échec de realloc(), le cycle courant est ignoré ;
 * les données précédentes restent visibles pour la boucle d’affichage jusqu’au prochain cycle réussi.
 * (on pourra voir comment sortir si l'échec se répète sans cesse - peu probable)
 * @param arg  Pointeur vers l’instance t_shared (converti depuis void *).
 * 
 * @return     Toujours NULL (convention pthread pour « terminé sans erreur »).
 */
void *collector_thread(void *arg);


#endif /* THREADSHARE_H */

