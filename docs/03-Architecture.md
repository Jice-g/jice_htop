# 03 - Architecture logicielle

**Projet :** JICE-HTOP

**Version :** 2.0 

---

# 1. Objet du document

Ce document décrit comment JICE-HTOP est organisé en interne : ses composants, la façon dont ils communiquent, et les choix qui expliquent cette organisation.

Il répond à une question simple :

> Comment le système est-il organisé pour produire une supervision temps réel des processus Linux ?

Il se lit en complément de :

* **01 - Exigences logicielles**
* **02 - Contraintes techniques**
* **04 - Feuille de route**

---

# 2. Principes d'architecture

J'ai voulu que le projet reste guidé par quelques principes simples plutôt que par une liste de règles abstraites :

* séparer clairement les responsabilités entre modules
* limiter le couplage entre les modules
* centraliser l'état dans une seule structure
* rendre explicite toute synchronisation entre threads
* éviter les dépendances externes non nécessaires

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
├── debug/
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

Les modules ne se parlent pas directement entre eux. Ils passent tous par un *contexte partagé unique* :

**Contexte partagé (t_shared)**
* Liste des processus (t_process)
* Statistiques mémoire
* État d'exécution du programme
* Mutex de synchronisation

`t_process` est un type déclaré dans `sysproc.h`. Chaque entrée de processus contient notamment :
* PID
* nom
* consommation mémoire

`t_shared` centralise tout ce qui doit être partagé entre threads : les données système collectées, l'état du programme, et la synchronisation elle-même. C'est le seul point d'échange entre les composants — les modules ne s'appellent volontairement pas les uns les autres directement.

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
* **t_shared** : structure centrale contenant l'état complet du système observé.
* **Renderer** : lit l'état et produit l'affichage terminal.
* **Interface utilisateur** : gère les interactions clavier et les commandes utilisateur.

Aucun module ne dépend de l'implémentation interne des autres.

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

L'accès au contexte partagé `t_shared` est protégé par un mutex POSIX. Sans ça, deux problèmes se poseraient : des données incohérentes (un processus à moitié mis à jour pendant que l'affichage le lit) et un risque de conditions de course pouvant faire planter le programme. Le mutex garantit que l'affichage lit toujours un état stable.

---

# 5. Organisation des modules

Le projet est structuré en modules indépendants :

* interface utilisateur (ncurses, input)
* moteur d'affichage (renderer)
* collecte système (/proc)
* parsing des données processus
* contexte partagé (`t_shared`)
* gestion des threads
* synchronisation (mutex)
* utilitaires

Chaque module expose une API minimale et ne dépend que des structures publiques nécessaires.

---

# 6. Modèle de concurrence

L'architecture repose sur un modèle producteur / consommateur :

* **producteur** : thread de collecte système
* **consommateur** : thread d'affichage

Le découplage entre les deux est assuré par trois éléments : un contexte partagé unique (`t_shared`), une synchronisation par mutex, et un flag de contrôle d'exécution (`running`) qui permet d'arrêter proprement les deux threads. Ce modèle garde l'interface fluide même quand la collecte des données prend un peu de temps — l'affichage n'attend jamais directement le résultat de la lecture de `/proc`.

---

# 7. Choix de conception

Voici les décisions structurantes du projet, et pourquoi je les ai prises.

## Architecture modulaire

J'ai découpé le système en modules spécialisés (collecte, synchronisation, affichage, coordination) dès la version 1, avant même de savoir si j'irais vers le multithread. Ce choix s'est avéré payant : quand je suis passé d'une boucle séquentielle unique à deux threads séparés en v2.0, je n'ai quasiment pas eu à toucher au code métier existant. Seule la répartition des responsabilités entre threads a changé.

## Accès direct à `/proc`

J'aurais pu passer par une bibliothèque comme `libproc2`, `procps-ng` ou `glibtop` pour récupérer les infos système. J'ai préféré lire directement les pseudo-fichiers de `/proc` : ça évite une dépendance externe, ça me force à comprendre comment Linux expose réellement ces informations, et ça me laisse un contrôle total sur le parsing.

Le revers de la médaille : je dois parser les structures à la main, et le code est sensible aux éventuelles évolutions du format de `/proc`. Pour un projet de cette taille, j'ai jugé que le compromis en valait la peine.

## Interface ncurses

`ncurses` reste le choix le plus simple pour une interface texte interactive et portable sur Linux, sans réinventer la gestion du terminal (positionnement du curseur, couleurs, saisie clavier).

## Modèle multithread POSIX

Séparer la collecte système et l'affichage sur deux threads distincts m'a permis de garder une interface réactive même quand la lecture de `/proc` prend un peu de temps — ce qui n'était pas garanti avec l'ancienne boucle unique où l'affichage attendait la fin de chaque cycle de collecte.

## Contexte partagé `t_shared`

Plutôt qu'un simple modèle de données passé d'une fonction à l'autre, `t_shared` fait office de véritable point central d'échange entre les threads : il stocke les processus, les métriques système, l'état d'exécution du programme, et porte la synchronisation elle-même (mutex, flag `running`). Centraliser tout ça au même endroit m'évite d'avoir à synchroniser plusieurs structures séparées.

---

# 8. Gestion des erreurs

Le système adopte une stratégie de gestion explicite des erreurs :

* détection locale
* propagation contrôlée
* libération systématique des ressources
* arrêt propre en cas d'erreur critique

---

# 9. Évolutivité

L'architecture permet les évolutions suivantes :

* ajout de nouvelles métriques système
* extension des informations processus
* ajout de nouvelles vues
* optimisation des performances
* ajout de fonctionnalités avancées de supervision

---

# 10. Conclusion

JICE-HTOP repose sur un contexte partagé synchronisé (`t_shared`), alimenté par un thread de collecte et consommé par un thread d'affichage. Cette organisation sépare clairement les responsabilités, garde un flux de données lisible, et laisse un contrôle explicite sur la concurrence — tout en laissant de la place pour les évolutions à venir.
