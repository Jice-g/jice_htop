# Changelog

Toutes les modifications notables de ce projet seront documentées dans ce fichier.

Le format s'inspire de **Keep a Changelog** et suit les principes du **Semantic Versioning**.

---

## [2.0.0] - 2026-05-16

### Ajouté

* Nouvelle architecture multithread basée sur POSIX Threads (`pthread`).
* Séparation des responsabilités entre le thread d'interface utilisateur et le thread de collecte système.
* Création d'une structure partagée (`t_shared`) pour l'échange des données entre les threads.
* Mise en œuvre d'une synchronisation par mutex afin de garantir la cohérence des données partagées.
* Introduction d'un mécanisme de **double-buffering (Snap)** afin de réduire le temps passé dans les sections critiques.
* Nouveau module `threadshare.c` dédié à la gestion des échanges entre threads.
* Documentation technique de l'architecture multithread.

### Modifié

* Refonte complète de l'architecture interne du projet.
* Le thread principal est désormais exclusivement consacré à l'interface utilisateur (`ncurses`).
* La collecte des informations système est réalisée de manière asynchrone par un thread dédié.
* Réorganisation du code afin de renforcer la modularité et la séparation des responsabilités.
* Amélioration de la fluidité de l'affichage lors du rafraîchissement des processus.

### Optimisé

* Réduction de la durée de verrouillage du mutex grâce au mécanisme de double-buffering.
* Limitation des accès concurrents aux structures de données partagées.
* Amélioration de la maintenabilité du code par une meilleure isolation des composants.

### Conservé

Les fonctionnalités de la version 1.x ont été intégralement conservées :

* Affichage des processus Linux.
* Tri par PID, nom et mémoire.
* Filtrage interactif.
* Scroll vertical et barre de défilement.
* Lecture du système de fichiers `/proc`.
* Affichage des statistiques mémoire.
* Interface utilisateur basée sur `ncurses`.

---

## [2.1.0] - 2026-06-28

### Corrigé

**Correction de bug :**

- Le tri alphabétique (SORT_NAME) affichait les threads noyau (ex. `[kthreadd]`)
  comme un groupe parasite intercalé entre les noms à majuscule et les noms en
  minuscule. Cause : `[` vaut ASCII 91, entre `Z` (90) et `a` (97). Corrigé par
  l'introduction de `skip_non_alpha()` et le remplacement de `strcmp()` par
  `strcasecmp()`qui prend en compte la casse.
  Voir `docs/bugfixes/BUGFIX_sort_nom.md`.
  
---

## [1.x] - Version initiale

### Architecture

Version entièrement séquentielle.

L'ensemble des traitements (lecture de `/proc`, mise à jour des données et affichage de l'interface) était exécuté dans une unique boucle principale située dans `main.c`.

Cette première version a permis de valider :

* la lecture des informations système via `/proc` ;
* l'interface utilisateur avec `ncurses` ;
* les fonctions de tri, de filtrage et de navigation ;
* l'organisation générale du projet avant son évolution vers une architecture concurrente.

Cette organisation initiale a aussi permis de faire évoluer l'application vers une architecture multithread sans remettre en cause le code métier existant. La refonte s'est principalement concentrée sur la redistribution des responsabilités entre le thread d'interface utilisateur et le thread de collecte système, tout en conservant les fonctionnalités et les comportements validés dans la version initiale.
Cette évolution illustre l'intérêt d'une architecture modulaire : lorsqu'elle est correctement conçue dès le départ, il devient possible de modifier profondément l'architecture d'exécution (ici, le passage d'un modèle séquentiel à un modèle concurrent) tout en limitant les modifications du code fonctionnel.

