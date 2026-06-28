/**
 * @file sysproc.h
 * @brief Process and memory data structures, /proc parsing interface.
 *
 * Public API for reading process information and RAM metrics from the Linux
 * /proc virtual filesystem, and for sorting the resulting process list.
 */

#ifndef SYSPROC_H
#define SYSPROC_H

#include <dirent.h>
#include <sys/resource.h>


/** Maximum length of a line buffer used when parsing /proc entries. */
#define STRLG 512


/**
 * @brief Snapshot of a single process at one point in time.
 *
 * Populated by remplir_liste_processus() from /proc/[PID]/comm
 * and /proc/[PID]/status (VmRSS field).
 */
typedef struct s_process
{
    int     pid;        /**< Numeric process identifier                      */
    char    name[256];  /**< Process name read from /proc/[PID]/comm         */
    long    mem_kb;     /**< Resident set size in kibibytes (VmRSS), or 0
                             if unavailable (kernel thread, zombie, etc.)    */
}   t_process;


/**
 * @brief Sort criterion for the process list.
 *
 * The active mode is toggled at runtime by the user (keys p / n / m)
 * and passed to switch_sort().
 */
typedef enum e_sort_mode {
    SORT_PID,   /**< Sort by PID, ascending                                  */
    SORT_NAME,  /**< Sort by process name, case-insensitive, ascending       */
    SORT_MEM    /**< Sort by resident memory usage, descending               */
} t_sort_mode;


/* ---------------------------------------------------------------------------
 * Process list — data acquisition
 * ------------------------------------------------------------------------- */

/**
 * @brief Count the number of numeric entries in an open /proc directory.
 *
 * Each numeric entry corresponds to one running process.
 * The directory is rewound to its start before returning so that the same
 * DIR handle can be passed immediately to remplir_liste_processus().
 *
 * @param d  Open DIR handle for /proc.  Must not be NULL.
 * @return   Number of process entries found.
 */
int compter_processus(DIR *d);

/**
 * @brief Populate a process array from an open /proc directory.
 *
 * Iterates over numeric entries in @p d and fills up to @p nb slots in
 * @p liste with PID, name, and resident memory (VmRSS).  Entries whose
 * /proc/[PID]/comm or /proc/[PID]/status files disappear mid-scan (e.g.
 * short-lived processes) are silently skipped.
 *
 * @param d      Open DIR handle for /proc.
 * @param liste  Destination array; must hold at least @p nb elements.
 * @param nb     Maximum number of entries to write.
 */
void remplir_liste_processus(DIR *d, t_process *liste, int nb);

/**
 * @brief Refresh RAM usage metrics from /proc/meminfo and getrusage().
 *
 * All output pointers are mandatory; none may be NULL.
 *
 * @param[out] total     Total installed RAM, in kibibytes.
 * @param[out] avail     Available RAM (MemAvailable), in kibibytes.
 * @param[out] used      Used RAM computed as (total - avail), in kibibytes.
 * @param[out] percent   Used RAM as a percentage of total (0.0 if total == 0).
 * @param[out] selfused  Peak resident set size of this process, in kibibytes.
 */
void update_ram_info(unsigned long *total, unsigned long *avail,
                     unsigned long *used, float *percent,
                     unsigned long *selfused);


/* ---------------------------------------------------------------------------
 * Process list — sorting
 * ------------------------------------------------------------------------- */

/**
 * @brief Sort @p liste in-place according to @p sort_mode.
 *
 * Delegates to qsort() with the appropriate comparator:
 *   - SORT_PID  : ascending by PID
 *   - SORT_NAME : case-insensitive ascending, leading non-alpha chars ignored
 *   - SORT_MEM  : descending by resident memory
 *
 * @param sort_mode  Requested sort criterion.
 * @param liste      Array to sort; may be NULL if @p nb is 0.
 * @param nb         Number of elements in @p liste.
 */
void switch_sort(t_sort_mode sort_mode, t_process *liste, int nb);


#endif /* SYSPROC_H */
