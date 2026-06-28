/**
 * @file main.c
 * @brief Entry point of jice_htop — a lightweight interactive process monitor.
 *
 * Spawns a background collector thread that periodically reads /proc and
 * updates shared system data.  The main thread runs the ncurses render loop:
 * it snapshots the shared data under a mutex, sorts and filters the process
 * list, renders the UI, then waits for a keyboard event before repeating.
 *
 * Architecture overview:
 *   - threadshare : shared data structure + collector thread lifecycle
 *   - sysproc     : /proc parsing, RAM metrics, sort comparators
 *   - uiwin       : ncurses initialisation, rendering helpers, input handling
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
     * User interaction state
     * ------------------------------------------------------------------------- */
    int         key;
    char        filter[256];    /* Substring filter typed by the user via '/'   */
    filter[0]   = '\0';
    t_sort_mode sort_mode = SORT_PID;
    int         err_flag  = 0;

    /* -------------------------------------------------------------------------
     * UI layout and scroll state
     * Each variable is reset at the top of every render iteration.
     * ------------------------------------------------------------------------- */
    char status_bar[512];       /* Formatted status bar string (bottom)          */
    int  lines_written  = 0;    /* Rows actually rendered in the process panel   */
    int  nb_displayed   = 0;    /* Rows matching the active filter               */
    int  lines_avail    = 0;    /* Rows available between top and bottom banners */
    int  bar_pos        = 0;    /* Scrollbar cursor position                     */
    int  scroll_offset  = 0;    /* Index of the first visible process row        */
    int  max_scroll     = 0;    /* Maximum reachable scroll offset               */
    int  bar_height     = 0;    /* Scrollbar height in rows                      */

    /* -------------------------------------------------------------------------
     * Shared data (collector thread <-> render loop)
     * ------------------------------------------------------------------------- */
    t_shared  shared;
    pthread_t th_collecte;

    /*
     * Snapshot buffer — the render loop works on a private copy of the shared
     * process list to keep the critical section as short as possible.
     * Initial capacity is 256 entries; realloc() extends it on demand.
     */
    t_process    *snap_list      = NULL;
    int           snap_count         = 0;
    int           snap_capacity   = 0;
    unsigned long snap_total_ram  = 0;
    unsigned long snap_avail_ram  = 0;
    unsigned long snap_ram_used   = 0;
    unsigned long snap_self_use   = 0;
    float         snap_ram_percent = 0.0f;

    snap_list = calloc(256, sizeof(t_process));
    if (!snap_list) {
        perror("calloc snap_list");
        return 1;
    }
    snap_capacity = 256;


    /* =========================================================================
     * Initialization
     * ========================================================================= */

    init_shared(&shared);

    if (pthread_create(&th_collecte, NULL, collector_thread, &shared) != 0) {
        perror("pthread_create");
        free_shared(&shared);
        free(snap_list);
        return 1;
    }

    init_ncurses();


    /* =========================================================================
     * Main render loop
     * ========================================================================= */

    do {
        /* Reset per-frame layout variables */
        lines_written = 0;
        lines_avail   = 0;
        bar_pos       = 0;
        bar_height    = 0;
        max_scroll    = 0;


        /* ---------------------------------------------------------------------
         * Critical section — snapshot shared data
         *
         * The lock is held only for the duration of the copy so the collector
         * thread is not blocked during rendering.  realloc() is performed
         * inside the lock because snap_list must match snap_count before memcpy.
         * --------------------------------------------------------------------- */
 
        pthread_mutex_lock(&shared.mutex);

        snap_count          = shared.nb;
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
                perror("realloc snap_list");
                err_flag = 1;
                break;
            }
        }

        if (snap_count > 0 && shared.proc_list)
            memcpy(snap_list, shared.proc_list, snap_count * sizeof(t_process));
            
        pthread_mutex_unlock(&shared.mutex);
        /* End of critical section */


        /* ---------------------------------------------------------------------
         * Sort the snapshot according to the current user-selected mode
         * --------------------------------------------------------------------- */
        switch_sort(sort_mode, snap_list, snap_count);


        /* ---------------------------------------------------------------------
         * Render — top banner
         * --------------------------------------------------------------------- */
        clear();

        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(0, 0, "JICE-HTOP | Processus : %d                      ", snap_count);
        attroff(COLOR_PAIR(1) | A_BOLD);

        ram_display(snap_total_ram, snap_avail_ram,
                    snap_ram_used, snap_ram_percent, snap_self_use);
        draw_header();


        /* ---------------------------------------------------------------------
         * Render — process list with optional filter
         * --------------------------------------------------------------------- */
        attron(COLOR_PAIR(3));

        /* Count rows that pass the current filter before rendering */
        nb_displayed = 0;
        for (int k = 0; k < snap_count; k++) {
            if (filter[0] == '\0' || match_filter(snap_list[k].name, filter))
                nb_displayed++;
        }

        if (nb_displayed == 0) {
            /* Clear the process panel and display a "no match" message */
            for (int y = 0; y < LINES - (L_LIST_PROCESS + 2); y++)
                mvprintw(L_LIST_PROCESS + y, 1, "                                        ");

            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(L_LIST_PROCESS, 1, "Aucun processus ne correspond au filtre.");
            attroff(COLOR_PAIR(3) | A_BOLD);

            refresh();
            key = getch();
            if (get_keypressed(key, &sort_mode, &scroll_offset,
                               filter, &shared.running, &shared.mutex))
                break;

            continue;
        }

        /* Compute scrollbar geometry, then render visible rows */
        lines_avail = LINES - (L_LIST_PROCESS + 2);
        compute_scroll(nb_displayed, lines_avail,
                      &scroll_offset, &max_scroll, &bar_height, &bar_pos);
        draw_scrollbar(bar_height, bar_pos, L_LIST_PROCESS);

        int i = 0;   /* Index relative to the filtered list                */
        int j = 0;   /* Absolute index into snap_list                     */

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
         * Render — bottom banners
         *   Line LINES-2 : static key-binding reminder
         *   Line LINES-1 : dynamic status (active sort mode + filter)
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
         * Input — flush display then wait up to REFRESH_TIME ms for a keypress
         * --------------------------------------------------------------------- */
        refresh();

        key = getch();
        if (get_keypressed(key, &sort_mode, &scroll_offset,
                           filter, &shared.running, &shared.mutex))
            break;

    } while (1);


    /* =========================================================================
     * Teardown — order matters:
     *   1. Join the collector thread (it checks shared.running to exit).
     *   2. Release shared memory and mutex.
     *   3. Free the snapshot buffer.
     *   4. Restore the terminal via endwin().
     * ========================================================================= */
    pthread_join(th_collecte, NULL);
    free_shared(&shared);
    free(snap_list);
    endwin();

    return err_flag;
}
