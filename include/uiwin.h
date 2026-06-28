/**
 * @file uiwin.h
 * @brief ncurses UI — initialisation, rendering helpers, and input handling.
 *
 * Layout constants define a fixed row structure inside the terminal window:
 *
 *   Row 0                  : title banner (process count)
 *   Rows L_DATA_GLOBAL … +2: RAM metrics (3 rows)
 *   Rows L_TAB_PROCESS … +2: column header (3 rows)
 *   Rows L_LIST_PROCESS …  : scrollable process list
 *   Row LINES-2            : key-binding reminder
 *   Row LINES-1            : dynamic status bar (sort mode + active filter)
 */

#ifndef UIWIN_H
#define UIWIN_H

#include "sysproc.h"
#include <pthread.h>


/** ncurses getch() timeout in milliseconds; controls the UI refresh rate. */
#define REFRESH_TIME    200

/** First row of the RAM metrics block (immediately below the title banner). */
#define L_DATA_GLOBAL   1

/** First row of the column header block (3 rows below L_DATA_GLOBAL). */
#define L_TAB_PROCESS   (L_DATA_GLOBAL + 3)

/** First row of the scrollable process list (3 rows below L_TAB_PROCESS). */
#define L_LIST_PROCESS  (L_TAB_PROCESS + 3)


/* ---------------------------------------------------------------------------
 * Initialisation
 * ------------------------------------------------------------------------- */

/**
 * @brief Initialise ncurses and configure the five colour pairs used by the UI.
 *
 * Colour pair index map:
 *   1 — black on white  (title banner)
 *   2 — white on black, bold (column headers)
 *   3 — green on black  (process rows)
 *   4 — cyan  on black  (key-binding reminder)
 *   5 — black on yellow (dynamic status bar)
 */
void init_ncurses(void);


/* ---------------------------------------------------------------------------
 * Rendering
 * ------------------------------------------------------------------------- */

/**
 * @brief Render the RAM metrics block at rows L_DATA_GLOBAL … +2.
 *
 * @param total     Total installed RAM, in kB.
 * @param avail     Available RAM, in kB.
 * @param used      Used RAM, in kB.
 * @param percent   Used RAM as a percentage of total.
 * @param self_use  Peak RSS of this process, in kB.
 */
void ram_display(const unsigned long total, const unsigned long avail,
                 const unsigned long used, const float percent,
                 const unsigned long self_use);

/**
 * @brief Render the column header block at rows L_TAB_PROCESS … +2.
 */
void draw_header(void);

/**
 * @brief Format the dynamic status bar string into @p dest.
 *
 * Writes "Tri: <mode> | Filtre: <filter>" into @p dest, NUL-terminated and
 * truncated to @p sMax bytes.
 *
 * @param dest    Destination buffer.
 * @param sMax    Size of @p dest in bytes (typically COLS).
 * @param smode   Active sort mode.
 * @param filter  Active filter string, or an empty string if none.
 * @return        Length of the formatted string (as returned by snprintf).
 */
int low_status_bar(char *dest, int sMax, t_sort_mode smode, const char *filter);

/**
 * @brief Compute scrollbar geometry from the current list and viewport state.
 *
 * Clamps @p scroll_offset to [0, max_scroll] and derives bar_height and
 * bar_pos so that the scrollbar reflects the visible portion of the list.
 *
 * @param nb_visible    Total number of rows matching the active filter.
 * @param avail_lines   Number of rows available in the process panel.
 * @param[in,out] scroll_offset  Current scroll offset; clamped in place.
 * @param[out]    max_scroll     Maximum reachable scroll offset.
 * @param[out]    bar_height     Height of the scrollbar track, in rows.
 * @param[out]    bar_pos        Position of the scrollbar thumb, in rows.
 */
void compute_scroll(int nb_visible, int avail_lines, int *scroll_offset,
                   int *max_scroll, int *bar_height, int *bar_pos);

/**
 * @brief Render the scrollbar at the right edge of the process panel.
 *
 * @param bar_height  Height of the scrollbar track, in rows.
 * @param bar_pos     Row index of the scrollbar thumb within the track.
 * @param y0          Absolute row of the first track cell (= L_LIST_PROCESS).
 */
void draw_scrollbar(int bar_height, int bar_pos, int y0);


/* ---------------------------------------------------------------------------
 * Filtering
 * ------------------------------------------------------------------------- */

/**
 * @brief Test whether @p sub appears as a substring of @p strg.
 *
 * The comparison is case-insensitive.  An empty @p sub always matches.
 *
 * @param strg  String to search within (process name).
 * @param sub   Substring to look for (user-supplied filter).
 * @return      1 if @p sub is found in @p strg, 0 otherwise.
 */
int match_filter(const char *strg, const char *sub);


/* ---------------------------------------------------------------------------
 * Input handling
 * ------------------------------------------------------------------------- */

/**
 * @brief Handle a keypress and signal exit when 'q' / 'Q' is pressed.
 *
 * If the key is 'q' or 'Q', acquires @p mutex, sets *running to 0, releases
 * the mutex, and returns 1 to instruct the caller to break out of the render
 * loop.  For all other keys, delegates to on_keypressed() and returns 0.
 *
 * This function is intentionally called outside critical sections; only
 * @p running is touched under the mutex.
 *
 * @param key           Key code returned by getch().
 * @param mode          Pointer to the active sort mode.
 * @param scroll_offset Pointer to the current scroll offset.
 * @param filter        Filter buffer (up to 255 chars + NUL).
 * @param running       Pointer to the shared running flag.
 * @param mutex         Mutex that guards @p running.
 * @return              1 if the render loop should exit, 0 otherwise.
 */
int get_keypressed(int key, t_sort_mode *mode, int *scroll_offset,
                   char *filter, int *running, pthread_mutex_t *mutex);

/**
 * @brief Apply the effect of a non-exit keypress to the UI state.
 *
 * Key bindings:
 *   'p' / 'P' : switch to SORT_PID  and reset scroll
 *   'n' / 'N' : switch to SORT_NAME and reset scroll
 *   'm' / 'M' : switch to SORT_MEM  and reset scroll
 *   KEY_UP    : scroll up one row
 *   KEY_DOWN  : scroll down one row
 *   '/'       : prompt for a filter string (blocks until Enter)
 *
 * @param key           Key code returned by getch().
 * @param mode          Pointer to the active sort mode.
 * @param scroll_offset Pointer to the current scroll offset.
 * @param filter        Filter buffer (up to 255 chars + NUL).
 */
void on_keypressed(int key, t_sort_mode *mode, int *scroll_offset, char *filter);


#endif /* UIWIN_H */
