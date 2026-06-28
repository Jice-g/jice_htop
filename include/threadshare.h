/**
 * @file threadshare.h
 * @brief Shared data structure and collector thread interface.
 *
 * t_shared is the single point of truth exchanged between the background
 * collector thread (thread_collector) and the main render loop.
 * All accesses to mutable fields must be guarded by the embedded mutex.
 */

#ifndef THREADSHARE_H
#define THREADSHARE_H

#include <pthread.h>
#include "sysproc.h"


/**
 * @brief Data shared between the collector thread and the render loop.
 *
 * The collector thread writes to this structure; the render loop reads from
 * it by taking a private snapshot under the mutex (see main.c).
 *
 * Ownership rules:
 *   - @c proc_list is heap-allocated by the collector and freed by free_shared().
 *   - @c running is the only field written by the main thread; it signals the
 *     collector to exit cleanly.
 */
typedef struct s_shared
{
    t_process       *proc_list;     /**< Heap-allocated process array (capacity: @c capacity) */
    int              nb;            /**< Number of valid entries in @c proc_list               */
    int              capacity;      /**< Allocated capacity of @c proc_list, in entries        */
    unsigned long    total_ram;     /**< Total installed RAM, in kB                    */
    unsigned long    avail_ram;     /**< Available RAM, in kB                          */
    unsigned long    ram_used;      /**< Used RAM (total - avail), in kB               */
    float            ram_percent;   /**< Used RAM as a percentage of total                    */
    unsigned long    self_use;      /**< Peak RSS of this process, in kB               */
    int              running;       /**< Collector loop control: 1 = run, 0 = stop            */
    pthread_mutex_t  mutex;         /**< Guards all fields above                              */
} t_shared;


/* ---------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

/**
 * @brief Initialise all fields of @p s to safe defaults and create the mutex.
 *
 * Must be called before spawning the collector thread.
 *
 * @param s  Pointer to the t_shared instance to initialise.  Must not be NULL.
 */
void init_shared(t_shared *s);

/**
 * @brief Release resources held by @p s (process array and mutex).
 *
 * Must be called only after the collector thread has been joined.
 *
 * @param s  Pointer to the t_shared instance to destroy.  Must not be NULL.
 */
void free_shared(t_shared *s);


/* ---------------------------------------------------------------------------
 * Collector thread
 * ------------------------------------------------------------------------- */

/**
 * @brief Entry point of the background collector thread.
 *
 * Runs in a loop at COLLECT_INTERVAL-microsecond intervals:
 *   1. Checks s->running; exits if 0.
 *   2. Opens /proc and counts live processes.
 *   3. Acquires the mutex and updates s->proc_list, s->nb, and RAM metrics.
 *   4. Releases the mutex and closes /proc.
 *
 * On realloc() failure the current cycle is skipped; the previous data
 * remains visible to the render loop until the next successful cycle.
 *
 * @param arg  Pointer to the t_shared instance (cast from void *).
 * @return     Always NULL (pthread convention for "terminated without error").
 */
void *collector_thread(void *arg);


#endif /* THREADSHARE_H */
