#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "sysproc.h"



// -----------------------------
//     PROCESSUS
// -----------------------------


// FONCTION DE COMPTAGE :
//
int compter_processus(DIR *d) {

    int total = 0;
    struct dirent *entree;

    while ((entree = readdir(d)) != NULL) {
        if (isdigit(entree->d_name[0])) {
            total++;
        }
    }
    rewinddir(d); // Revenir au début pour re-parcourir le dossier après
    
    return total;    
}




// FONCTIONS COMPARATEURS (TRI) :
//
// Ces fonctions sont conçues pour être "encapsulées" dans la fonction de la librairie standart qsort(),
// conçue pour être universelle : elle peut trier n'importe quel type de données (entiers, structures, pointeurs, etc.).
// Pour atteindre cette flexibilité, elle ne sait pas à l'avance ce qu'elle trie.
// Elle utilise donc des pointeurs génériques : int (*compar)(const void *, const void *)
// Nous ferons de même pour éviter toute erreur ou ambiguité --> on castera les constantes passées en argument.

// Tri décroissant sur le numero pid :
int compare_by_pid(const void *a, const void *b) {
	
    int pa = ((t_process *)a)->pid;  // check risque de débordement dans les comparateurs
    int pb = ((t_process *)b)->pid;  // ---
    return (pa > pb) - (pa < pb);
}

// Tri décroissant sur la mémoire mem_kb :
int compare_by_mem(const void *a, const void *b) {

    long ma = ((t_process *)a)->mem_kb;  // check risque de débordement dans les comparateurs
    long mb = ((t_process *)b)->mem_kb;  // ...
    
    return (mb > ma) - (mb < ma); // décroissant

}

// Tri décroissant le nom du process name :
int compare_by_name(const void *a, const void *b) {
    const t_process *pa = a;
    const t_process *pb = b;
    return strcmp(pa->name, pb->name);
}



// FONCTION D'ENREGISTREMENT
//
void remplir_liste_processus(DIR *d, t_process *liste, int nb)
{
    struct dirent   *entree;
    char            chemin[STRLG];
    char            ligne[STRLG];
    FILE            *f;
    int             i;

    i = 0;
    while ((entree = readdir(d)) != NULL && i < nb)
    {
        // si c'est pas un digit on laisse tomber et on passe au suivant
        if (!isdigit(entree->d_name[0])) 
            continue;

	// PID : conversion string -> entier (le nom du fichier c'est le PID)
	liste[i].pid = atoi(entree->d_name);

	// NOM : lecture de /proc/[PID]/comm, fichier qui contient le nom du processus
	snprintf(chemin, sizeof(chemin), "/proc/%s/comm", entree->d_name);
	f = fopen(chemin, "r");
	if (f)
	{
	  // une ligne avec le nom du processus
	    if (fgets(liste[i].name, sizeof(liste[i].name), f))
		liste[i].name[strcspn(liste[i].name, "\n")] = 0; // supprime "end-of-line" si nécessaire
	    fclose(f);
	}

	// MEMOIRE : lecture de VmRSS dans /proc/[PID]/status
	// --------------------------------------------------
	// VmRSS peut être absent dans le fichier :
	// - Les processus noyau (kernel) : Leur 'status' n'a pas de ligne VmRSS car pas de processus utilisateur.
	// - Les processus "zombies" : mort mais pas encore "ramassé" par le parent. Il n'a plus de mémoire allouée.
	// - Les processus pourraient parfois avoir terminé entre le moment où on lit la liste et le moment où on ouvre le 'status'.

	snprintf(chemin, sizeof(chemin), "/proc/%s/status", entree->d_name);
	f = fopen(chemin, "r");
	
	liste[i].mem_kb = 0;  // ou initialise même si calloc() est sensé le faire
	if (f)
	{
	    while (fgets(ligne, sizeof(ligne), f))
	    { // Il y aura possiblement une ligne : "VmRSS: 15320 kB" par exemple 
		if (strncmp(ligne, "VmRSS:", 6) == 0)
		{
		    sscanf(ligne, "VmRSS: %ld", &liste[i].mem_kb); // on identifie un %ld qui suit directement et on l'enregistre
		    break;
		}
	    }
	    fclose(f);
	}
        i++;
    }
}



// -----------------------------
//     RAM
// -----------------------------

// FONCTIONS POUR LIRE INFO RAM :
///
//  RAM globale  :  Lecture de "/proc/meminfo" :
//  et puis, recherche et lecture des champs :
// - "MemTotal : "
// - "MemAvailable : "
// on écrit cette valeur en 'string' dans 'ligne' avec sscanf()
       
int lire_ram(unsigned long *total, unsigned long *available)
{
    FILE *f = fopen("/proc/meminfo", "r");
    char ligne[256];

    *total = 0;
    *available = 0;

    if (!f)
        return 0;

    while (fgets(ligne, sizeof(ligne), f))
    {      
        if (sscanf(ligne, "MemTotal: %lu kB", total) == 1)
            continue;
        if (sscanf(ligne, "MemAvailable: %lu kB", available) == 1)
            continue;
            
        if (*total != 0 && *available != 0)
        break; // on va pas continuer de lire tout le fichier, ça sert à rien
    }

    fclose(f);    
    return 1;
} 



// Mode tri des enregistrements :
///
void switch_sort(t_sort_mode sort_mode, t_process *liste, int nb) {
        
	switch (sort_mode) {
    		case SORT_PID:
        		qsort(liste, nb, sizeof(t_process), compare_by_pid);
        		break;
    	case SORT_NAME:
        		qsort(liste, nb, sizeof(t_process), compare_by_name);
        		break;
    	case SORT_MEM:
        		qsort(liste, nb, sizeof(t_process), compare_by_mem);
        		break;
        default : break;
        }
}  



// Mise à jour des données RAM : 
///
void update_ram_info(unsigned long *total, unsigned long *avail, unsigned long *used, float *percent)
{
    if (!lire_ram(total, avail)) {  // Si on a zero c'est qu'on a un probleme avec l'accès à /proc/meminfo
        *total = *avail = 0;
        *percent = 0;
        return;
    }
    *used = *total - *avail;
    *percent = (*total > 0) ? (float)(*used * 100) / *total : 0;
}



/* fin */
