/**
 * @file sysproc.c
 * @brief Implementation of /proc parsing and process list sorting.
 *
 * All functions declared in sysproc.h are implemented here.
 * Static helpers (comparators, lire_ram, skip_non_alpha) are internal to
 * this translation unit and not exposed in the public header.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "sysproc.h"


/* =========================================================================
 * Process list — data acquisition
 * ========================================================================= */

int compter_processus(DIR *d)
{
    int total = 0;
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL) {
        if (isdigit(entry->d_name[0]))
            total++;
    }
    /* Rewind so the same handle can be passed to remplir_liste_processus(). */
    rewinddir(d);
    return total;
}


void remplir_liste_processus(DIR *d, t_process *liste, int nb)
{
    struct dirent *entry;
    char  path[STRLG];
    char  line[STRLG];
    FILE *f;
    int   i = 0;

    while ((entry = readdir(d)) != NULL && i < nb) {

        if (!isdigit(entry->d_name[0]))
            continue;

        /* PID — the directory name itself is the numeric PID. */
        liste[i].pid = atoi(entry->d_name);

        /*
         * Process name — /proc/[PID]/comm contains a single line with the
         * executable name (up to 15 chars, kernel-truncated).  Strip the
         * trailing newline left by fgets().
         */
        snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);
        liste[i].name[0] = '\0';
        f = fopen(path, "r");
        if (f) {
            if (fgets(liste[i].name, sizeof(liste[i].name), f))
                liste[i].name[strcspn(liste[i].name, "\n")] = '\0';
            fclose(f);
        }

        /*
         * Resident memory — VmRSS in /proc/[PID]/status, in kibibytes.
         *
         * VmRSS may be absent for:
         *   - Kernel threads  : no user-space memory mapping.
         *   - Zombie processes: memory already released, not yet reaped.
         *   - Processes that exited between compter_processus() and here.
         * In all those cases mem_kb stays at 0.
         */
        snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);
        liste[i].mem_kb = 0;
        f = fopen(path, "r");
        if (f) {
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "VmRSS:", 6) == 0) {
                    sscanf(line, "VmRSS: %ld", &liste[i].mem_kb);
                    break;
                }
            }
            fclose(f);
        }

        i++;
    }
}


/* =========================================================================
 * RAM metrics
 * ========================================================================= */

/*
 * Read MemTotal and MemAvailable from /proc/meminfo.
 *
 * Parsing stops as soon as both values are found to avoid reading the entire
 * file.  Returns 1 on success, 0 if /proc/meminfo cannot be opened.
 */
static int lire_ram(unsigned long *total, unsigned long *available)
{
    FILE *f = fopen("/proc/meminfo", "r");
    char  line[256];

    *total     = 0;
    *available = 0;

    if (!f)
        return 0;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %lu kB",     total)     == 1) continue;
        if (sscanf(line, "MemAvailable: %lu kB", available) == 1) continue;
        if (*total != 0 && *available != 0)
            break;
    }

    fclose(f);
    return 1;
}


void update_ram_info(unsigned long *total, unsigned long *avail,
                     unsigned long *used, float *percent,
                     unsigned long *selfused)
{
    if (!lire_ram(total, avail)) {
        *total = *avail = *used = 0;
        *percent = 0.0f;
        return;
    }

    *used    = *total - *avail;
    *percent = (*total > 0) ? (float)(*used * 100) / *total : 0.0f;

    /*
     * ru_maxrss (getrusage RUSAGE_SELF) reports the peak RSS of this process
     * in kibibytes on Linux.  Used to display jice_htop's own footprint.
     */
    struct rusage self;
    *selfused = (getrusage(RUSAGE_SELF, &self) == 0) ? self.ru_maxrss : 0;
}


/* =========================================================================
 * Sort comparators (file-private)
 * ========================================================================= */

/*
 * Branchless three-way comparison idiom: (a > b) - (a < b).
 * Avoids signed-integer overflow that would occur with a direct subtraction
 * when values span the full int/long range.
 */

static int compare_by_pid(const void *a, const void *b)
{
    int pa = ((const t_process *)a)->pid;
    int pb = ((const t_process *)b)->pid;
    return (pa > pb) - (pa < pb);   /* ascending */
}

static int compare_by_mem(const void *a, const void *b)
{
    long ma = ((const t_process *)a)->mem_kb;
    long mb = ((const t_process *)b)->mem_kb;
    return (mb > ma) - (mb < ma);   /* descending */
}

/*
 * Return a pointer to the first alphabetic character in s.
 *
 * Linux kernel threads expose names bracketed with '[' (e.g. [kthreadd]).
 * '[' is ASCII 91, which falls between 'Z' (90) and 'a' (97).  A raw
 * strcmp() would sort them into a spurious group between upper- and
 * lower-case names.  Skipping leading non-alpha characters makes
 * [kthreadd] sort with the 'k' entries, [migration/0] with 'm', etc.
 *
 * Edge case: if the entire string is non-alphabetic, return s - 1 so that
 * strcasecmp() still receives a valid, non-empty pointer.
 */
static const char *skip_non_alpha(const char *s)
{
    while (*s && !isalpha((unsigned char)*s))
        s++;
    return (*s) ? s : s - 1;
}

static int compare_by_name(const void *a, const void *b)
{
    const t_process *pa = a;
    const t_process *pb = b;
    return strcasecmp(skip_non_alpha(pa->name), skip_non_alpha(pb->name));
}


/* =========================================================================
 * Public sort dispatcher
 * ========================================================================= */

void switch_sort(t_sort_mode sort_mode, t_process *liste, int nb)
{
    switch (sort_mode) {
        case SORT_PID:  qsort(liste, nb, sizeof(t_process), compare_by_pid);  break;
        case SORT_NAME: qsort(liste, nb, sizeof(t_process), compare_by_name); break;
        case SORT_MEM:  qsort(liste, nb, sizeof(t_process), compare_by_mem);  break;
        default:        break;
    }
}
