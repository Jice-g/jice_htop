/**
 * @file uiwin.h
 * @brief Interface ncurses — initialisation, aides d’affichage et gestion des entrées.
 *
**/

#ifndef UIWIN_H
#define UIWIN_H

#include "sysproc.h"
#include <pthread.h>


/** Timeout de getch() en millisecondes ; contrôle la fréquence de rafraîchissement de l’UI. */
#define REFRESH_TIME    200

/*
 * La "mise en page" est réalisée par la définition de bandeaux.
 * Et des lignes dans ces bandeau affectées à des information particulière.
 * Les constantes de mise en page définissent une structure fixe de lignes
 * dans la fenêtre du terminal :
 *
 *   Ligne 0           		: bannière de titre, nombre de processus (1 ligne)
 *   Lignes L_DATA_GLOBAL 	: métriques RAM (3 lignes)
 *   Lignes L_TAB_PROCESS 	: en-tête des colonnes (3 lignes)
 *   Lignes L_LIST_PROCESS 	: liste des processus défilante
 *   Ligne LINES-2     		: rappel des raccourcis clavier
 *   Ligne LINES-1        	: barre d’état dynamique (mode de tri + filtre actif)
 *
 * Leurs positions sont définies en relatif avec des #define :
 */

/** Première ligne du bloc des métriques RAM (juste sous la bannière de titre). */
#define L_DATA_GLOBAL   1

/** Première ligne du bloc d’en-tête des colonnes (3 lignes sous L_DATA_GLOBAL). */
#define L_TAB_PROCESS   (L_DATA_GLOBAL + 3)

/** Première ligne de la liste défilante des processus (3 lignes sous L_TAB_PROCESS). */
#define L_LIST_PROCESS  (L_TAB_PROCESS + 3)


/* ---------------------------------------------------------------------------
 * Initialisation
 * ------------------------------------------------------------------------- */

/**
 * @brief Initialise ncurses et configure les cinq paires de couleurs utilisées par l’UI.
 *
 * Correspondance des paires de couleurs :
 *   1 — noir sur blanc  (bannière de titre)
 *   2 — blanc sur noir, gras (en-têtes de colonnes)
 *   3 — vert sur noir   (lignes de processus)
 *   4 — cyan sur noir   (rappel des raccourcis)
 *   5 — noir sur jaune  (barre d’état dynamique)
 */
void init_ncurses(void);


/* ---------------------------------------------------------------------------
 * Affichage
 * ------------------------------------------------------------------------- */

/**
 * @brief Affiche le bloc des métriques RAM aux lignes L_DATA_GLOBAL … +2.
 *
 * @param total     RAM totale installée, en kB.
 * @param avail     RAM disponible, en kB.
 * @param used      RAM utilisée, en kB.
 * @param percent   Pourcentage de RAM utilisée.
 * @param self_use  Pic de RSS de ce processus, en kB.
 */
void ram_display(const unsigned long total, const unsigned long avail,
                 const unsigned long used, const float percent,
                 const unsigned long self_use);

/**
 * @brief Affiche le bloc d’en-tête des colonnes aux lignes L_TAB_PROCESS … +2.
 */
void draw_header(void);

/**
 * @brief Formate la chaîne de la barre d’état dynamique dans @p dest.
 *
 * Écrit "Tri: <mode> | Filtre: <filter>" dans @p dest, terminé par NUL et
 * tronqué à @p sMax octets.
 *
 * @param dest    Tampon de destination.
 * @param sMax    Taille de @p dest en octets (souvent COLS) [pas de cas "spéciaux"]
 * @param smode   Mode de tri actif.
 * @param filter  Chaîne de filtre active, ou chaîne vide si aucun.
 * @return        Longueur de la chaîne formatée (retour de snprintf).
 */
int low_status_bar(char *dest, int sMax, t_sort_mode smode, const char *filter);

/**
 * @brief Calcule la géométrie de la barre de défilement selon la liste et la zone visible.
 *
 * Force @p scroll_offset dans l’intervalle [0, max_scroll] et déduit bar_height
 * et bar_pos pour que la barre reflète la portion visible de la liste.
 *
 * @param nb_visible    Nombre total de lignes correspondant au filtre actif.
 * @param avail_lines   Nombre de lignes disponibles dans le panneau des processus.
 * @param[in,out] scroll_offset  Décalage de défilement actuel ; ajusté si nécessaire.
 * @param[out]    max_scroll     Décalage maximal atteignable.
 * @param[out]    bar_height     Hauteur de la barre de défilement, en lignes.
 * @param[out]    bar_pos        Position du curseur de défilement, en lignes.
 */
void compute_scroll(int nb_visible, int avail_lines, int *scroll_offset,
                   int *max_scroll, int *bar_height, int *bar_pos);

/**
 * @brief Affiche la barre de défilement sur le bord droit du panneau des processus.
 *
 * @param bar_height  Hauteur de la barre de défilement, en lignes.
 * @param bar_pos     Position du curseur dans la barre. Et définition du curseur avec '|||' et '[=]
 * @param y0          Ligne absolue de la première cellule de la barre (= L_LIST_PROCESS).
 */
void draw_scrollbar(int bar_height, int bar_pos, int y0);


/* ---------------------------------------------------------------------------
 * Filtrage
 * ------------------------------------------------------------------------- */

/**
 * @brief Teste si @p sub apparaît comme sous-chaîne dans @p strg.
 *
 * La comparaison ignore la casse. Une chaîne @p sub vide correspond toujours.
 *
 * @param strg  Chaîne dans laquelle chercher (nom du processus).
 * @param sub   Sous-chaîne recherchée (filtre utilisateur).
 * @return      1 si @p sub est trouvée dans @p strg, 0 sinon.
 */
int match_filter(const char *strg, const char *sub);


/* ---------------------------------------------------------------------------
 * Gestion des entrées
 * ------------------------------------------------------------------------- */

/**
 * @brief Gère une touche pressée et signale la sortie si 'q' / 'Q' est pressé.
 *
 * Si la touche est 'q' ou 'Q', verrouille @p mutex, met *running à 0,
 * déverrouille le mutex, et retourne 1
 * pour indiquer au code appelant qu’il doit quitter la boucle d’affichage (while 0).  
 * Pour toutes les autres touches, délègue à on_keypressed() et retourne 0.
 * on_keypressed n'est pas un "évènement" techniquement parlant, mais s'en rapproche.
 *
 * Cette fonction est volontairement appelée hors des sections critiques ;
 * seul @p running est modifié sous mutex. Il existe un risque de collision
 * qui rendrait le comportement indéterminé.
 *
 * @param key           Code de la touche retourné par getch().
 * @param mode          Pointeur vers le mode de tri actif.
 * @param scroll_offset Pointeur vers le décalage de défilement.
 * @param filter        Tampon de filtre (jusqu’à 255 caractères + NUL).
 * @param running       Pointeur vers le drapeau partagé running.
 * @param mutex         Mutex protégeant @p running.
 * @return              1 si la boucle d’affichage doit se terminer, 0 sinon.
 */
int get_keypressed(int key, t_sort_mode *mode, int *scroll_offset,
                   char *filter, int *running, pthread_mutex_t *mutex);

/**
 * @brief Applique l’effet d’une touche (hors sortie) sur l’état de l’UI.
 *
 * Raccourcis :
 *   'p' / 'P' : passer à SORT_PID  et réinitialiser le défilement
 *   'n' / 'N' : passer à SORT_NAME et réinitialiser le défilement
 *   'm' / 'M' : passer à SORT_MEM  et réinitialiser le défilement
 *   KEY_UP    : défiler d’une ligne vers le haut
 *   KEY_DOWN  : défiler d’une ligne vers le bas
 *   '/'       : demander une chaîne de filtre (bloque jusqu’à Entrée)
 *
 * @param key           Code de la touche retourné par getch().
 * @param mode          Pointeur vers le mode de tri actif.
 * @param scroll_offset Pointeur vers le décalage de défilement.
 * @param filter        Tampon de filtre (jusqu’à 255 caractères + NUL).
 */
void on_keypressed(int key, t_sort_mode *mode, int *scroll_offset, char *filter);


#endif /* UIWIN_H */

