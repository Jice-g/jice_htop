#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <ncurses.h>
#include "uiwin.h"


 

// INITIALISATION DE NCURSES
//
void init_ncurses(void) {
   
    initscr();
    noecho();
    curs_set(FALSE); 	    // pas de saisie au départ
    keypad(stdscr, TRUE);   // gestion des couleurs 
    timeout(REFRESH_TIME);  // getch() attendra au plus REFRESH_TIME  millisecondes.    
//  Colorisation :
    start_color(); // Activation du système de couleur de ncurses.
    init_pair(1, COLOR_BLACK, COLOR_WHITE); // bandeau haut    
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // titres
    init_pair(3, COLOR_GREEN, COLOR_BLACK);// processus
    init_pair(4, COLOR_CYAN, COLOR_BLACK); // menu contextuel   
    init_pair(5, COLOR_BLACK, COLOR_YELLOW); // bandeau bas

}


// AFFICHAGE RAM
//
void ram_display(const unsigned long total, const unsigned long avail, const unsigned long used, const float percent) {    
        attron(COLOR_PAIR(3));  ; // Couleur verte sur fond noir            
        mvprintw(L_DATA_GLOBAL + 0, 0, "%-16s %6lu MB", "Mem totale: ", total / 1024);
        mvprintw(L_DATA_GLOBAL + 1, 0, "%-16s %6lu MB  %.1f%%", "Mem utilisee : ", used / 1024, percent);    
        mvprintw(L_DATA_GLOBAL + 2, 0, "%-16s %6lu MB", "Mem libre : ", avail / 1024);
        attroff(COLOR_PAIR(3));  ; 
}


// FONCTIONS UTILES POUR L'AFFICHAGE :
//
void draw_header(void) {     
        attron(COLOR_PAIR(2) | A_BOLD); // Couleur noir sur fond bleu                            
        mvprintw(L_TAB_PROCESS + 0, 0, "------------------------------------------------");
        mvprintw(L_TAB_PROCESS + 1, 0, " %s     %s   %s", "PID", "MEM (kb)", "NOM");
        mvprintw(L_TAB_PROCESS + 2, 0, "------------------------------------------------");
        attroff(COLOR_PAIR(2) | A_BOLD);     
}


int bandeau_bas(char* dest, int sMax, t_sort_mode smode, const char* filter) {
    // 1) Texte du mode de tri
    const char* tri =
        smode == SORT_PID  ? "PID" :
        	smode == SORT_NAME ? "NOM" : "MEM";
        	
    // 2) Texte du filtre
    const char* f = (filter && filter[0]) ? filter : ""; // filter ni NULL ni nul sinon rien

    // 3) Écriture sécurisée dans le buffer fourni
    // sMax = taille max utilisable (COLS - 1)
    snprintf(dest, sMax, "Tri: %s | Filtre: %s", tri, f);
    
    return strlen(dest);
}

// Calculer la hauteur du scroll-bar et la position du curseur
void calcul_scroll(int nb_affiches, int lignes_dispo, int *scroll_offset,
                    int *max_scroll, int *bar_height, int *bar_pos) {
                    
    *max_scroll = nb_affiches - lignes_dispo;
    if (*max_scroll < 0) *max_scroll = 0;

    if (*scroll_offset < 0) *scroll_offset = 0;
    if (*scroll_offset > *max_scroll) *scroll_offset = *max_scroll;

    *bar_height = (nb_affiches > lignes_dispo) ? lignes_dispo : nb_affiches;
    *bar_pos = (*max_scroll == 0) ? 0 : (*scroll_offset * (*bar_height - 1)) / *max_scroll;
}


void draw_scrollbar(int bar_height, int bar_pos, int y0) {
    for (int y = 0; y < bar_height; y++) {
        mvprintw(y0 + y, COLS - 3, (y == bar_pos) ? "[=]" : "|||");
    }
}

// Fonction de comparaison avec filtre, insensible à la casse
int cmp_filtre(const char* strg, const char* sub)
{
    if (!strg || !sub || !sub[0])
        return 1; // filtre vide = tout passe
        // à présent on au moins un caractère dans la chaine

	    for (int i = 0; strg[i]; i++)
	    { 
	    
		int j = 0;
		while ( sub[j] 	&&  tolower((unsigned char)strg[i + j]) == tolower((unsigned char)sub[j])) {
		   j++;   }
		   		
		if (sub[j] == '\0')
		    return 1; // on au moins a une correspondance
	        }
	    
	    return 0;
}    


// FONCTION SELECTION IHM
// Sur touche clavier :
void on_keypressed(int key, t_sort_mode *mode, int *scroll_offset, char *filter) 
{
    if (key == 'p' || key == 'P') { 
    	*mode = SORT_PID;
    	*scroll_offset = 0; }
    	
    else if (key == 'n' || key == 'N') { 
    	*mode = SORT_NAME;
    	*scroll_offset = 0; }
    	
    else if (key == 'm' || key == 'M') { 
    	*mode = SORT_MEM;
    	*scroll_offset = 0; }
    	
    else if (key == KEY_UP)
    	(*scroll_offset)--;
    else if (key == KEY_DOWN)
    	(*scroll_offset)++;
    	
    else if (key == '/') {
        timeout(-1);
        echo();
        curs_set(TRUE);
        getnstr(filter, 255);
        noecho();
        curs_set(FALSE);
        timeout(REFRESH_TIME);
    }
}

/* FIN */      
 


