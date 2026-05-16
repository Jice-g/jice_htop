********************************************************
JICE‑HTOP  V2.0
Mini‑réimplémentation pédagogique de htop en C / ncurses  
********************************************************

La V2.0 intègre des modifications :
 - Majeures : intégration du multithreading
 - Mineures : diverses (factorisation, lisibilité, robustesse (vérifications),...)

PRESENTATION
------------

JICE‑HTOP est une application console interactive écrite en C, utilisant la bibliothèque ncurses, qui reproduit quelques fonctionnalités essentielles de l’outil htop.  
Le programme lit directement les informations système dans le pseudo‑système de fichiers /proc et affiche en temps réel :

- la liste des processus actifs  
- leur PID  
- leur nom  
- leur mémoire résidente (VmRSS)  
- l’utilisation globale de la RAM  
- un tri dynamique (PID / NOM / MEM)  
- un filtrage interactif  - message si aucune correspondance
- un scroll vertical fluide  
- une barre de défilement proportionnelle (scrollbar)

++ le multitache a été intégré.

Le projet a été réalisé dans le cadre du test d’entrée B3 – La Plateforme,
dans le respect des exigences du "cahier des charges" avec un accent particulier sur la qualité du code, la modularité et la robustesse.



FONCTIONALITES
---------------

Affichage des processus :  
- Lecture de /proc/[PID]/comm pour le nom  
- Lecture de /proc/[PID]/status pour la mémoire (VmRSS)  
- Tri dynamique :  
  --> tri par PID  
  --> tri par NOM  
  --> tri par MEM  
  
N.B : Le tri par NOM commence par une dizaine d'enregistrements au dessus du tri alphabétique général
Il y a les noms spéciaux commençant par '('. Ok. 
Mais pour le reste, la cause est inconnue.
qsort(liste_proc, nb, sizeof(t_process), compare_by_name); semble tout à fait correct.
Hyppothèse : Noms tronqués par /proc/[PID]/comm , ou bien c’est la source des noms qui est "exotique" (générés automatiquement par systemd, kernel, sandbox, etc. (??))


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

_____________________________

jice_htop/
│
├── src/
│   ├── main.c
│   ├── sysproc.c  
│   ├── uiwin.c
│
├── include/
│   ├── sysproc.h  
│   ├── uiwin.h
│
└── Makefile
______________________________


COMPILATION
-----------

ncurses installé :
sudo apt install libncurses5-dev libncursesw5-dev

Compiler le projet : make

Nettoyer les objets : make clean

Recompiler entièrement : make re

Exécution  
---------

./jice_htop


COMMANDE ET SERVICES 
--------------------

q, Q : quitter  
p, P : tri par PID  
n, N : tri par NOM  
m, M : tri par MEM  
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
Calcul : 	RAM utilisée = MemTotal – MemAvailable.
		RAM % = (RAM utilisée * 100) / MemTotal;

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


***********************************
ARCHITECTURE MULTITHREAD :
-----------------------------------

Séparer :
 - l’interface utilisateur ;
 - la collecte des données système.
 
Architecture de JICE_HTOP V1.2.1
-------------------------------------
La V1.2.1 fonctionne avec :

 - une seule boucle principale ;
 - un seul thread ;
 - un cycle :
 - lecture /proc
 - tri
 - affichage ncurses
 - attente clavier
 - recommencer

┌──────────────────────────────┐
│ Boucle principale unique     │
├──────────────────────────────┤
│ Lire /proc                   │
│ Lire meminfo                 │
│ Trier les processus          │
│ Afficher ncurses             │
│ Lire clavier                 │
└──────────────────────────────┘



Architecture JICE_HTOP V2:
--------------------------------

┌──────────────────────────────┐
│ Thread UI (principal)        │
├──────────────────────────────┤
│ ncurses                      │
│ clavier                      │
│ scroll                       │
│ affichage                    │
└──────────────┬───────────────┘
               │
               │ mutex
               ▼
┌──────────────────────────────┐
│ Données partagées            │
├──────────────────────────────┤
│ liste processus              │
│ RAM                          │
│ nb processus                 │
│ état global                  │ 
│ synchronisation              │
└──────────────┬───────────────┘
               │
               │ mutex
               ▼
┌──────────────────────────────┐
│ Thread Collecte              │
├──────────────────────────────┤
│ lecture /proc                │
│ lecture meminfo              │
│ mise à jour des données      │
└──────────────────────────────┘


1. Thread principal = Interface utilisateur
----------------------------------------
Responsabilités : (dans le main et avec uiwin.c)
 - ncurses ;
 - affichage ;
 - clavier ; 
 - tri ;
 - filtrage ;
 - scrollbar ;
 - navigation.

Il ne lit plus /proc.

Son rôle devient uniquement visuel et interactif.


2. Thread secondaire = Collecte système
--------------------------------------
Responsabilités :
 - parcourir /proc ;
 - remplir t_process;
 - lire /proc/meminfo;
 - mettre à jour les structures globales.

Ce thread tourne en boucle indépendante :

while (running)  // running sera défini dans la structure des données partagées
{
    collecter_processus();
    ...
    lire_ram();
    ...
    usleep(200000); // 200 ms
}



La V2 ne constitue pas une simple amélioration fonctionnelle.
-------------------------------------------------------------

Elle transforme profondément :
 - l’architecture ;
 - le modèle d’exécution ;
 - la gestion des données ;
 - la qualité temps réel de l’application.

La solution retenue est :
 - cohérente ;
 - scalable ;
 - robuste ;
 - conforme aux architectures professionnelles Linux.

La migration depuis la V1.2.1 est particulièrement adaptée car votre code est déjà :
 - modulaire ;
 - propre ;
 - structuré ;
 - découplé.

Angle d'approche :
 - moniteurs système réels ;
 - outils Linux professionnels ;
 - architectures temps réel modernes.


FONCTIONNEMENT
--------------
main
 ├── init_shared
 ├── pthread_create
 └── boucle UI 
 'q' (gestion dans une fonction get_keypressed)
 ├── running = 0
 ├── pthread_join
 ├── free_shared
 ├── pthread_mutex_destroy
 └── endwin()


________________________________________________

AUTEUR
------
Projet réalisé par Jean-Christophe Gerace @Jicé,  
dans le cadre du test d’entrée B3 – La Plateforme - 2026.


*******************************************
FIN

