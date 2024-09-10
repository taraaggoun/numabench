# Optimisation des processus selon les données en mémoire

# Sommaire
 - [Présentation](#Présentation)
 - [Installation](#Installation)
 - [Utilisation](#Utilisation)
 - [Fonctionnalité](#Fonctionnalité)
 - [Perspective](#Perspective)

# Présentation
Le but de ce projet est d'optimiser les entrées/sorties des fichiers sur une architecture NUMA (Non-Uniform Memory Access), en déplaçant un processus vers un cœur situé sur le nœud où les fichiers qu'il utilise sont chargés en mémoire.

## Lien vers le projet
Vous pouvez consulter et télécharger ce projet à partir du dépôt suivant :  [https://github.com/taraaggoun/numabench.git](https://github.com/taraaggoun/numabench.git).

# Installation
Ce projet nécessite l'installation des paquets suivants :
- libnuma-dev
- fio

Il a été développé avec la version [6.6.21](https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.6.21.tar.xz) du noyau Linux.

# Utilisation

## Patch
Dans le répertoire `patch`, se trouvent 4 patchs du noyau Linux (ainsi que leurs versions inverses) qui affectent un nœud NUMA à un processus en fonction de l'utilisation de fichiers déjà chargés en mémoire sur ce nœud.

- Deux des patchs appliquent l'optimisation lors de l'ouverture du fichier (préfixe `open`), en vérifiant si le fichier est déjà chargé en mémoire. Les deux autres effectuent l'optimisation à l'exécution (préfixe `exec`), en vérifiant si les paramètres sont chargés en mémoire.
- Deux des patchs forcent le processus à changer de nœud NUMA (suffixe `force`). Les deux autres proposent simplement un nœud a l'ordonnanceur (suffixe `try`). Il décidera ou non de basculer le processus de noeud.

### Compilation et Exécution
L'optimisation au niveau de l'ouverture de fichier est protégée par un ioctl. Voici les étapes pour l'activer :
```bash=
cd ioctl
make
insmod openctl.ko
dmesg
mknod /dev/openctl c [major] 0
```
Le numero major est recupéré par la comande `dmesg`

## Benchmark
Ce projet dispose de 3 benchmarks différents :

1. Numabench : Un benchmark qui mesure le temps de lecture/écriture d’un fichier en fonction de la localité des pages du fichier et du buffer de lecture/écriture. Il propose 4 configurations :
    - LL : Les pages et le buffer sont situés sur le même nœud que le processus.
    - LD : Les pages sont locales au processus, mais le buffer est sur un nœud distant.
    - DL : Les pages sont distantes au processus, mais le buffer est sur le nœud local.
    - DD : Les pages et le buffer sont situés sur un nœud distant par rapport au processus.

2. Fio : Ce programme charge un fichier en mémoire, puis exécute la commande `fio` soit sur un nœud local, soit sur un nœud distant par rapport aux pages chargées dans le page cache.

3. Grep : Ce programme charge un fichier en mémoire, puis exécute la commande `grep` depuis un nœud local ou distant par rapport aux pages présentes dans le page cache.

### Compilation et Exécution
```bash=
cd [nom du test]
make
```
#### Numabench :
-   Pour toutes les configuration sauf exec 
    ```bash=
    ./numabench.sh [num_config]
    ```

- Pour les configuration exec
    ```bash=
    cd exec
    make
    ./run_exec [num_config]
    ```

#### Fio :
```bash=
./run_fio [num_config]
```
#### Grep :
```bash=
./run_grep [num_config]
```

Il est possible de compiler tout les répertoire depuis la racine avec la commande `make` ou un seul sous-répertoire avec `make [nom du sous répertoire]`.
Les fichiers contenant les résultat sont sauvegarder dans le répertoire `[nom du test]/media`
Pour généré les graph il suffit d'exécuté la commande `python3 graph.py`

# Fonctionnalité
## configuration
Les benchmarks sont exécutés sur 6 configurations différentes :
- Témoin (0) : Le processus est lancé sur un nœud et est forcé de rester dessus.
- Linux (1) : Le processus est lancé sur un nœud et pourra en changer durant son exécution.
- Open_force (2) : Le processus est lancé sur un nœud. À chaque ouverture de fichier, si le fichier est chargé en mémoire, le processus sera forcé de s'exécuter sur le nœud où il est chargé.
- Open_try (3) : Le processus est lancé sur un nœud. À chaque ouverture de fichier, si le fichier est chargé en mémoire, il sera proposé a l'ordonnanceur de s'exécuter sur le nœud où il est chargé.
- Exec_force (4) : Le processus est lancé sur un nœud. À l'exécution, le processus sera forcé de s'exécuter sur le nœud où le plus de paramètres sont chargés en mémoire.
- Exec_try (5) : Le processus est lancé sur un nœud. À l'exécution, il sera proposé a l'ordonnanceur que le processus s'exécute sur le nœud où le plus de paramètres sont chargés en mémoire.

## Résultats
### Configuration de la machine de test
Les expérimentations ont été réalisées sur une machine comportant 2 noeuds NUMA, avec les caractéristiques suivantes :
- Processeur : 2 x Intel Xeon Silver 4410Y @ 3.90GHz (24 cœurs / 48 threads)
- Mémoire : 251 Go de RAM (125 + 126 Go)
- Système d'exploitation : Debian 12 avec noyau 6.6.21 (x86_64)

Voici le résultat de la commande `lstopo` :
![quetzal_lstopo](numabench/media/quetzal_lstopo.png)

### Numabench
| ![quetzal_read_all](numabench/media/graph/quetzal_read_all.png) | ![quetzal_write_all](numabench/media/graph/quetzal_write_all.png) |
|------------------------|------------------------|

Il n'y a pas de valeurs pour les configurations `exec` dans les catégories `LD` et `DD`, car le processus est déplacé de cœur à l'exécution, et par défaut, l'allocation de mémoire anonyme se fait localement.

### Fio
![graph_all](fio/media/graph_all.png)

### Grep
![graph_all](grep/media/graph_all.png)

# Perspective
Voici plusieurs perspectives pour améliorer notre approche :

- Dans notre réalisation, à chaque ouverture de fichier, nous recherchons le nombre de pages chargées en mémoire sur chaque nœud pour le fichier. Une amélioration possible serait d'utiliser un compteur mis à jour chaque fois qu'une page est ajoutée ou retirée du page cache.
- Dans l'optimisation au niveau de l'exécution, à chaque exécution d'un exécutable, nous vérifions tous les paramètres pour déterminer s'ils sont des fichiers. Une amélioration possible serait d'annoter l'exécutable afin qu'il enregistre s'il utilise des fichiers en paramètres.