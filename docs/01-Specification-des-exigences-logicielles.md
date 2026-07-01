# 01 - Spécification des exigences logicielles (SRS)

**Projet :** JICE-HTOP

**Version :** 1.0

---

# 1. Introduction

## 1.1 Objet du document

Le présent document définit les exigences fonctionnelles et non fonctionnelles du projet **JICE-HTOP**.

Il constitue la référence décrivant les fonctionnalités attendues du logiciel ainsi que les principaux objectifs de qualité qui guideront sa conception, son développement, ses tests et ses évolutions.

Ce document décrit **ce que le logiciel doit réaliser**. Les choix techniques permettant de satisfaire ces exigences sont décrits dans les documents d'architecture associés.

---

## 1.2 Présentation du projet

JICE-HTOP est un moniteur système interactif fonctionnant dans un terminal Linux.

Inspiré du logiciel *htop*, il permet de visualiser en temps réel différentes informations relatives à l'état du système, notamment l'utilisation des ressources matérielles et la liste des processus actifs.

---

## 1.3 Contexte du projet

Le projet JICE-HTOP est né dans le cadre de l'épreuve d'admission directe en troisième année du Bachelor IT – Développeur Logiciel & DevOps de La Plateforme.

Au-delà de ce contexte initial, il évolue comme un projet personnel ayant plusieurs objectifs :

* démontrer des compétences en développement système sous Linux ;
* servir de support technique à destination des recruteurs lors d'une recherche d'alternance ;
* approfondir la pratique de l'ingénierie logicielle, de la documentation technique et des méthodes de développement modernes.

---

## 1.4 Objectifs

Les principaux objectifs du projet sont les suivants :

* fournir une visualisation temps réel de l'état du système ;
* proposer une interface utilisateur fluide en mode texte ;
* démontrer une architecture logicielle modulaire et évolutive ;
* appliquer des pratiques modernes de qualité logicielle ;
* produire une documentation technique cohérente et structurée.

---

## 1.4 Périmètre fonctionnel

La première version du logiciel couvre principalement :

* l'observation des ressources système ;
* l'affichage des processus en cours d'exécution ;
* la navigation dans une interface interactive en mode texte.

Les fonctionnalités avancées (export, plugins, supervision distante, etc.) sont volontairement exclues du périmètre initial et pourront être intégrées lors des évolutions futures.

---

# 2. Parties prenantes

Le projet s'adresse principalement :

* aux recruteurs et responsables techniques évaluant les compétences du développeur ;
* aux utilisateurs Linux souhaitant disposer d'un outil léger de supervision système ;
* aux développeurs souhaitant étudier une architecture logicielle en langage C ;
* aux éventuels contributeurs souhaitant participer au projet.

---

# 3. Exigences fonctionnelles

## FR-01 — Démarrage de l'application

Le logiciel doit pouvoir être exécuté depuis un terminal Linux.

---

## FR-02 — Affichage temps réel

Le logiciel doit afficher les informations système en temps réel avec une fréquence de rafraîchissement régulière.

---

## FR-03 — Surveillance du processeur

Le logiciel doit afficher l'utilisation du ou des processeurs.

---

## FR-04 — Surveillance de la mémoire

Le logiciel doit afficher :

* la mémoire physique utilisée ;
* la mémoire disponible ;
* - empreinte mémoire du processus jice_htop lui-même

---

## FR-05 — Liste des processus

Le logiciel doit afficher les processus actuellement exécutés sur le système.

Chaque processus affiché doit comporter au minimum :

* son identifiant (PID) ;
* son nom ;
* son utilisation mémoire.

---

## FR-06 — Rafraîchissement des informations

Les informations affichées doivent être mises à jour automatiquement sans nécessiter d'action de l'utilisateur.

---

## FR-07 — Navigation

Le logiciel doit permettre la navigation dans l'interface au clavier.

---

## FR-08 — Filtrage

Le logiciel doit permettre de filtrer les processus affichés selon différents critères.

---

## FR-09 — Gestion des erreurs

Le logiciel doit détecter les erreurs critiques pouvant empêcher son fonctionnement et informer l'utilisateur de manière explicite.

---

## FR-10 — Arrêt propre

Le logiciel doit libérer les ressources qu'il utilise avant sa fermeture.

---

# 4. Exigences non fonctionnelles

## NFR-01 — Performance

Le logiciel doit conserver une interface réactive durant son exécution.

---

## NFR-02 — Faible consommation de ressources

Le logiciel doit limiter sa propre consommation processeur et mémoire afin de ne pas perturber significativement le système observé.

---

## NFR-03 — Fiabilité

Le logiciel doit continuer à fonctionner malgré des erreurs ponctuelles de lecture des informations système lorsque celles-ci ne compromettent pas son exécution.

---

## NFR-04 — Maintenabilité

Le logiciel doit être conçu de manière modulaire afin de faciliter :

* les évolutions ;
* les corrections ;
* les tests ;
* la réutilisation des composants.

---

## NFR-05 — Lisibilité

Le code source doit privilégier :

* la clarté ;
* la cohérence ;
* la simplicité de compréhension.

---

## NFR-06 — Portabilité

Le logiciel est destiné aux systèmes Linux disposant du système de fichiers virtuel `/proc`.

Aucune compatibilité avec d'autres systèmes d'exploitation n'est exigée dans cette version.

---

## NFR-07 — Documentation

Le projet doit être accompagné d'une documentation permettant de comprendre :

* les besoins fonctionnels ;
* les choix d'architecture ;
* les contraintes techniques ;
* les perspectives d'évolution.

---

# 5. Hypothèses

Le logiciel suppose :

* un système d'exploitation Linux ;
* un terminal compatible avec les bibliothèques utilisées ;
* un accès en lecture aux informations système nécessaires.

---

# 6. Contraintes

Les contraintes techniques détaillées (langage, bibliothèques, système d'exploitation, architecture, performances, dépendances, etc.) sont décrites dans le document :

**02-Contraintes-techniques.md**

---

# 7. Hors périmètre

La version actuelle ne couvre pas :

* une interface graphique ;
* la supervision distante ;
* l'historisation des métriques ;
* l'export des données ;
* un système de plugins ;
* une architecture distribuée.

Ces fonctionnalités pourront être étudiées lors des évolutions futures.

---
# 8. Évolution des exigences

Les exigences définies dans ce document correspondent au périmètre fonctionnel actuellement visé par le projet.

Elles pourront évoluer au fil des versions afin d'accompagner l'enrichissement fonctionnel du logiciel, tout en assurant la cohérence avec les documents d'architecture et la feuille de route.

---

# 9. Traçabilité documentaire

Les exigences définies dans ce document sont prises en compte dans :

* **01-Specification-des-exigences-logicielles.md** pour les contraintes techniques ;
* **02-Contraintes-techniques.md** pour les choix de conception ;
* **03-Architecture.md** pour les évolutions prévues.

Ce document constitue la référence fonctionnelle du projet.
