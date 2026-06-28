# JICE_HTOP
**Moniteur système Linux interactif développé en langage C avec ncurses.**


JICE-HTOP est un projet personnel inspiré de *htop*, conçu pour approfondir le développement système sous Linux et démontrer une démarche professionnelle de conception logicielle.

> **Objectif :** mettre en œuvre une architecture modulaire, un modèle concurrent basé sur POSIX Threads et une collecte des informations système via `/proc`.

---

**Contexte**

Ce projet a été initié dans le cadre de mon admission directe en *3ᵉ année Bachelor IT – Développeur Logiciel & DevOps* à **La Plateforme** (Marseille).

Il est désormais développé comme un projet logiciel PERSONNEL documenté afin d'accompagner ma montée en compétence et ma recherche d'alternance en Développement Logiciel / DevOps.

---

## Aperçu

![Interface principale](assets/img/screen-shot-01.png)

---

## Fonctionnalités

### Supervision système

- mémoire RAM (totale, utilisée, disponible, pourcentage)
- empreinte mémoire du processus jice_htop lui-même
- nombre de processus actifs


### Gestion des processus

- lecture du système `/proc`
- liste des processus actifs
- tri par PID, nom ou mémoire
- filtrage interactif
- rafraîchissement temps réel

### Interface utilisateur

- interface texte avec **ncurses**
- navigation clavier
- affichage plein écran
- scrollbar proportionnelle
- interface réactive grâce au multithreading

---

## Compétences mises en œuvre

| Domaine | Technologies |
|----------|--------------|
| Langage | C |
| Système | Linux, `/proc` |
| Interface | ncurses |
| Concurrence | POSIX Threads, mutex |
| Architecture | Modulaire |
| Mémoire | Allocation dynamique |
| Build | GCC, Make |
| Versioning | Git, GitHub |
---

## Architecture

Le projet repose sur une séparation stricte des responsabilités :

- acquisition des données système ;
- synchronisation des données partagées ;
- affichage de l'interface utilisateur.

### Organisation du dépôt

```text
jice_htop/
├── src/
├── include/
├── docs/
├── test/
├── assets/
├── Makefile
├── CHANGELOG.md
└── README.md
```

### Vue d'ensemble


![Interface principale](assets/img/mermaid_Architect-jice_htop_macro.png)

Le cœur de l'application repose sur un **contexte partagé (`t_shared`)** synchronisé par mutex entre un thread de collecte et le thread principal chargé de l'affichage.

Cette architecture permet de maintenir une interface fluide tout en limitant les sections critiques.

---

## Choix de conception

### Accès direct à `/proc`

Le projet choisit de parser directement les pseudo-fichiers du noyau Linux plutôt que d'utiliser une bibliothèque spécialisée.

Ce choix permet :

- une meilleure compréhension des mécanismes internes de Linux ;
- une réduction des dépendances externes ;
- une maîtrise complète du traitement des données.

### Architecture multithread

La collecte système est exécutée dans un thread dédié.

Le thread principal reste exclusivement responsable de l'interface utilisateur.

Les échanges sont réalisés via un contexte partagé protégé par mutex.

---

## Documentation

Le dépôt est accompagné d'une documentation technique décrivant la démarche d'ingénierie du projet.

- [01 - Exigences logicielles](docs/01-Exigences-logicielles.md)
- [02 - Contraintes techniques](docs/02-Contraintes-techniques.md)
- [03 - Architecture logicielle](docs/03-Architecture.md)
- [04 - Feuille de route](docs/04-Feuille-de-route.md)

Consulter le dossier **docs/**.

---

## Compilation

Installation de la dépendance :

```bash
sudo apt install libncurses-dev
```

Compilation :

```bash
make
```

Exécution :

```bash
./jice_htop
```

---

## Commandes

| Touche |      Action           |
|--------|-----------------------|
| q, Q   |      Quitter          |
| p, P   |     Tri par PID       |
| n, N   |     Tri par nom       |
| m, M   |     Tri par mémoire   |
| ↑ ↓   |     Défilement        |
|  /     | Filtrer les processus |

---

## Roadmap

### Fonctionnalités

- [ ] Arrêt ("kill") d'un processus
- [ ] Nouvelles métriques système
- [ ] Amélioration du moteur de rendu

**Affichage**
- utilisation CPU
- mémoire Swap
- uptime

### Qualité logicielle

- [ ] Intégration de `cppcheck`
- [ ] Intégration de `clang-tidy`
- [ ] Pipeline GitHub Actions
- [ ] Couverture de tests
- [ ] Choix d'une licence Open Source
---

## Qualité logicielle

Le projet est développé dans une démarche de qualité logicielle visant à produire un code robuste, lisible et maintenable.

Les vérifications réalisées incluent notamment :

- compilation sans avertissement (`-Wall -Wextra -Werror`)
- analyse mémoire avec **Valgrind (Memcheck)**
- vérification de l'exécution avec **AddressSanitizer (ASan)**
- détection des comportements indéfinis avec **UndefinedBehaviorSanitizer (UBSan)**

Les analyses réalisées n'ont mis en évidence **aucune fuite mémoire imputable au code applicatif** (`definitely lost : 0`).

L'intégration d'outils complémentaires (analyse statique et intégration continue) est prévue dans de prochaines évolutions du projet.


## Auteur

**Jean-Christophe Gerace**   *@jicé*

Projet personnel développé dans le cadre de ma spécialisation en Développement Logiciel & DevOps.


