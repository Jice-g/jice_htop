/**
 * @file uiwin.c
 * @brief Initialisation ncurses, fonctions d’affichage et gestion du clavier.
 *
 * Toutes les fonctions d’affichage supposent que init_ncurses() a été appelée avant.
 * Les indices des paires de couleurs correspondent à ceux déclarés dans uiwin.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <ncurses.h>
#include "uiwin.h"


/* =========================================================================
 * Initialisation
 * ========================================================================= */

void init_ncurses(void)
{
    initscr();
    noecho();
    curs_set(FALSE);            /* Cache le curseur pendant l’affichage normal. */
    keypad(stdscr, TRUE);       /* Active les touches spéciales (flèches, F1, etc.). */
    timeout(REFRESH_TIME);      /* getch() renvoie ERR après REFRESH_TIME ms. */

    start_color();
    init_pair(1, COLOR_BLACK,  COLOR_WHITE);   /* Bannière de titre */
    init_pair(2, COLOR_WHITE,  COLOR_BLACK);   /* En-têtes de colonnes */
    init_pair(3, COLOR_GREEN,  COLOR_BLACK);   /* Lignes de processus */
    init_pair(4, COLOR_CYAN,   COLOR_BLACK);   /* Rappel des raccourcis */
    init_pair(5, COLOR_BLACK,  COLOR_YELLOW);  /* Barre d’état dynamique */
}


/* =========================================================================
 * Affichage
 * ========================================================================= */

void ram_display(const unsigned long total, const unsigned long avail,
                 const unsigned long used, const float percent,
                 const unsigned long self_use)
{
    attron(COLOR_PAIR(3));
    mvprintw(L_DATA_GLOBAL + 0, 0, "%-16s %6lu MB", "Mem totale:",   total / 1024);
    mvprintw(L_DATA_GLOBAL + 1, 0, "%-16s %6lu MB  %.1f%%      |    JICE_HTOP ressources : %lu kB",
             "Mem utilisee:", used  / 1024, percent, self_use);
    mvprintw(L_DATA_GLOBAL + 2, 0, "%-16s %6lu MB", "Mem libre:",    avail / 1024);
    attroff(COLOR_PAIR(3));
}


void draw_header(void)
{
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(L_TAB_PROCESS + 0, 0, "------------------------------------------------");
    mvprintw(L_TAB_PROCESS + 1, 0, " %-5s  %9s   %s", "PID", "MEM (kB)", "NOM");
    mvprintw(L_TAB_PROCESS + 2, 0, "------------------------------------------------");
    attroff(COLOR_PAIR(2) | A_BOLD);
}


int low_status_bar(char *dest, int sMax, t_sort_mode smode, const char *filter)
{
    const char* sort_label =
        (smode == SORT_PID)  ? "PID" :
        (smode == SORT_NAME) ? "NOM" : "MEM"; // pas d'autres choix possibles

    /* Affiche une chaîne vide si aucun filtre n’est actif. */
    const char* fltr = (filter && filter[0]) ? filter : "";

    snprintf(dest, sMax, "Tri: %s | Filtre: %s", sort_label, fltr);
    
    return (int)strlen(dest); //Le type renvoyé par strlen(...) est size_t, on a décidé que la fonction renvoyait un int.
}


void compute_scroll(int nb_visible, int avail_lines, int *scroll_offset, int *max_scroll, int *bar_height, int *bar_pos)
{
    *max_scroll = nb_visible - avail_lines;
    if (*max_scroll < 0) *max_scroll = 0;

    /* Force scroll_offset dans l’intervalle valide [0, max_scroll]. */
    if (*scroll_offset < 0)		*scroll_offset = 0;
    if (*scroll_offset > *max_scroll)	*scroll_offset = *max_scroll;

    /*
     * bar_height vaut la hauteur de la zone visible si la liste déborde,
     * ou la longueur de la liste si elle tient entièrement — ce qui fait
     * que le curseur occupe toute la barre dans ce cas.
     */
    *bar_height = (nb_visible > avail_lines) ? avail_lines : nb_visible;

    /*
     * Convertit scroll_offset en une position de curseur dans [0, bar_height-1].
     * La division entière est volontaire : la précision fine n’est pas utile ici.
     */
    *bar_pos = (*max_scroll == 0)
               ? 0
               : (*scroll_offset * (*bar_height - 1)) / *max_scroll;
}


void draw_scrollbar(int bar_height, int bar_pos, int y0)
{
    for (int y = 0; y < bar_height; y++)
        mvprintw(y0 + y, COLS - 3, (y == bar_pos) ? "[=]" : "|||");
/*
*   Pour un rendu semblable à ceci :
*
*		|||
*		|||
*		[=]  qui monte et qui descend
*		|||
*        	|||
*/
}


/* =========================================================================
 * Filtrage
 * ========================================================================= */

int match_filter(const char *strg, const char *sub)
{
    /* Un filtre vide ou NULL correspond toujours. */
    if (!strg || !sub || !sub[0])
        return 1;

    /*
     * Parcourt strg avec une fenêtre de la taille de sub,
     * en comparant caractère par caractère sans tenir compte de la casse,
     * dans des boucles imbriquées for... while et deux index i et j
     * Renvoie 1 dès qu’une correspondance est trouvée dans le while à la position i du for.
     */
    for (int i = 0; strg[i]; i++) {
        int j = 0;
        while (sub[j] &&
               tolower((unsigned char)strg[i + j]) ==
               tolower((unsigned char)sub[j]))
            j++;
        if (sub[j] == '\0')
            return 1;
    }

    return 0;
}


/* =========================================================================
 * Gestion des entrées
 * ========================================================================= */

int get_keypressed(int key, t_sort_mode *mode, int *scroll_offset,
                   char *filter, int *running, pthread_mutex_t *mutex)
{
    if (key == 'q' || key == 'Q') {
        /*
         * Signale au thread collecteur qu’il doit s’arrêter. "Pseudo-évènement".
         * L’écriture dans *running est protégée par le mutex, même si
         * ce n’est qu’une simple affectation, pour respecter le contrat
         * de synchronisation de manière formelle.
         * Une lecture simultanée n'est peut-être pas exclue.
         */
        pthread_mutex_lock(mutex);
        *running = 0;
        pthread_mutex_unlock(mutex);
        return 1;
    }

    on_keypressed(key, mode, scroll_offset, filter);
    return 0;
}


void on_keypressed(int key, t_sort_mode *mode, int *scroll_offset, char *filter)
{
    switch (key) {
        case 'p': case 'P': *mode = SORT_PID;  *scroll_offset = 0; break;
        case 'n': case 'N': *mode = SORT_NAME; *scroll_offset = 0; break;
        case 'm': case 'M': *mode = SORT_MEM;  *scroll_offset = 0; break;

        case KEY_UP:   (*scroll_offset)--; break;
        case KEY_DOWN: (*scroll_offset)++; break;

        case '/':
            /* L'utilisateur introduit un filtre :
             * Désactiver le timeout et activer l’écho pour permettre à l’utilisateur
             * de saisir une chaîne de filtre.
             * getnstr() bloque jusqu’à l’appui ==> La liste est figée dans l'attente de saisie
             * sur Entrée. Une saisie vide supprime le filtre actif.
             */
            timeout(-1);
            echo();
            curs_set(TRUE);
            getnstr(filter, 255);
            noecho();
            curs_set(FALSE);
            timeout(REFRESH_TIME);
            break;

        default: break;
    }
}

