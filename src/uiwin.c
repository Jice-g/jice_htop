/**
 * @file uiwin.c
 * @brief ncurses initialisation, rendering helpers, and keyboard input.
 *
 * All rendering functions assume that init_ncurses() has been called first.
 * Colour pair indices match those declared in uiwin.h.
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
    curs_set(FALSE);            /* Hide the cursor during normal display.      */
    keypad(stdscr, TRUE);       /* Enable special keys (arrows, function keys).*/
    timeout(REFRESH_TIME);      /* getch() returns ERR after REFRESH_TIME ms.  */

    start_color();
    init_pair(1, COLOR_BLACK,  COLOR_WHITE);   /* Title banner                */
    init_pair(2, COLOR_WHITE,  COLOR_BLACK);   /* Column headers              */
    init_pair(3, COLOR_GREEN,  COLOR_BLACK);   /* Process rows                */
    init_pair(4, COLOR_CYAN,   COLOR_BLACK);   /* Key-binding reminder        */
    init_pair(5, COLOR_BLACK,  COLOR_YELLOW);  /* Dynamic status bar          */
}


/* =========================================================================
 * Rendering
 * ========================================================================= */

void ram_display(const unsigned long total, const unsigned long avail,
                 const unsigned long used, const float percent,
                 const unsigned long self_use)
{
    attron(COLOR_PAIR(3));
    mvprintw(L_DATA_GLOBAL + 0, 0, "%-16s %6lu MB",
             "Mem totale:",   total / 1024);
    mvprintw(L_DATA_GLOBAL + 1, 0, "%-16s %6lu MB  %.1f%%      |    JICE_HTOP ressources : %lu kB",
             "Mem utilisee:", used  / 1024, percent, self_use);
    mvprintw(L_DATA_GLOBAL + 2, 0, "%-16s %6lu MB",
             "Mem libre:",    avail / 1024);
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


int bandeau_bas(char *dest, int sMax, t_sort_mode smode, const char *filter)
{
    const char *sort_label =
        (smode == SORT_PID)  ? "PID" :
        (smode == SORT_NAME) ? "NOM" : "MEM";

    /* Display an empty string when no filter is active. */
    const char *f = (filter && filter[0]) ? filter : "";

    snprintf(dest, sMax, "Tri: %s | Filtre: %s", sort_label, f);
    return (int)strlen(dest);
}


void calcul_scroll(int nb_affiches, int lignes_dispo, int *scroll_offset,
                   int *max_scroll, int *bar_height, int *bar_pos)
{
    *max_scroll = nb_affiches - lignes_dispo;
    if (*max_scroll < 0) *max_scroll = 0;

    /* Clamp scroll_offset to the valid range [0, max_scroll]. */
    if (*scroll_offset < 0)           *scroll_offset = 0;
    if (*scroll_offset > *max_scroll) *scroll_offset = *max_scroll;

    /*
     * bar_height equals the viewport height when the list overflows, or the
     * list length when it fits entirely — ensuring the thumb fills the track.
     */
    *bar_height = (nb_affiches > lignes_dispo) ? lignes_dispo : nb_affiches;

    /*
     * Map scroll_offset linearly to a thumb position within [0, bar_height-1].
     * Integer division is intentional; fractional precision is not needed here.
     */
    *bar_pos = (*max_scroll == 0)
               ? 0
               : (*scroll_offset * (*bar_height - 1)) / *max_scroll;
}


void draw_scrollbar(int bar_height, int bar_pos, int y0)
{
    for (int y = 0; y < bar_height; y++)
        mvprintw(y0 + y, COLS - 3, (y == bar_pos) ? "[=]" : "|||");
}


/* =========================================================================
 * Filtering
 * ========================================================================= */

int cmp_filtre(const char *strg, const char *sub)
{
    /* An empty or NULL filter matches everything. */
    if (!strg || !sub || !sub[0])
        return 1;

    /*
     * Slide a window of length strlen(sub) over strg, comparing
     * character-by-character in a case-insensitive manner.
     * Return 1 on the first matching position found.
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
 * Input handling
 * ========================================================================= */

int get_keypressed(int key, t_sort_mode *mode, int *scroll_offset,
                   char *filter, int *running, pthread_mutex_t *mutex)
{
    if (key == 'q' || key == 'Q') {
        /*
         * Signal the collector thread to stop.  The write to *running is
         * guarded by the mutex even though it is a single int assignment,
         * to maintain formal correctness of the synchronisation contract.
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
            /*
             * Suspend the render timeout and enable echo so the user can
             * type a filter string.  getnstr() blocks until Enter is pressed.
             * An empty input clears the active filter.
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
