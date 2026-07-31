# 04 - Feuille de route

**Projet :** JICE-HTOP  
**Version :** 1.0

---

# 1. Objet du document

Ce document présente les principales évolutions envisagées pour le projet **JICE-HTOP**.

Il ne constitue pas un engagement de développement, mais décrit les orientations retenues afin d'améliorer progressivement les fonctionnalités, la qualité logicielle et la démarche d'ingénierie du projet.

---

# 2. Évolutions fonctionnelles

Les fonctionnalités suivantes pourront être intégrées dans les prochaines versions :

* arrêt d'un processus (`kill`) ;
* enrichissement des informations sur les processus ;
* nouvelles vues de supervision ;
* ajout de nouvelles métriques système.

Nouvelles métriques possibles :

| Métrique                                 | Source Linux         |
| ---------------------------------------- | -------------------- |
| Charge système (Load Average)            | `/proc/loadavg`      |
| Nombre de cœurs CPU                      | `/proc/cpuinfo`      |
| Temps CPU utilisateur / noyau            | `/proc/stat`         |
| Temps d'exécution des processus          | `/proc/[pid]/stat`   |
| Utilisation CPU par processus            | `/proc/[pid]/stat`   |
| Nombre de threads par processus          | `/proc/[pid]/status` |
| Mémoire virtuelle (VmSize)               | `/proc/[pid]/status` |
| État du processus (Running, Sleeping...) | `/proc/[pid]/stat`   |

---

# 3. Qualité logicielle

Le projet a vocation à intégrer progressivement des outils et pratiques de qualité :

* analyse statique du code (cppcheck, clang-tidy) ;
* intégration continue (GitHub Actions) ;
* tests automatisés ;
* amélioration de la couverture documentaire ;
* choix d'une licence Open Source.

---

# 4. Évolutions techniques

Les pistes d'amélioration envisagées incluent notamment :

* optimisation des performances ;
* amélioration du moteur d'affichage ;
* évolution de l'architecture en fonction des nouvelles fonctionnalités ;
* renforcement de la robustesse et de la maintenabilité.

---

## 4.1. Architecture multi‑threads (UI / Collecte / Orchestration)

* Création d’un second thread `th_render` dédié à l’affichage ncurses.  
* Le thread d’affichage disposerait de son propre cycle de rendu, avec accès synchronisé aux données partagées.  
* Le thread principal deviendrait un orchestrateur : initialisation, lancement des threads, gestion du filtre et du mode de tri, arrêt propre de l’ensemble.  
* Cette séparation améliorerait la réactivité de l’UI et isolerait complètement la logique d’affichage de la logique de supervision système.

---

## 4.2. Externalisation complète de l’affichage

* Déplacement des affichages de bannière, bandeau, en‑têtes et panneaux dans `uiwin.h / uiwin.c`.  
* L’ensemble serait géré directement par `th_render`.  
* Le code de `main.c` serait allégé et recentré sur l’orchestration.

---

## 4.3. Gestion du snapshot par le thread UI

* Le remplissage du buffer `snap_list` deviendrait la responsabilité de `th_render`.  
* Réduction de la durée des sections critiques dans le thread collecteur.  
* Amélioration de la fluidité de l’interface.

---

## 4.4. Optimisation du parsing de /proc (module sysproc)

* Actuellement, les processus sont comptés puis lus : méthode sûre, mais pouvant introduire un léger décalage temporel.  
* Une évolution possible consiste à **compter et lire en une seule passe**, afin d’éviter tout mismatch entre le nombre de processus et la liste réellement parcourue.  
* Cette optimisation impliquerait :  
  - un buffer dynamique (realloc progressif) ou une liste chaînée temporaire ;  
  - la suppression de `count_processes()` ;  
  - une logique de collecte plus flexible ;  
  - une meilleure robustesse face aux processus très courts.

---

# 5. Vision du projet

Au-delà de son objectif pédagogique initial, **JICE-HTOP** a vocation à constituer un projet démontrant une démarche professionnelle de développement logiciel, depuis l'expression des besoins jusqu'à la conception, la documentation, les pratiques de qualité et les perspectives d'évolution.  

---

