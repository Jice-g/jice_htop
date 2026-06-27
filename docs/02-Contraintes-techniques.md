# 02 - Contraintes techniques

**Projet :** JICE-HTOP

**Version :** 1.0 (brouillon)

---

# 1. Objet du document

Ce document recense les contraintes techniques qui encadrent le développement de **JICE-HTOP**.

Il précise les hypothèses techniques, les dépendances, les contraintes de l'environnement d'exécution ainsi que les exigences de qualité qui influencent les choix de conception.

Les décisions d'architecture permettant de répondre à ces contraintes sont décrites dans le document **03-Architecture.md**.

---

# 2. Environnement cible

Le logiciel est destiné à être exécuté sur :

* un système d'exploitation Linux ;
* un terminal texte compatible avec la bibliothèque `ncurses` ;
* un environnement disposant du système de fichiers virtuel `/proc`.

Le projet n'a pas vocation à être compatible avec Windows ou macOS.

---

# 3. Technologies retenues

Le développement repose sur les technologies suivantes :

| Élément              | Choix                   |
| -------------------- | ----------------------- |
| Langage              | C                       |
| Compilateur          | GCC                     |
| Build                | Make                    |
| Interface            | ncurses                 |
| Threads              | POSIX Threads (pthread) |
| Informations système | `/proc`                 |
| Contrôle de version  | Git                     |
| Hébergement          | GitHub                  |

---

# 4. Contraintes système

Le projet est développé pour un environnement Linux.

À ce titre, il repose notamment sur :
- la disponibilité du système de fichiers virtuel `/proc` ;
- un terminal compatible avec `ncurses` ;
- les interfaces POSIX nécessaires à l'utilisation des threads.

Ces contraintes découlent directement du système d'exploitation ciblé.

---

# 5. Contraintes de développement

Le projet doit privilégier :

* une architecture modulaire ;
* une séparation claire des responsabilités ;
* des interfaces limitées entre modules ;
* un code lisible et documenté.

Les dépendances externes doivent rester limitées afin de conserver un projet léger et facilement compilable.

---

# 6. Contraintes de performance

Le logiciel doit :

* maintenir une interface fluide ;
* limiter sa propre consommation CPU ;
* limiter son empreinte mémoire ;
* éviter les traitements inutiles lors des rafraîchissements.

---

# 7. Contraintes de robustesse

Le logiciel doit :

* gérer correctement les erreurs système ;
* détecter les ressources indisponibles ;
* éviter les fuites mémoire ;
* garantir une libération correcte des ressources avant la fermeture.

---

# 8. Contraintes de qualité logicielle

Le développement vise notamment :

* la lisibilité du code ;
* la maintenabilité ;
* la modularité ;
* la facilité d'évolution ;
* la reproductibilité de la compilation.

À terme, ces objectifs pourront être accompagnés par des outils de contrôle qualité tels que :

* Valgrind ;
* AddressSanitizer ;
* UndefinedBehaviorSanitizer ;
* cppcheck ;
* clang-tidy ;
* GitHub Actions.

---
# Contraintes imposées :

* système Linux ;
* interface terminal ;
* langage C (lié au sujet initial).

# Contraintes de conception :

* architecture modulaire ;
* informations disponibles via /proc ;
* faible nombre de dépendances ;
* priorité à la maintenabilité ;
* séparation des responsabilités ;
* documentation systématique.

---

# 9. Limites connues

La version actuelle présente volontairement certaines limitations :

* compatibilité limitée aux systèmes Linux ;
* absence de supervision distante ;
* absence de persistance des données ;
* absence de système de plugins.

Ces limitations sont cohérentes avec le périmètre fonctionnel défini dans le document **01-Exigences-logicielles.md**.

---

# 10. Références documentaires

Ce document est à mettre en relation avec :

* **01-Exigences-logicielles.md**, qui définit les besoins fonctionnels ;
* **03-Architecture.md**, qui décrit les choix de conception retenus ;
* **04-Feuille-de-route.md**, qui présente les évolutions prévues.
