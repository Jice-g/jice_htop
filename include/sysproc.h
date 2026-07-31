/**
 * @file sysproc.h
 * @brief Structures de données pour les processus et la mémoire, interface de parsing de /proc.
 *
 * Lit les informations de processus et les métriques de RAM
 * depuis le système de fichiers virtuel /proc de Linux, et pour trier la liste
 * de processus obtenue.
 */

#ifndef SYSPROC_H
#define SYSPROC_H

#include <dirent.h>
#include <sys/resource.h>


/** Longueur maximale d’un tampon de ligne utilisé lors du parsing des entrées /proc. */
#define STRLG 512


/**
 * @brief Instantané d’un processus à un moment donné.
 *
 * Rempli par fill_process_list() à partir de /proc/[PID]/comm
 * et /proc/[PID]/status (champ VmRSS).
 */
typedef struct s_process
{
    int     pid;        /**< Identifiant numérique du processus                       */
    char    name[256];  /**< Nom du processus lu depuis /proc/[PID]/comm              */
    long    mem_kb;     /**< Taille RSS en kB (VmRSS), ou 0 si non disponible (thread noyau, zombie, etc.) */
}   t_process;


/**
 * @brief Critère de tri pour la liste des processus.
 *
 * Le mode actif est changé à l’exécution par l’utilisateur :
 * touches p / n / m ou en P / N / M, ce n'est pas case-sensitive
 * et transmis à switch_sort(). Voir la fonction de uiwin : On_keyPressed()
 */
typedef enum e_sort_mode {
    SORT_PID,   /**< Tri par PID, ordre croissant                                 */
    SORT_NAME,  /**< Tri par nom de processus, insensible à la casse, croissant   */
    SORT_MEM    /**< Tri par mémoire résidente, ordre décroissant                 */
} t_sort_mode;


/* ---------------------------------------------------------------------------
 * Liste des processus — acquisition des données
 * ------------------------------------------------------------------------- */

/**
 * @brief Compte le nombre d’entrées numériques dans un répertoire /proc ouvert.
 *
 * Chaque entrée numérique correspond à un processus en cours d’exécution.
 * Le pointeur sur répertoire (handle) DIR* d est "remis au début" avant le retour de fonction (rewind(d)),
 * afin que le même * handle DIR puisse être passé directement et sans décalage à fill_process_list().
 *
 * @param d  Handle DIR ouvert pour /proc. Ne doit pas être NULL.
 * @return   Nombre d’entrées de processus trouvées.
 */
int count_processes(DIR *d);


/**
 * @brief Remplit un tableau de processus à partir d’un répertoire /proc ouvert.
 *
 * Parcourt les entrées numérique (isdigit) dans @p d
 * et remplit jusqu’à @p nb cases dans @p list avec le PID, le nom et la mémoire résidente (VmRSS).
 *
 * Les entrées dont les fichiers /proc/[PID]/comm ou /proc/[PID]/status
 * disparaissent pendant le scan (processus très courts) sont ignorées silencieusement.
 *
 * @param d      Handle DIR ouvert pour /proc.
 * @param list   Tableau de destination ; doit contenir au moins @p nb éléments.
 * @param nb     Nombre maximal d’entrées à écrire.
 */
void fill_process_list(DIR *d, t_process *list, int nb);


/**
 * @brief Met à jour les métriques d’utilisation de la RAM depuis /proc/meminfo et getrusage().
 *
 * Tous les pointeurs de sortie sont obligatoires ; aucun ne peut être NULL.
 *
 * @param[out] total     RAM totale installée, en kB.
 * @param[out] avail     RAM disponible (MemAvailable), en kB.
 * @param[out] used      RAM utilisée calculée comme (total - avail), en kB.
 * @param[out] percent   Pourcentage de RAM utilisée (0.0 si total == 0).
 * @param[out] selfused  Pic de RSS de ce processus, en kB.
 */
void update_ram_info(unsigned long *total, unsigned long *avail,
                     unsigned long *used, float *percent,
                     unsigned long *selfused);


/* ---------------------------------------------------------------------------
 * Liste des processus — tri
 * ------------------------------------------------------------------------- */

/**
 * @brief Trie @p list en place selon @p sort_mode.
 *
 * Utilise qsort() avec le comparateur approprié :
 *   - SORT_PID  : tri croissant par PID
 *   - SORT_NAME : tri croissant insensible à la casse, caractères non alphabétiques ignorés
 *   - SORT_MEM  : tri décroissant par mémoire résidente
 *
 * @param sort_mode  Critère de tri demandé.
 * @param list       Tableau à trier ; peut être NULL si @p nb vaut 0.
 * @param nb         Nombre d’éléments dans @p list.
 */
void switch_sort(t_sort_mode sort_mode, t_process *list, int nb);


#endif /* SYSPROC_H */

