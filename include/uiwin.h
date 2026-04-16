#ifndef UIWIN_H
#define UIWIN_H

#include "sysproc.h"

#define REFRESH_TIME 200			// Rafraichissement de la fenêtre
#define L_DATA_GLOBAL 1 			// position du cartouche qui affiche les infos globales (3 lignes) apres le titre (+1)
#define L_TAB_PROCESS (L_DATA_GLOBAL + 3) 	// 3 lignes de prévues pour les données globales
#define L_LIST_PROCESS (L_TAB_PROCESS + 3) 	// affichage des processus

   

void init_ncurses(void);

void ram_display(const unsigned long total, const unsigned long avail, const unsigned long used, const float percent, const unsigned long self_use)  ;

void draw_header(void);

int bandeau_bas(char *dest, int sMax, t_sort_mode smode, const char *filter);

void calcul_scroll(int nb_affiches, int lignes_dispo, int *scroll_offset, int *max_scroll, int *bar_height, int *bar_pos);

void draw_scrollbar(int bar_height, int bar_pos, int y0);

int cmp_filtre(const char* strg, const char* sub);

void on_keypressed(int key, t_sort_mode *mode, int *scroll_offset, char *filter);

#endif

