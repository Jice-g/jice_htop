# 03 - Architecture logicielle

**Projet :** JICE-HTOP
**Version :** 1.0 

---

# 1. Objet du document

Ce document décrit l’architecture logicielle du projet **JICE-HTOP**.

Il présente l’organisation interne du système, ses composants principaux, ainsi que les décisions structurantes ayant guidé sa conception.

Il répond à la question :

> Comment le système est-il organisé pour produire une supervision temps réel des processus Linux ?

Ce document est complémentaire de :

* **01 - Exigences logicielles**
* **02 - Contraintes techniques**
* **04 - Feuille de route**

---

# 2. Principes d’architecture

L’architecture repose sur les principes suivants :

* séparation des responsabilités
* modularité fonctionnelle
* faible couplage entre modules
* centralisation de l’état applicatif
* synchronisation explicite des accès concurrents
* minimisation des dépendances externes

**Architecture modulaire**

main.c
Coordonne l'application.

sysproc.c
Collecte les informations système.

threadshare.c
Assure la synchronisation entre les threads.

uiwin.c
Construit l'interface ncurses.

Organisation du dépôt :
```
jice_htop/
├── src/
│   ├── main.c
│   ├── sysproc.c
│   ├── threadshare.c
│   └── uiwin.c
│
├── include/
│   ├── sysproc.h
│   ├── threadshare.h
│   └── uiwin.h
│
├── docs/
│
├── test/
│
├── assets/
│
├── Makefile
├── CHANGELOG.md
└── README.md

```

---

## 2.1 Modèle de communication

Les modules ne communiquent pas directement entre eux.

Ils interagissent exclusivement via un *contexte partagé unique* :

**Contexte partagé (t_shared)**
* Liste des processus (t_process)
* Statistiques mémoire
* État d'exécution du programme
* Mutex de synchronisation

t_process est un type déclaré dans le sysproc.h
Chaque entrée de processus contient notamment :
* PID
* nom
* consommation mémoire

Ce contexte centralise :

* les données système collectées
* l’état du programme
* la synchronisation inter-thread

Il constitue le point unique d’échange entre les composants.

---

# 3. Vue logique — architecture fonctionnelle

Cette vue décrit la séparation des responsabilités fonctionnelles.

```text id="logical-view"
+------------------------+
| Interface utilisateur  |
+-----------+------------+
            |
            v
+------------------------+
|  Couche affichage      |
+-----------+------------+
            |
            v
+------------------------+
|  Contexte partagé      |
+-----------+------------+
            ^
            |
+------------------------+
| Collecteur système     |
|  (parsing /proc)       |
+------------------------+

```

### Description

* **Collecteur système** : récupère et met à jour les données issues de `/proc`.
* **t_shared** : structure centrale contenant l’état complet du système observé.
* **Renderer** : lit l’état et produit l’affichage terminal.
* **Interface utilisateur** : gère les interactions clavier et les commandes utilisateur.

Aucun module ne dépend de l’implémentation interne des autres.

---

# 4. Vue générale

![Aperçu de JICE-HTOP - Mermaid Architect](assets/img/mermaid_Architect-jice_htop.png)

**Flux de données**

```
Linux (/proc)
      │
        ▼
Lecture des pseudo-fichiers
      │
        ▼
Parsing
      │
        ▼
Construction des t_process
      │
        ▼
Mise à jour du contexte partagé (t_shared)
      │
        ▼
Affichage ncurses
```

### Synchronisation

L’accès au contexte partagé `t_shared` est protégé par un mutex POSIX afin de garantir :

* la cohérence des données
* l’absence de conditions de course
* la stabilité de l’affichage

---

# 5. Organisation des modules

Le projet est structuré en modules indépendants :

* interface utilisateur (ncurses, input)
* moteur d’affichage (renderer)
* collecte système (/proc)
* parsing des données processus
* contexte partagé (`t_shared`)
* gestion des threads
* synchronisation (mutex)
* utilitaires

Chaque module expose une API minimale et ne dépend que des structures publiques nécessaires.

---

# 6. Modèle de concurrence

L’architecture repose sur un modèle producteur / consommateur :

* **producteur** : thread de collecte système
* **consommateur** : thread d’affichage

Le découplage est assuré par :

* un contexte partagé unique (`t_shared`)
* une synchronisation par mutex
* un flag de contrôle d’exécution (`running`)

Ce modèle permet de maintenir une interface fluide malgré des opérations de collecte potentiellement coûteuses.

---

# 7. Décisions d’architecture

---

## Décision n°1 — Architecture modulaire

Le système est découpé en modules spécialisés afin de limiter les dépendances et faciliter la maintenance.

---

## Décision n°2 — Accès direct à /proc

Le projet fait le choix d’accéder directement aux informations exposées par le système de fichiers virtuel `/proc`.

### Motivations

* réduction des dépendances externes
* compréhension des mécanismes internes Linux
* contrôle total sur le parsing des données
* architecture légère et transparente

### Alternatives

* libproc2
* procps-ng
* glibtop

### Conséquences

**Avantages**

* maîtrise complète du traitement
* absence de dépendances lourdes
* compréhension fine du système

**Inconvénients**

* parsing manuel des structures
* sensibilité aux évolutions de `/proc`

---

## Décision n°3 — Interface ncurses

L’interface repose sur ncurses afin de fournir une TUI légère et portable.

---

## Décision n°4 — Modèle multithread POSIX

Le recours aux threads permet de séparer :

* acquisition des données système
* restitution de l’interface utilisateur

---

## Décision n°5 — Contexte partagé `t_shared`

Le système repose sur une structure unique `t_shared` servant de point central d’échange.

### Rôle

* stockage des processus
* stockage des métriques système
* état d’exécution du programme
* synchronisation inter-thread

### Nature architecturale

`t_shared` constitue un **contexte applicatif partagé synchronisé**, et non un simple modèle de données.

---

# 8. Gestion des erreurs

Le système adopte une stratégie de gestion explicite des erreurs :

* détection locale
* propagation contrôlée
* libération systématique des ressources
* arrêt propre en cas d’erreur critique

---

# 9. Évolutivité

L’architecture permet les évolutions suivantes :

* ajout de nouvelles métriques système
* extension des informations processus
* ajout de nouvelles vues
* optimisation des performances
* ajout de fonctionnalités avancées de supervision

---

# 10. Conclusion

JICE-HTOP repose sur une architecture centrée sur un contexte partagé synchronisé (`t_shared`), alimenté par un thread de collecte et consommé par un thread d’affichage.

Cette organisation permet :

* une séparation claire des responsabilités
* une bonne lisibilité du flux de données
* un contrôle explicite de la concurrence
* une base évolutive pour des fonctionnalités futures

Cette architecture privilégie la lisibilité, le découplage des responsabilités et la maîtrise explicite des mécanismes de concurrence, tout en conservant une base légère et facilement extensible.

---
