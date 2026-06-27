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
* nouvelles vues de supervision.
* ajout de nouvelles métriques système ;

Nouvelles métriques possibles :
```
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
```

---

# 3. Qualité logicielle

Le projet a vocation à intégrer progressivement des outils et pratiques de qualité :

* Intégration d'une analyse statique du code (cppcheck, clang-tidy)
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

# 5. Vision du projet

Au-delà de son objectif pédagogique initial, **JICE-HTOP** a vocation à constituer un projet démontrant une démarche professionnelle de développement logiciel, depuis l'expression des besoins jusqu'à la conception, la documentation, les pratiques de qualité et les perspectives d'évolution.

