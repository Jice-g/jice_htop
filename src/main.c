
/********************************************
    Fichier /src/main.c
*********************************************/

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




/***************************************************************************************
****************************************************************************************

	MAIN de jice_htop

****************************************************************************************
****************************************************************************************

** DESCRIPTION :
----------------
  On fonctionnera avec un t_process (type déclaré en header)
  Dans le répertoire /proc avec un pointeur DIR
  On controle la capacité du t_process pour realloc dynamique si nécessaire

  Initialisation de Ncurses basique :
  Avec un rafraichissement en timeout qui se fera toutes les REFRESH_TIME ms voir rubrique des #define

  Tri : Le cahier des charges demande de pouvoir trier les processus par PID / par utilisation mémoire / par nom
  Le programme prévoit de switcher selon le choix de l'utilisateur final : getch()
  Switch selon un typedef enum de  valeurs fixes (sort_mode (SORT_PIB par défaut)

  La boucle do-while qui peut être ensuite paraléllisée en multitache prévoit gère l'affichage des données en temps réel.

  Fermeture propre du programme

=========================================================================================================================*/

	    
	    

int main(void) {


/*********************************************************************************************************
	TABLE DES VARIABLES
**********************************************************************************************************/
    
 /* Interactions untilisateur :
    ------------------------------				*/
    int key;  			// touche tapée par l'utilisateur
    char filter[256]; 		// il servira à saisir un filtre par l'utilisateur
    filter[0] = '\0';    
    t_sort_mode sort_mode = SORT_PID; // sysproc.h    
    int errFlag = 0;  
    
 
 /* UI, Filtre et scroll :
    -------------------------- 					*/
    char bandeau[512];		// Bandeau d'affichage
    int lignes_ecrites = 0; 	// le nombre de lignes effectivement écrites dans le panneau en mode normal
    int nb_affiches = 0;	// ligne affichées quand on filtre
    int lignes_dispo = 0;   	// le nombre de lignes totales disponibles dans le panneau entre les bandeau du haut et du bas
    int bar_pos = 0;		// position du curseur scroll-bar
    int scroll_offset = 0; 	// décalage de la lecture des enregistrements t_process par le scroll Page_Up ou Page_Down
    int max_scroll = 0;  	// combien on peut scroller au maximum : quand on est au bout de la liste et de lignes_dispo
    int bar_height = 0;		// hauteur scroll_bar 


 /* structure d'enregistrement :
    -------------------------------				*/
    t_shared   shared;
    pthread_t  th_collecte;
    
// snap : buffer de données
    t_process *snap_liste = NULL;
    int snap_nb    = 0;
    int snap_capacite = 0;
    unsigned long snap_total_ram = 0, snap_avail_ram = 0,
              snap_ram_used = 0, snap_self_use = 0;
    float snap_ram_percent = 0.0f;
    
    
/* Allocation mémoire du buffer snap :
   -----------------------------------				*/
	snap_liste = calloc(256, sizeof(t_process));
	snap_capacite = 256; // mesure pour realloc éventuel
	if (!snap_liste) {
    		perror("calloc snap_liste");
    	return 1;
	}	



/*********************************************************************************************************
	INITIALISATION
**********************************************************************************************************/


/* Initialisation de la structure partagée et création du thread collecteur
   ----------------------------------------------------------------------------		*/
    
    init_shared(&shared);  // contient 'pthread_mutex_init(&s->mutex, NULL);'

    // Création et vérification du thread
    if (pthread_create(&th_collecte, NULL, thread_collecte, &shared) != 0)
    {
    	perror("pthread_create");
    	free_shared(&shared);
    	free(snap_liste);
    	return 1;
    }


/* Initialisation de Ncurses
   ----------------------------------------------------------------------------		*/
    
    init_ncurses(); // uiwi.c .h 






/*********************************************************************************************************
	BOUCLE DO {  } WHILE ()
**********************************************************************************************************/
	

    do {
    
	lignes_ecrites = 0;
	lignes_dispo = 0;
	bar_pos = 0;
	bar_height = 0;
	max_scroll = 0;

	
   //======================================================================
   //	GESTION DE L'AFFICHAGE	
   //======================================================================
    
    
    
    //!!!!!!!!!!!!!!!  SECTION CRITIQUE MUTEX !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
        pthread_mutex_lock(&shared.mutex);	

	// Copie des données dans le buffer :
    	snap_nb          = shared.nb;
    	snap_total_ram   = shared.total_ram;
    	snap_avail_ram   = shared.avail_ram;
    	snap_ram_used    = shared.ram_used;
    	snap_ram_percent = shared.ram_percent;  
    	snap_self_use    = shared.self_use;
    	 	

    //   Avant la copie du tableau de la liste des processus vérifier sa capacité et l'adapter si nécessaire 	
    	if (snap_nb > snap_capacite)
    	{
    	  snap_capacite = snap_nb + 20;
    	  t_process *tmp = realloc(snap_liste, snap_capacite*sizeof(t_process));
    	  
    	 if (tmp)     	   
    	   snap_liste = tmp;
   	  else {
   	     pthread_mutex_unlock(&shared.mutex); // déverouiller avant de sortir en erreur
   	     perror("realloc snap_liste");
   	     errFlag = 1;
    	     break;
    	   }
    	 }
    	
    // OK : Copie du tableau de processus
    	if (snap_nb > 0 && shared.listProc)
        	memcpy(snap_liste, shared.listProc, snap_nb * sizeof(t_process));

	
        pthread_mutex_unlock(&shared.mutex);     
        
    //!!!!!!!!!!!   FIN SECTION CRITIQUE MUTEX !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!         
  
 
   
    /* Tri des affichages en focntion du mode choisi par l'utilisateur :
    -------------------------------------------------------------------			*/
 	switch_sort(sort_mode, snap_liste, snap_nb);  // uiwin.c .h
 	  	
         
    /* Nettoyage de tout l'affichage (ncurses)
    -------------------------------------------------------------------			*/
    	clear();


    /* Affichage du bandeau supérieur :
    -------------------------------------------------------------------			*/        
        
        // Titre : bandeau supérieur
        attron(COLOR_PAIR(1) | A_BOLD); // Couleur noir sur fond bleu 
        mvprintw(0, 0, "JICE-HTOP | Processus : %d                      ", snap_nb);
        attroff(COLOR_PAIR(1) | A_BOLD);        
        
	// Affichage des données avec notre buffer
	ram_display(snap_total_ram, snap_avail_ram, snap_ram_used, snap_ram_percent, snap_self_use);	// uiwin.c .h
	
        // Affichage de l'entête 	
        draw_header();	//	uiwin.c .h

        
        
    /* Affichage des lignes processus en gérant l'éventuel filtrage par l'utilisateur :
       ---------------------------------------------------------------------------------  */
	attron(COLOR_PAIR(3));  ; // Couleur verte sur fond noir 	
	
	nb_affiches = 0;
	for (int k = 0; k < snap_nb; k++) {
	    if (filter[0] == '\0' || cmp_filtre(snap_liste[k].name, filter))
		nb_affiches++;
	}	
	
	if (nb_affiches == 0) {
	
	    // Effacer la zone d’affichage des processus
	    for (int y = 0; y < LINES - (L_LIST_PROCESS + 2); y++) {
		mvprintw(L_LIST_PROCESS + y, 1,
		"                                        ");
	    }
	    // indiquer qu'on a pas trouvé de correspondance
	    attron(COLOR_PAIR(3) | A_BOLD);
	    mvprintw(L_LIST_PROCESS, 1, "Aucun processus ne correspond au filtre.");
	    attroff(COLOR_PAIR(3) | A_BOLD);

	    // Pas de scroll, pas de liste, pas de scroll-bar
	    // Tout ce qui suit dans le main() n'a pas lieu d'être
	    refresh();
	    key = getch();
	    if (get_keypressed(key, &sort_mode, &scroll_offset, filter, &shared.running, &shared.mutex))
    		break;	        
	    
	    // on_keypressed(key, &sort_mode, &scroll_offset, filter);
	    continue;   // Retour au début de la boucle
	}

	
 
       /* Affichage et filtre avec scroll :	
       ---------------------------------------------------------------------------------  */
       
        lignes_dispo = LINES - (L_LIST_PROCESS + 2);
        
	calcul_scroll(nb_affiches, lignes_dispo, &scroll_offset, &max_scroll, &bar_height, &bar_pos);	// uiwin.c .h	
	
	draw_scrollbar(bar_height, bar_pos, L_LIST_PROCESS);

	int i = 0;
	int j = 0;

	while ((i + scroll_offset < snap_nb) && (lignes_ecrites < lignes_dispo)) {

	    j = i + scroll_offset;

	    // filtrage
	    
	    if ( filter[0] != '\0' && !cmp_filtre(snap_liste[j].name, filter) ) {  // cmp_filtre uiwin.c .h
	     // un filtre et pas de correspondance
		i++;            // avancer dans la liste
		continue;       // mais ne pas afficher
	    	}

	    mvprintw(lignes_ecrites + L_LIST_PROCESS, 1, "%-5d %9ld    %s",
		     snap_liste[j].pid,
		     snap_liste[j].mem_kb,
		     snap_liste[j].name);
	    lignes_ecrites++;
	    i++;
	}
	
        attroff(COLOR_PAIR(3));
        
	
      /* Affichage du bandeau en bas de la fenêtre
         ---------------------------------------------------------------------------------  */
        
        attron(COLOR_PAIR(4)); // Couleur bleu sur fond noir 	        
        mvprintw(LINES - 2, 0, "'q' quitter | 'p' tri PID | 'n' tri NOM | 'm' tri MEM | '/'+""filtre""+Entrer pour filtrer, puis '/'+Entrer pour défiltrer.");
        attroff(COLOR_PAIR(4));
        
        attron(COLOR_PAIR(5) | A_BOLD); // Couleur noir sur fond jaune
	bandeau_bas(bandeau, COLS, sort_mode, filter);
	mvprintw(LINES - 1, 0, "%s", bandeau);	
        attroff(COLOR_PAIR(5) | A_BOLD);    
        
  
      
   //======================================================================
   //		ACTION UTILISATEUR
   //======================================================================     
   
   // Rafraichissement et attente de frappe au clavier :
        refresh();  // timeout(REFRESH_TIME)  
   
   /*Selection de l'utilisateur "On Key Pressed"
   --------------------------------------------				*/
          
	key = getch();
	if (get_keypressed(key, &sort_mode, &scroll_offset, filter, &shared.running, &shared.mutex))  // uiwin.c .h retourne 1 si erreur
 	    break;	 
    		
 	// Action sur touche pressée :
	// on_keypressed(key, &sort_mode, &scroll_offset, filter);	// uiwin.c .h


	
    //========================================================================================
    // Fin de la boucle principale do .. while    
                                                  
    } while (1);
    
        
   
   //======================================================================
   //		TERMINER
   //======================================================================  
  
    pthread_join(th_collecte, NULL);    
    free_shared(&shared);
    free(snap_liste); 
    endwin();  // ferme la fenêtre
    
    return errFlag;
    
} /** FIN DU PROGRAMME **/



