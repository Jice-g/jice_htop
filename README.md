********************************************************
JICE‑HTOP  V2.0
Mini‑réimplémentation pédagogique de htop en C / ncurses  
********************************************************

La V2.0 intègre des modifications :
 - Majeures : intégration du multithreading
 - Mineures : diverses (factorisation, lisibilité, robustesse (vérifications),...)

# JICE-HTOP

Mini outil de supervision système sous Linux développé en C, inspiré de htop.

Projet réalisé dans le cadre du test d'admission en 3e année à La Plateforme (Marseille).

## Compétences démontrées

* Développement système Linux
* Langage C
* Programmation multithread (POSIX Threads)
* Synchronisation par mutex
* Gestion mémoire dynamique
* Lecture du système de fichiers `/proc`
* Interface terminal avec ncurses
* Architecture modulaire
* Tri, filtrage et affichage temps réel
* Gestion robuste des erreurs

---

## Présentation

JICE-HTOP est une application console interactive qui affiche en temps réel des informations système directement collectées depuis le pseudo-système de fichiers Linux `/proc`.

L'objectif du projet était de concevoir une application complète, maintenable et évolutive, en privilégiant :

* la robustesse ;
* la modularité ;
* la lisibilité du code ;
* la séparation des responsabilités.

L'application permet notamment :

* l'affichage des processus actifs ;
* le tri dynamique par PID, nom ou mémoire ;
* le filtrage interactif ;
* l'affichage de la mémoire système ;
* le défilement vertical avec scrollbar ;
* la mise à jour temps réel des données.

---

## Architecture

### Version 2 : architecture multithread

L'application repose sur deux threads distincts.

### Thread principal : Interface utilisateur

Responsabilités :

* affichage ncurses ;
* gestion clavier ;
* tri ;
* filtrage ;
* navigation ;
* scrollbar.

### Thread secondaire : Collecte système

Responsabilités :

* parcours de `/proc` ;
* collecte des processus ;
* lecture de `/proc/meminfo` ;
* mise à jour des données système.

Les données sont partagées via une structure commune protégée par mutex.

```text
                +------------------+
                |  Thread UI       |
                |  ncurses         |
                +--------+---------+
                         |
                         | mutex
                         |
                +--------v---------+
                |  Données         |
                |  partagées       |
                +--------+---------+
                         |
                +--------v---------+
                | Thread Collecte  |
                | /proc            |
                +------------------+
```

Le thread d'interface travaille sur une copie locale ("snapshot") des données afin de minimiser le temps passé en section critique.

---

## Structure du projet

```text
jice_htop/
│
├── src/
│   ├── main.c
│   ├── sysproc.c
│   ├── uiwin.c
│   └── threadshare.c
│
├── include/
│   ├── sysproc.h
│   ├── uiwin.h
│   └── threadshare.h
│
├── obj/
│
├── Makefile
│
└── README.md
```

---

## Informations collectées

### Processus

Lecture directe de :

```text
/proc/[PID]/comm
/proc/[PID]/status
```

Informations affichées :

* PID ;
* nom du processus ;
* mémoire résidente (VmRSS).

### Mémoire système

Lecture directe de :

```text
/proc/meminfo
```

Informations affichées :

* mémoire totale ;
* mémoire disponible ;
* mémoire utilisée ;
* pourcentage d'utilisation ;
* consommation mémoire de l'application.

---

## Commandes

| Touche | Action                 |
| ------ | ---------------------- |
| q      | Quitter                |
| p      | Tri par PID            |
| n      | Tri par nom            |
| m      | Tri par mémoire        |
| ↑ ↓    | Défilement             |
| /      | Filtrage des processus |

---

## Compilation

### Dépendances

* GCC
* ncurses
* Linux

Installation de ncurses :

```bash
sudo apt install libncurses5-dev libncursesw5-dev
```

Compilation :

```bash
make
```

Nettoyage :

```bash
make clean
```

Reconstruction complète :

```bash
make re
```

---

## Exécution

```bash
./jice_htop
```

---

## Points techniques intéressants

* utilisation de `qsort()` avec comparateurs dédiés ;
* gestion mémoire dynamique avec `calloc()` et `realloc()` ;
* synchronisation par mutex POSIX ;
* réduction du temps de verrouillage grâce à un mécanisme de snapshot local ;
* prise en compte des cas limites liés aux processus disparaissant pendant la collecte ;
* architecture facilement extensible.

---

## Perspectives d'évolution

* affichage CPU par processus ;
* affichage multi-cœurs ;
* statistiques historiques ;
* couleurs dynamiques ;
* export des métriques ;
* optimisation des performances de collecte.

---

## Auteur

Jean-Christophe Gerace  @jice

Projet pédagogique réalisé dans le cadre du test d'admission en troisième année à La Plateforme.



*******************************************
FIN

