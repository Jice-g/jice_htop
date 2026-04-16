
*******
# JICE‑HTOP  
Mini‑conception et programmation pédagogique de htop en C / ncurses.  
Projet réalisé par @Jicé dans le cadre du test d’entrée B3 – La Plateforme - 2026.
*******



PRESENTATION
-----------

JICE‑HTOP est une application console interactive écrite en C, utilisant la bibliothèque ncurses, qui reproduit quelques fonctionnalités essentielles de l’outil htop.  
Le programme lit directement les informations système dans le pseudo‑système de fichiers /proc et affiche en temps réel :

- la liste des processus actifs  
- leur PID  
- leur nom  
- leur mémoire virtuelle résidente (VmRSS) et celle de jice_htop
- l’utilisation globale de la RAM  
- un tri dynamique (PID / NOM / MEM)  
- un filtrage interactif  - message si aucune correspondance
- un scroll vertical fluide  
- une barre de défilement proportionnelle (scrollbar)

~ le multitache n'a pas été intégré.

Le projet a été réalisé dans le cadre du test d’entrée B3 – La Plateforme,
dans le respect des exigences du "cahier des charges" avec un accent particulier sur la qualité du code, la modularité et la robustesse.


FONCTIONALITES
--------------

Affichage des processus :  
- Lecture de /proc/[PID]/comm pour le nom  
- Lecture de /proc/[PID]/status pour la mémoire (VmRSS)  
- Tri dynamique :  
  - p, P --> tri par PID  
  - n, N --> tri par NOM  
  - m, M --> tri par MEM  
  
Informations système :  
- Lecture de /proc/meminfo  
- Affichage de la mémoire totale, utilisée, disponible et du pourcentage d’utilisation  

Interface ncurses :  
- Scroll vertical avec les flèches et la souris fonctionne
- Barre de défilement proportionnelle  
- Bandeau supérieur et inférieur  
- Mise en forme colorée et lisible  

Filtrage interactif avec la touche '/' :


ARCHITECTURE DU PROJET
-----------------------

Le projet est organisé de manière modulaire :

src/  
- main.c : boucle ncurses, affichage, interactions  
- sysproc.c : lecture /proc, RAM, tri, structures  
- uiwin.c : bandeau bas, fonctions d’interface  

include/  
- sysproc.h : structures, énumérations, prototypes système  
- uiwin.h : prototypes UI  

obj/  
- fichiers objets générés automatiquement  

Makefile  
- compilation modulaire, dépendances automatiques, règles clean/fclean/re  


COMPILATION
------------

ncurses installé :
sudo apt install libncurses5-dev libncursesw5-dev

Compiler le projet : make

Nettoyer les objets : make clean

Recompiler entièrement : make re

Exécution  
./jice_htop


COMMANDE ET SERVICES 
-------------------

q : quitter  
p : tri par PID  
n : tri par NOM  
m : tri par MEM  
 KEY_UP et KEY_DOWN  : scroll vertical  
/ : filtrer par nom (Entrée pour valider)
   - Pour filtrer taper '/' puis le texte du filtre, puis Entrer
   - Pour défiltrer taper '/' puis Entrer  


DETAILS TECHNIQUES 
------------------

Lecture des processus :  
Le programme parcourt /proc, détecte les entrées numériques (PID), puis lit :  
- /proc/[PID]/comm 	pour le nom  
- /proc/[PID]/status 	pour VmRSS  

Lecture de la RAM :  
Lecture de MemTotal et MemAvailable dans /proc/meminfo.  
Calcul :  RAM utilisée = MemTotal – MemAvailable.
		  RAM % = (RAM utilisée * 100) / MemTotal;
La consommation du programme JICE_HTOP est issue de :
(getrusage(RUSAGE_SELF, &selfusage)  <sys/resource.h>


Scrollbar proportionnelle :  
La position du curseur est calculée ainsi :  
bar_pos = (scroll_offset * (bar_height - 1)) / max_scroll  
Ce qui garantit que la barre atteint réellement le bas.
Sauf dans le cas d'un filtre où tout est à affiché et donc rien à dérouler.


DEPENDANCES
-----------
- ncurses  
- Linux (accès à /proc)  
- GCC  


AUTEUR
------
Projet réalisé par Jean-Christophe Gerace @Jicé,  


RESERVES :
-----------------------------------
La partie multithread n'a pas encore été implémentée.
-------
Elle pourra l'être sur demande.
Les fonctions de gestion d'affichage et de mise à jour des données ont déjà été séparées avec -sysproc -uiwin

L’objectif du multithreading serait alors de séparer deux responsabilités actuellement exécutées à la suite dans la même boucle : l’affichage ncurses et la collecte des données système. Cette séparation permettrait d’obtenir une interface plus fluide, même lorsque la lecture de /proc prend du temps.

Par exemple :
 - Thread principal qui gère l’affichage, capture les entrées clavier, met à jour la scrollbar et le scroll, reste réactif même lorsque la collecte des données prend du temps
  - Thread secondaire (collecte des données système) qui rafraîchit périodiquement la liste des processus, relit /proc/[PID]/*, relit /proc/meminfo, met à jour une structure partagée contenant les données système

Synchronisation : Pour éviter les corruptions de données entre les deux threads (qui accèdent à la même struct !), il faudrait implémenter un mutex:
- Le thread secondaire, verrouille, met à jour les données puis il déverrouille.
- Le thread principal : verrouille, lit les données, déverrouille.

Fréquence de rafraîchissement : 200 ms, identique au timeout ncurses actuel

*******************************************
FIN

