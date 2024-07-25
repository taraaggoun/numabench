#!/bin/bash

# Définir les versions des sources du noyau
KERNEL_VERSIONS=("linux-6.6.21_1" "linux-6.6.21_2" "linux-6.6.21_3" "linux-6.6.21_4")
BASE_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x"
TAR_FILE="linux-6.6.21.tar.xz"
DIR_NAME="linux-6.6.21"

# Fonction pour vérifier et installer un paquet
check_and_install() {
    PACKAGE=$1
    if dpkg -l | grep -q "^ii  $PACKAGE "; then
        echo "$PACKAGE est déjà installé."
    else
        echo "$PACKAGE n'est pas installé. Installation en cours..."
        apt install -y "$PACKAGE" || {
            echo "Erreur lors de l'installation de $PACKAGE."
            exit 1
        }
    fi
}

# Fonction pour nettoyer et compiler un noyau
compile_kernel() {
    VERSION=$1
    CGROUP=$2
    LOGFILE=$3
    CHRONO=$4

    echo "Compilation des sources $VERSION sur le cgroup $CGROUP..."
    cd "$VERSION" || {
        echo "Impossible de se déplacer dans le répertoire $VERSION."
        exit 1
    }

    # Nettoyer avant la compilation
    make clean || {
        echo "Erreur lors du nettoyage dans $VERSION."
        exit 1
    }

    # Chronométrer la compilation si demandé
    if [ "$CHRONO" = true ]; then
        { time cgexec -g cpuset:$CGROUP make -j$(nproc); } 2>> "$LOGFILE"
    else
        cgexec -g cpuset:$CGROUP make -j$(nproc) || {
            echo "Erreur lors de la compilation dans $VERSION."
            exit 1
        }
    fi

    cd .. || {
        echo "Impossible de revenir au répertoire parent."
        exit 1
    }
}

# Préparer l'environnement une seule fois
setup_environment() {
    # Télécharger les sources du noyau si ce n'est pas déjà fait
    if [ ! -f "$TAR_FILE" ]; then
        echo "Téléchargement des sources du noyau..."
        wget "$BASE_URL/$TAR_FILE" || {
            echo "Erreur lors du téléchargement des sources du noyau."
            exit 1
        }
    fi

    # Installer les outils nécessaires
    echo "Installation des outils nécessaires..."
    apt update
    apt install -y xz-utils gcc make bc build-essential libncurses-dev bison flex libssl-dev libelf-dev || {
        echo "Erreur lors de l'installation des outils nécessaires."
        exit 1
    }

    # Extraire les sources du noyau si ce n'est pas déjà fait
    if [ ! -d "$DIR_NAME" ]; then
        echo "Extraction des sources du noyau..."
        tar -xvf "$TAR_FILE" || {
            echo "Erreur lors de l'extraction des sources du noyau."
            exit 1
        }
    fi

    # Préparer les répertoires pour chaque version
    for VERSION in "${KERNEL_VERSIONS[@]}"; do
        if [ -d "$VERSION" ]; then
            echo "Le répertoire $VERSION existe déjà."
        else
            echo "Création du répertoire $VERSION et copie des sources..."
            cp -r "$DIR_NAME" "$VERSION" || {
                echo "Erreur lors de la copie des sources vers $VERSION."
                exit 1
            }

            # Se déplacer dans le répertoire des sources
            cd "$VERSION" || {
                echo "Impossible de se déplacer dans le répertoire $VERSION."
                exit 1
            }

            # Copier la configuration actuelle du noyau
            cp /boot/config-$(uname -r) .config || {
                echo "Erreur lors de la copie de la configuration du noyau."
                exit 1
            }

            # Préparer la configuration
            make olddefconfig || {
                echo "Erreur lors de la préparation de la configuration du noyau."
                exit 1
            }

            # Revenir au répertoire parent
            cd .. || {
                echo "Impossible de revenir au répertoire parent."
                exit 1
            }
        fi
    done

    # Supprimer le répertoire source et le fichier tar.xz
    echo "Suppression du répertoire source et du fichier tar.xz..."
    rm -rf "$DIR_NAME" "$TAR_FILE" || {
        echo "Erreur lors de la suppression du répertoire source et du fichier tar.xz."
        exit 1
    }
}

# Vérifier et installer les librairies numactl et cgroup-tools
setup_cgroups() {
    echo "Vérification des librairies numactl et cgroup-tools..."
    check_and_install "numactl"
    check_and_install "cgroup-tools"

    # Créer le répertoire /sys/fs/cgroup s'il n'existe pas
    if [ ! -d /sys/fs/cgroup ]; then
        echo "Création du répertoire /sys/fs/cgroup..."
        mkdir -p /sys/fs/cgroup
    fi

    # Créer le répertoire /sys/fs/cgroup/cpuset s'il n'existe pas
    if [ ! -d /sys/fs/cgroup/cpuset ]; then
        echo "Création du répertoire /sys/fs/cgroup/cpuset..."
        mkdir -p /sys/fs/cgroup/cpuset
    fi

    # Monter le cgroup cpuset s'il n'est pas déjà monté
    if ! mountpoint -q /sys/fs/cgroup/cpuset; then
        echo "Montage du cgroup cpuset..."
        mount -t cgroup -o cpuset cpuset /sys/fs/cgroup/cpuset || {
            echo "Erreur lors du montage du cgroup cpuset."
            exit 1
        }
    fi

    # Créer les cgroups pour chaque nœud NUMA
    echo "Création des cgroups pour les nœuds NUMA..."
    mkdir -p /sys/fs/cgroup/cpuset/node0
    mkdir -p /sys/fs/cgroup/cpuset/node1

    # Configurer les cgroups avec les plages de cœurs correctes
    echo "0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62" > /sys/fs/cgroup/cpuset/node0/cpuset.cpus
    echo "0" > /sys/fs/cgroup/cpuset/node0/cpuset.mems

    echo "1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63" > /sys/fs/cgroup/cpuset/node1/cpuset.cpus
    echo "1" > /sys/fs/cgroup/cpuset/node1/cpuset.mems

    # Assurez-vous que les cgroups sont exclusifs
    echo 1 > /sys/fs/cgroup/cpuset/cpuset.cpu_exclusive
    echo 1 > /sys/fs/cgroup/cpuset/cpuset.mem_exclusive

    echo "Les cgroups ont été configurés correctement."
}

# Préparer l'environnement et les cgroups une seule fois
setup_environment
setup_cgroups

# Boucle de 10 itérations
for i in {1..10}; do
    echo "Itération $i..."

    # Nettoyage du cache
    echo "Nettoyage du cache..."
    sync
    echo 1 > /proc/sys/vm/drop_caches

    echo "Chargement en mémoire des fichiers"
    compile_kernel "linux-6.6.21_1" "node0" "/dev/null" false
    compile_kernel "linux-6.6.21_2" "node1" "/dev/null" false
    compile_kernel "linux-6.6.21_3" "node0" "/dev/null" false
    compile_kernel "linux-6.6.21_4" "node1" "/dev/null" false

    # Nettoyage des répertoires après les compilations séquentielles
    echo "Nettoyage des répertoires après les compilations séquentielles..."
    for VERSION in "${KERNEL_VERSIONS[@]}"; do
        cd "$VERSION" || {
            echo "Impossible de se déplacer dans le répertoire $VERSION."
            exit 1
        }
        make clean || {
            echo "Erreur lors du nettoyage dans $VERSION."
            exit 1
        }
        cd .. || {
            echo "Impossible de revenir au répertoire parent."
            exit 1
        }
    done

    echo "Compilation parallèle des noyaux"
    (
        compile_kernel "linux-6.6.21_1" "node0" "../local_1" true &
        compile_kernel "linux-6.6.21_2" "node1" "../local_2" true
        wait
    )

    # Nettoyage après la première compilation parallèle
    echo "Nettoyage des répertoires après la première compilation parallèle..."
    for VERSION in "linux-6.6.21_1" "linux-6.6.21_2"; do
        cd "$VERSION" || {
            echo "Impossible de se déplacer dans le répertoire $VERSION."
            exit 1
        }
        make clean || {
            echo "Erreur lors du nettoyage dans $VERSION."
            exit 1
        }
        cd .. || {
            echo "Impossible de revenir au répertoire parent."
            exit 1
        }
    done

    (
        compile_kernel "linux-6.6.21_3" "node1" "../remote_1" true &
        compile_kernel "linux-6.6.21_4" "node0" "../remote_2" true
        wait
    )

    # Nettoyage après la deuxième compilation parallèle
    echo "Nettoyage des répertoires après la deuxième compilation parallèle..."
    for VERSION in "linux-6.6.21_3" "linux-6.6.21_4"; do
        cd "$VERSION" || {
            echo "Impossible de se déplacer dans le répertoire $VERSION."
            exit 1
        }
        make clean || {
            echo "Erreur lors du nettoyage dans $VERSION."
            exit 1
        }
        cd .. || {
            echo "Impossible de revenir au répertoire parent."
            exit 1
        }
    done

    echo "Itération $i terminée."
done

echo "Script terminé."
