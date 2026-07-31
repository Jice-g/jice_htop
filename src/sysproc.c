/**
 * @file sysproc.c
 * @brief Implémentation du parsing de /proc et du tri de la liste des processus.
 *
 * Toutes les fonctions déclarées dans sysproc.h sont implémentées ici.
 * Les aides internes (comparateurs, lire_ram, skip_non_alpha) sont privées
 * à cette unité de traduction et ne sont pas exposées dans le header public.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "sysproc.h"


/* =========================================================================
 * Liste des processus — acquisition des données
 * ========================================================================= */

int count_processes(DIR *d)
{
    int total = 0;
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL) {
        if (isdigit(entry->d_name[0]))
            total++;
    }
    /* Remet le répertoire au début pour pouvoir passer le même handle à fill_process_list(). */
    rewinddir(d);
    return total;
}

/*
 *  NOTA :
 *  ------
 *  Ici, on compte les processus puis on les lit. C’est correct, et surtout sûr :
 *  on dimensionne le tableau avant de le remplir, ce qui évite tout dépassement.
 *
 *  On pourrait aussi compter les processus directement pendant la lecture,
 *  afin d’éviter un éventuel décalage entre le moment du comptage et celui
 *  de la lecture (processus très courts, zombies, forks rapides, etc.).
 *
 *  Cette optimisation réduirait le risque de mismatch entre nb et la liste
 *  réellement parcourue, mais impliquerait une logique de remplissage plus
 *  dynamique (realloc progressif ou liste chaînée temporaire).
 *
 *  Cet élément figure dans la feuille de route du projet (docs/)
 */



void fill_process_list(DIR *d, t_process *list, int nb)
{
/*
* La structure dirent fournit les informations élémentaires sur une entrée de répertoire 
* lors de l’utilisation de fonctions comme readdir().
* Essentiel à retenir : elle représente un fichier ou dossier rencontré dans un répertoire,
* avec son nom, son inode, et parfois son type.
*/
    struct dirent *entry;
    char  path[STRLG];
    char  line[STRLG];
    FILE *f;
    int   i = 0;

    while ((entry = readdir(d)) != NULL && i < nb) {

        if (!isdigit(entry->d_name[0]))
            continue;

        /* PID — le nom du répertoire est lui-même le PID numérique. */
        list[i].pid = atoi(entry->d_name);

        /*
         * Nom du processus — /proc/[PID]/comm contient une seule ligne avec
         * le nom de l’exécutable (jusqu’à 15 caractères, tronqué par le noyau).
         * On enlève le retour à la ligne laissé par fgets() en le repérant avec strcspn().
         */
        snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);
        list[i].name[0] = '\0';
        
        f = fopen(path, "r");
        
        if (f) {
            if (fgets(list[i].name, sizeof(list[i].name), f))
                list[i].name[strcspn(list[i].name, "\n")] = '\0';
            fclose(f);
        }

        /*
         * Mémoire résidente — VmRSS dans /proc/[PID]/status, en kB.
         *
         * VmRSS peut être absent pour :
         *   - Threads noyau  : pas de mémoire en espace utilisateur.
         *   - Processus zombies : mémoire déjà libérée, pas encore nettoyée.
         *   - Processus qui ont quitté entre count_processes() et ici.
         * Dans tous ces cas, mem_kb reste à 0.
         */
        snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);
        list[i].mem_kb = 0;
        f = fopen(path, "r");
        if (f) {
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "VmRSS:", 6) == 0) {
                    sscanf(line, "VmRSS: %ld", &list[i].mem_kb);
                    break;
                }
            }
            fclose(f);
        }

        i++;
    }
}


/* =========================================================================
 * Métriques RAM
 * ========================================================================= */

/*
 * Lit MemTotal et MemAvailable depuis /proc/meminfo.
 *
 * Le parsing s’arrête dès que les deux valeurs sont trouvées pour éviter
 * de lire tout le fichier. Retourne 1 en cas de succès, 0 si /proc/meminfo
 * ne peut pas être ouvert.
 */
static int read_ram_info(unsigned long *total, unsigned long *available)
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
    if (!read_ram_info(total, avail)) {
        *total = *avail = *used = 0;
        *percent = 0.0f;
        return;
    }

    *used    = *total - *avail;
    *percent = (*total > 0) ? (float)(*used * 100) / *total : 0.0f;

    /*
     * ru_maxrss (getrusage RUSAGE_SELF) indique le pic de RSS de ce processus
     * en kB sous Linux. Utilisé pour afficher l’empreinte mémoire de jice_htop.
     */
    struct rusage self;
    *selfused = (getrusage(RUSAGE_SELF, &self) == 0) ? self.ru_maxrss : 0;
}


/* =========================================================================
 * Comparateurs de tri (privés au fichier)
 * ========================================================================= */

/*
 * Idiome de comparaison à trois voies sans branche : (a > b) - (a < b).
 * Évite les dépassements d’entiers signés qui pourraient arriver avec une
 * soustraction directe lorsque les valeurs couvrent toute la plage int/long.
 * Pour éviter les dépassements j'ai choisi  void* dans le prototype de la fonction
 * et un typage const dans le corps.
 */

static int compare_by_pid(const void *a, const void *b)
{
    int pa = ((const t_process *)a)->pid;
    int pb = ((const t_process *)b)->pid;
    return (pa > pb) - (pa < pb);   /* ordre croissant */
}

static int compare_by_mem(const void *a, const void *b)
{
    long ma = ((const t_process *)a)->mem_kb;
    long mb = ((const t_process *)b)->mem_kb;
    return (mb > ma) - (mb < ma);   /* ordre décroissant */
}

/*
 * Retourne un pointeur vers le premier caractère alphabétique dans s.
 *
 * Les threads noyau ont des noms entre crochets (ex. [kthreadd]).
 * '[' est ASCII 91, entre 'Z' (90) et 'a' (97). Un strcmp brut les placerait
 * dans un groupe artificiel entre majuscules et minuscules.
 * Dans la première version, ils apparaissaient au-dessus des valeurs triés --> non-conformité.
 * Ignorer les caractères non alphabétiques au début permet de trier
 * [kthreadd] avec les entrées en 'k', [migration/0] avec celles en 'm', etc.
 *
 * Cas particulier : si toute la chaîne est non alphabétique, retourner s - 1
 * pour que strcasecmp() reçoive quand même un pointeur valide et non vide.
 * (Je n'ai jamais rencontré ce cas me semble-t-il.)
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
 * Dispatcher de tri public
 * ========================================================================= */

void switch_sort(t_sort_mode sort_mode, t_process *list, int nb)
{
    switch (sort_mode) {
        case SORT_PID:  qsort(list, nb, sizeof(t_process), compare_by_pid);  break;
        case SORT_NAME: qsort(list, nb, sizeof(t_process), compare_by_name); break;
        case SORT_MEM:  qsort(list, nb, sizeof(t_process), compare_by_mem);  break;
        default:        break;
    }
}

