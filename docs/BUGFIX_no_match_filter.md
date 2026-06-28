# Rapport de correction — Gestion de l'interface lors d'un filtre sans résultat

**Projet :** JICE-HTOP
**Version concernée :** 2.1

---

## Référence

- **Fichier corrigé :** `src/main.c`
- **Bloc corrigée :** `Ligne 178       if (nb_displayed == 0) {`
- **Sévérité :** Fonctionnelle (visible immédiatement par un utilisateur)

---

## Symptôme observé

## Description du dysfonctionnement

Lorsqu'un filtre ne correspond à aucun processus, l'application affiche correctement le message :

> *Aucun processus ne correspond au filtre.*

Cependant, dans cette situation :

* le bandeau d'état (*status bar*) n'est plus affiché ;
* la saisie utilisateur initiée par la touche `/` n'est plus réalisée dans le bandeau inférieur ;
* l'écho de la saisie apparaît à la suite du message affiché dans la zone principale.

Le comportement devient alors incohérent avec le reste de l'interface.

---

## Analyse

Le traitement du cas particulier `nb_displayed == 0` (Ligne 178) possédait sa propre boucle de gestion des événements.
Ligne 186
Cette branche exécutait directement :
* `refresh()`
* `getch()`
* `get_keypressed()`

puis quittait l'itération via :

```c
continue;
```

En conséquence, le flux normal de rafraîchissement de l'interface était interrompu.

Les traitements responsables de l'affichage du bandeau inférieur n'étaient plus exécutés.

Le comportement de l'interface différait donc selon que la liste des processus était vide ou non, ce qui introduisait une duplication de logique et une incohérence fonctionnelle.

---

## Cause racine

La gestion des événements était dupliquée dans un cas particulier (`nb_displayed == 0`).

Cette duplication contournait le cycle normal de rendu de l'interface.

Le défaut ne concernait pas le moteur de filtrage, mais l'organisation du flux d'exécution dans la boucle principale.

---

## Correctif appliqué

La branche spécifique contenait :

le morceau de code :
```c
	if (get_keypressed(key, &sort_mode, &scroll_offset, filter, &shared.running, &shared.mutex))
    	    break;	        
	    
	// on_keypressed(key, &sort_mode, &scroll_offset, filter);
	 continue;
```

a été supprimée.

Le cas *aucun processus affiché* suit désormais exactement le même cycle de traitement que les autres situations.

L'ensemble des opérations de rendu (affichage principal, bandeau d'état, saisie utilisateur) est ainsi exécuté de manière uniforme.

---

## Résultat

Après correction :

* le message *« Aucun processus ne correspond au filtre »* est toujours affiché ;
* le bandeau inférieur reste visible ;
* la saisie du filtre s'effectue au bon emplacement ;
* le défiltrage (`/` puis `Entrée`) fonctionne normalement ;
* le comportement de l'interface est désormais homogène quel que soit le nombre de processus affichés.

---

## Impact architectural

Cette correction simplifie la boucle principale en supprimant un traitement particulier devenu inutile.

Elle renforce le principe d'une **gestion centralisée de l'interface utilisateur**, dans laquelle tous les états de l'application suivent le même cycle de rendu et de gestion des événements.

Cette approche réduit les risques de divergence de comportement lors des évolutions futures.

