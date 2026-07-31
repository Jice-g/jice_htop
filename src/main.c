/**
 * @file main.c
 * @brief Point d’entrée de jice_htop — un moniteur de processus interactif léger.
 *
 * Lance un thread collecteur en arrière‑plan qui lit périodiquement /proc
 * et met à jour les données système partagées.  
 * Le thread principal exécute la boucle d’affichage ncurses :
 * il prend une copie des données partagées sous mutex, trie et filtre la liste
 * des processus, puis affiche l’interface.
 *
 * Vue d’ensemble de l’architecture :
 *   - threadshare : structure de données partagée + cycle de vie du thread collecteur
 *   - sysproc     : parsing de /proc, métriques RAM, comparateurs de tri
 *   - uiwin       : initialisation ncurses, fonctions d’affichage, gestion des entrées
 *
/*
 *
 *  NOTA :
 * -------
 *  Des améliorations sont encore possibles :
 *
 *  1°) Créer un second thread th_render plutôt que de gérer l'affichage directement dans main.
 *      Ceci impliquerait que le thread d’affichage possède son propre cycle de rendu indépendant,
 *      avec un accès synchronisé aux données partagées.
 *      Le thread principal deviendrait alors un orchestrateur : initialisation, lancement des threads,
 *      gestion du filtre et du mode de tri, puis arrêt propre de l’ensemble.
 *      Cette séparation renforcerait la réactivité de l’UI et isolerait complètement la logique d’affichage
 *      de la logique de supervision système.
 *
 *  2°)
 *      a- Sortir les affichages de bannière et de bandeau pour les placer dans uiwin.h / uiwin.c,
 *       et les faire gérer directement par th_render.
 *
 *      b- Le remplissage du buffer snap deviendrait la responsabilité de th_render,
        ce qui permettrait de réduire encore la durée des sections critiques dans le thread collecteur.
 *
 *  Ces deux éléments figurent dans la Feuille de route du projet (docs/)
 */


#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include "sysproc.h"
#include "uiwin.h"
#include "threadshare.h"


int main(void)
{


    /* -------------------------------------------------------------------------
     * État d’interaction utilisateur
     * ------------------------------------------------------------------------- */
     
    int         key;
    char        filter[256];    // Filtre de sous-chaîne saisi par l’utilisateur via '/'
    filter[0]   = '\0';
    t_sort_mode sort_mode = SORT_PID;
    int         err_flag  = 0;



    /* -------------------------------------------------------------------------
     * Mise en page de l’UI et état du défilement
     * Chaque variable est réinitialisée au début de chaque itération d’affichage.
     * ------------------------------------------------------------------------- */
     
    char status_bar[512];       // Chaîne formatée de la barre d’état (bas de l’écran) 
    int  lines_written  = 0;    // Lignes réellement affichées dans le panneau des processus 
    int  nb_displayed   = 0;    // Lignes correspondant au filtre actif 
    int  lines_avail    = 0;    // Lignes disponibles entre les bandeaux haut et bas 
    int  bar_pos        = 0;    // Position du curseur de la barre de défilement 
    int  scroll_offset  = 0;    // Index de la première ligne visible 
    int  max_scroll     = 0;    // Décalage maximal atteignable 
    int  bar_height     = 0;    // Hauteur de la barre de défilement 



    /* -------------------------------------------------------------------------
     * Données partagées (thread collecteur <-> boucle d’affichage)
     * ------------------------------------------------------------------------- */
     
    t_shared  shared;
    pthread_t th_collector;

    /*
     * Tampon de snapshot — la boucle d’affichage travaille sur une copie privée
     * de la liste des processus pour garder la section critique aussi courte
     * que possible.  
     * La capacité initiale est de 256 entrées ; realloc() l’étend si nécessaire.
     */
    t_process      *snap_list       = NULL;
    int            snap_count       = 0;
    int            snap_capacity    = 0;
    unsigned long  snap_total_ram   = 0;
    unsigned long  snap_avail_ram   = 0;
    unsigned long  snap_ram_used    = 0;
    unsigned long  snap_self_use    = 0;
    float          snap_ram_percent = 0.0f;

    snap_list = calloc(256, sizeof(t_process));
    if (!snap_list) {
        perror("calloc snap_list");
        return 1;
    }
    snap_capacity = 256;


    /* =========================================================================
     * Initialisation
     * ========================================================================= */

    init_shared(&shared);

    if (pthread_create(&th_collector, NULL, collector_thread, &shared) != 0) {
        perror("pthread_create");
        free_shared(&shared);
        free(snap_list);
        return 1;
    }

    init_ncurses();


    /* =========================================================================
     * Boucle principale d’affichage
     * ========================================================================= */

    do {
        /* Réinitialisation des variables de mise en page pour cette frame */
        lines_written = 0;
        lines_avail   = 0;
        bar_pos       = 0;
        bar_height    = 0;
        max_scroll    = 0;


        /* ---------------------------------------------------------------------
         * Section critique — copie des données partagées
         *
         * Le verrou est maintenu uniquement pendant la copie, afin que le
         * thread collecteur ne soit pas bloqué pendant l’affichage.  
         * realloc() est effectué sous verrou car snap_list doit correspondre
         * à snap_count avant memcpy.
         * --------------------------------------------------------------------- */
 
        pthread_mutex_lock(&shared.mutex);

        snap_count       = shared.nb;
        snap_total_ram   = shared.total_ram;
        snap_avail_ram   = shared.avail_ram;
        snap_ram_used    = shared.ram_used;
        snap_ram_percent = shared.ram_percent;
        snap_self_use    = shared.self_use;

        if (snap_count > snap_capacity) {
            snap_capacity = snap_count + 20;
            t_process *tmp = realloc(snap_list, snap_capacity * sizeof(t_process));
            if (tmp) {
                snap_list = tmp;
            } else {
                pthread_mutex_unlock(&shared.mutex);
                perror("realloc snap_list failed");
                err_flag = 1;
                break;
            }
        }

        if (snap_count > 0 && shared.proc_list)
            memcpy(snap_list, shared.proc_list, snap_count * sizeof(t_process));
            
        pthread_mutex_unlock(&shared.mutex);
        /* Fin de la section critique */


        /* ---------------------------------------------------------------------
         * Tri du snapshot selon le mode choisi par l’utilisateur
         * --------------------------------------------------------------------- */
        switch_sort(sort_mode, snap_list, snap_count);


        /* ---------------------------------------------------------------------
         * Affichage — bannière supérieure
         * --------------------------------------------------------------------- */
        clear();

        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(0, 0, "JICE-HTOP | Processus : %d                      ", snap_count);
        attroff(COLOR_PAIR(1) | A_BOLD);

        ram_display(snap_total_ram, snap_avail_ram,
                    snap_ram_used, snap_ram_percent, snap_self_use);
        draw_header();


        /* ---------------------------------------------------------------------
         * Affichage — liste des processus avec filtre optionnel
         * --------------------------------------------------------------------- */
        attron(COLOR_PAIR(3));

        /* Compter les lignes correspondant au filtre avant l’affichage */
        nb_displayed = 0;
        for (int k = 0; k < snap_count; k++) {
            if (filter[0] == '\0' || match_filter(snap_list[k].name, filter))
                nb_displayed++;
        }

        if (nb_displayed == 0) {
            /* Efface le panneau des processus et affiche un message */
            for (int y = 0; y < LINES - (L_LIST_PROCESS + 2); y++)
                mvprintw(L_LIST_PROCESS + y, 1, "                                        ");

            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(L_LIST_PROCESS, 1, "Aucun processus ne correspond au filtre.");
            attroff(COLOR_PAIR(3) | A_BOLD);

        }

        /* Calcul de la barre de défilement, puis affichage des lignes visibles */
        lines_avail = LINES - (L_LIST_PROCESS + 2);
        compute_scroll(nb_displayed, lines_avail,
                      &scroll_offset, &max_scroll, &bar_height, &bar_pos);
        draw_scrollbar(bar_height, bar_pos, L_LIST_PROCESS);

        int i = 0;   /* Index relatif à la liste filtrée */
        int j = 0;   /* Index absolu dans snap_list */

        while ((i + scroll_offset < snap_count) && (lines_written < lines_avail)) {
            j = i + scroll_offset;

            if (filter[0] != '\0' && !match_filter(snap_list[j].name, filter)) {
                i++;
                continue;
            }

            mvprintw(lines_written + L_LIST_PROCESS, 1, "%-5d %9ld    %s",
                     snap_list[j].pid,
                     snap_list[j].mem_kb,
                     snap_list[j].name);
            lines_written++;
            i++;
        }

        attroff(COLOR_PAIR(3));


        /* ---------------------------------------------------------------------
         * Affichage — bandeaux inférieurs
         *   Ligne LINES-2 : rappel des raccourcis
         *   Ligne LINES-1 : état dynamique (tri + filtre)
         * --------------------------------------------------------------------- */
        attron(COLOR_PAIR(4));
        mvprintw(LINES - 2, 0,
            "'q' quitter | 'p' tri PID | 'n' tri NOM | 'm' tri MEM"
            " | '/' + filtre + Entree pour filtrer, '/' + Entree pour effacer.");
        attroff(COLOR_PAIR(4));

        attron(COLOR_PAIR(5) | A_BOLD);
        low_status_bar(status_bar, COLS, sort_mode, filter);
        mvprintw(LINES - 1, 0, "%s", status_bar);
        attroff(COLOR_PAIR(5) | A_BOLD);


        /* ---------------------------------------------------------------------
         * Entrée — rafraîchit l’affichage puis attend une touche jusqu’à REFRESH_TIME ms
         * --------------------------------------------------------------------- */
        refresh();

        key = getch();
        if (get_keypressed(key, &sort_mode, &scroll_offset,
                           filter, &shared.running, &shared.mutex))
            break;

    } while (1);


    /* =========================================================================
     * Nettoyage — l’ordre est important :
     *   1. Joindre le thread collecteur (il vérifie shared.running pour sortir).
     *   2. Libérer la mémoire partagée et le mutex.
     *   3. Libérer le tampon de snapshot.
     *   4. Restaurer le terminal via endwin().
     * ========================================================================= */
    pthread_join(th_collector, NULL);
    free_shared(&shared);
    free(snap_list);
    endwin();

    return err_flag;
}

