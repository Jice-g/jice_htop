
#ifndef SYSPROC_H
#define SYSPROC_H

#include <dirent.h>
#include <sys/resource.h>


// Définition de la longueur maximale d'une ligne dans notre liste
#define STRLG 512

// Structure processus
// ---------------------------------------------------------------
//  Le cahier des charges demande de pouvoir trier les processus
//  par PID / par utilisation mémoire / par nom

typedef struct s_process
{
    int     pid;
    char    name[256];
    long    mem_kb;
}   t_process;

// il faudra pouvoir switcher selon le choix de l'utilisateur final : getch()
// Switch selon :
typedef enum e_sort_mode {
    	SORT_PID,
    	SORT_NAME,
    	SORT_MEM
     }   t_sort_mode;
     
     
     
// Fonctions processus:
// ----------------------------------------------------------
// Lecture des données
int compter_processus(DIR *d);
void remplir_liste_processus(DIR *d, t_process *liste, int nb);
void update_ram_info(unsigned long *total, unsigned long *avail, unsigned long *used, float *percent, unsigned long *selfused);

// Tri des données :
void switch_sort(t_sort_mode sort_mode, t_process *liste, int nb) ;

#endif

