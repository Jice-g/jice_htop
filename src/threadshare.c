/**
 * @file threadshare.c
 * @brief Lifecycle of t_shared and the background collector thread.
 *
 * The collector thread runs independently of the render loop.  It owns the
 * write side of t_shared: it is the only thread that modifies listProc, nb,
 * capacite, and the RAM metrics.  The main thread reads those fields by
 * taking a snapshot under the mutex (see main.c).
 */

#include <stdlib.h>
#include <unistd.h>
#include "threadshare.h"
#include "sysproc.h"

/** Collector sleep duration between two /proc scans, in microseconds. */
#define COLLECT_INTERVAL 200000


/* =========================================================================
 * Lifecycle
 * ========================================================================= */

void init_shared(t_shared *s)
{
    s->listProc    = NULL;
    s->nb          = 0;
    s->capacite    = 0;
    s->total_ram   = 0;
    s->avail_ram   = 0;
    s->ram_used    = 0;
    s->ram_percent = 0.0f;
    s->self_use    = 0;
    s->running     = 1;

    /*
     * pthread_mutex_init() must be called before any lock/unlock attempt.
     * NULL attributes select the default (fast, non-recursive) mutex type.
     */
    pthread_mutex_init(&s->mutex, NULL);
}


void free_shared(t_shared *s)
{
    free(s->listProc);
    s->listProc = NULL;
    pthread_mutex_destroy(&s->mutex);
}


/* =========================================================================
 * Collector thread
 * ========================================================================= */

void *thread_collecte(void *arg)
{
    t_shared *s = (t_shared *)arg;
    DIR      *d;
    int       nb;
    int       new_capa;

    while (1) {

        /*
         * Check the stop flag without acquiring the mutex.
         * s->running is written exactly once by the main thread (set to 0
         * when the user presses 'q').  On x86/x86-64 a single aligned int
         * read is atomic at the hardware level, making this safe in practice.
         * A portable solution would use atomic_int or a second mutex.
         */
        if (!s->running)
            break;

        d = opendir("/proc");
        if (!d) {
            usleep(COLLECT_INTERVAL);
            continue;
        }

        nb = compter_processus(d);

        /* -----------------------------------------------------------------
         * Critical section — write shared data
         * ----------------------------------------------------------------- */
        pthread_mutex_lock(&s->mutex);

        if (nb > s->capacite) {
            /*
             * Grow with a headroom of 20 slots to reduce future reallocations
             * when the process count fluctuates near the current capacity.
             */
            new_capa = nb + 20;
            t_process *tmp = realloc(s->listProc, new_capa * sizeof(t_process));
            if (tmp) {
                s->listProc = tmp;
                s->capacite = new_capa;
            } else {
                /*
                 * realloc() failed: release the mutex and skip this cycle.
                 * The render loop keeps displaying the previous list until
                 * the next successful collection.
                 */
                pthread_mutex_unlock(&s->mutex);
                closedir(d);
                usleep(COLLECT_INTERVAL);
                continue;
            }
        }

        remplir_liste_processus(d, s->listProc, nb);
        s->nb = nb;
        update_ram_info(&s->total_ram, &s->avail_ram,
                        &s->ram_used, &s->ram_percent, &s->self_use);

        pthread_mutex_unlock(&s->mutex);
        /* End of critical section */

        /*
         * closedir() does not touch shared data, so it runs outside the
         * critical section to minimise lock hold time.
         */
        closedir(d);
        usleep(COLLECT_INTERVAL);
    }

    /* pthread convention: return NULL to signal clean termination. */
    return NULL;
}
