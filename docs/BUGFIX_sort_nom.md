# Rapport de correction — Bug tri alphabétique par NOM

## Référence

- **Fichier corrigé :** `src/sysproc.c`
- **Fonction corrigée :** `compare_by_name()`
- **Sévérité :** Fonctionnelle (visible immédiatement par un utilisateur)

---

## Symptôme observé

En mode tri par **NOM** (touche `n`), la liste affichait deux groupes distincts
au-dessus de la liste alphabétique principale :

```
[1] Noms commençant par une MAJUSCULE  (ex: NetworkManager, Xorg)
[2] Noms entre CROCHETS               (ex: [kthreadd], [rcu_gp])   ← lot parasite
[3] Noms en minuscules                (ex: bash, chrome, sshd)      ← liste principale
```

Les modes tri PID et MEM n'étaient pas affectés.

---

## Analyse de la cause racine

### Les threads noyau Linux

Sur Linux, les threads du noyau (kernel threads) apparaissent dans `/proc` avec des
noms encadrés de crochets dans certains contextes : `[kthreadd]`, `[rcu_gp]`,
`[migration/0]`, `[kworker/0:0H]`, etc.

> **Note :** Le fichier `/proc/[PID]/comm` ne contient pas les crochets directement —
> ceux-ci apparaissent dans `/proc/[PID]/status` (champ `Name:`) ou dans la sortie
> de `ps aux`. Selon le noyau et la version du système, certains threads noyau peuvent
> exposer ces caractères dans `comm`. Le bug se manifeste dès lors qu'un nom de
> processus commence par un caractère dont la valeur ASCII est en dehors de la plage
> `[a-z]` ou `[A-Z]`.

### La valeur ASCII en cause

| Caractère | Valeur ASCII |
|-----------|-------------|
| `Z`       | 90          |
| `[`       | **91**      |
| `a`       | 97          |

Le crochet ouvrant `[` (ASCII 91) se trouve **entre** les majuscules et les
minuscules dans la table ASCII.

### Comportement de `strcmp()` brut

`strcmp()` compare les chaînes octet par octet selon leur valeur ASCII. Résultat :

```
strcmp("[kthreadd]", "bash")  → négatif  ('[' < 'b')  → [kthreadd] avant bash ✓ apparent
strcmp("[kthreadd]", "Bash")  → positif  ('[' > 'B')  → [kthreadd] après Bash ✗ incohérent
```

La liste apparaissait donc dans l'ordre :
1. Noms à majuscule (`A`–`Z`, ASCII 65–90)
2. Noms entre crochets (`[`, ASCII 91)
3. Noms en minuscule (`a`–`z`, ASCII 97+)

---

## Correction appliquée

### Stratégie

Ajouter une fonction auxiliaire `skip_non_alpha()` qui avance un pointeur
jusqu'au premier caractère alphabétique du nom. Le comparateur utilise ensuite
`strcasecmp()` sur les chaînes normalisées, ce qui :

- intègre `[kthreadd]` dans le groupe des `k` ;
- intègre `[migration/0]` dans le groupe des `m` ;
- rend le tri insensible à la casse (bonus : `Bash` et `bash` sont adjacents).

### Code avant correction

```c
// Tri décroissant le nom du process name :
static int compare_by_name(const void *a, const void *b) {
    const t_process *pa = a;
    const t_process *pb = b;
    return strcmp(pa->name, pb->name);    // croissant
}
```

### Code après correction

```c
// Pointeur sur le premier caractère alphabétique d'un nom de processus.
// Les threads noyau Linux ont des noms entre crochets : [kthreadd], [rcu_gp]...
// Le crochet '[' vaut ASCII 91, entre 'Z'(90) et 'a'(97) : avec un strcmp brut,
// ces noms se trient entre les noms à majuscule et les noms en minuscule,
// formant un "lot" parasite visible au milieu de la liste alphabétique.
// On saute les caractères non alphabétiques en tête pour un tri naturel.
static const char *skip_non_alpha(const char *s)
{
    while (*s && !isalpha((unsigned char)*s))
        s++;
    return (*s) ? s : s - 1;
}

// Tri croissant sur le nom du processus, insensible aux crochets et caractères spéciaux en tête :
static int compare_by_name(const void *a, const void *b) {
    const t_process *pa = a;
    const t_process *pb = b;
    return strcasecmp(skip_non_alpha(pa->name), skip_non_alpha(pb->name));
}
```

### Résultat après correction

```
AVANT (strcmp brut)          APRÈS (skip_non_alpha + strcasecmp)
─────────────────────        ──────────────────────────────────
NetworkManager               bash
Xorg                         Chrome
[kthreadd]          →        dbus-daemon
[migration/0]       →        [kthreadd]        (trié avec les k)
[rcu_gp]            →        [migration/0]     (trié avec les m)
bash                         NetworkManager
dbus-daemon                  python3
chrome                       [rcu_gp]          (trié avec les r)
python3                      sshd
sshd                         systemd
```

---

## Fonctions C impliquées

| Fonction | En-tête | Rôle |
|----------|---------|------|
| `isalpha(unsigned char)` | `<ctype.h>` | Détecte un caractère alphabétique |
| `strcasecmp(s1, s2)` | `<string.h>` | Comparaison insensible à la casse (POSIX) |

Le cast `(unsigned char)` dans `isalpha()` est obligatoire : passer un `char`
signé avec une valeur négative (caractères étendus > 127) provoquerait un
comportement indéfini selon la norme C.

---

## Autres causes envisagées et écartées

| Hypothèse | Verdict |
|-----------|---------|
| `\n` résiduel dans le nom | Écarté — `strcspn()` l'élimine déjà à la lecture |
| Chaîne non terminée par `\0` | Écarté — `calloc()` initialise à zéro et `fgets()` ajoute `\0` |
| Mauvais usage de `strcmp()` | Partiellement vrai — `strcmp()` est correct mais inadapté au contexte |
| Caractères non imprimables | **Cause principale** — `[` (ASCII 91) perturbe l'ordre ASCII brut |

