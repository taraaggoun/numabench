#!/bin/bash

# Définir les versions et chemins
KERNEL_VERSION="linux-6.6.21"
TAR_FILE="${KERNEL_VERSION}.tar.xz"
DIR_NAME="${KERNEL_VERSION}"
BASE_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x"

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

# Fonction pour télécharger et extraire les sources du noyau
setup_kernel_sources() {
    # Télécharger les sources du noyau si ce n'est pas déjà fait
    if [ ! -f "$TAR_FILE" ]; then
        echo "Téléchargement des sources du noyau..."
        wget "$BASE_URL/$TAR_FILE" || {
            echo "Erreur lors du téléchargement des sources du noyau."
            exit 1
        }
    fi

    # Extraire les sources du noyau si ce n'est pas déjà fait
    if [ ! -d "$DIR_NAME" ]; then
        echo "Extraction des sources du noyau..."
        tar -xvf "$TAR_FILE" || {
            echo "Erreur lors de l'extraction des sources du noyau."
            exit 1
        }
    fi

    cd "$DIR_NAME" || {
        echo "Impossible de se déplacer dans le répertoire $DIR_NAME."
        exit 1
    }

    cp /boot/config-$(uname -r) .config || {
        echo "Erreur lors de la copie de la configuration du noyau."
        exit 1
    }

    cd .. || {
        echo "Impossible de revenir au répertoire parent."
        exit 1
    }
}

# Fonction pour configurer les cgroups
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

    # Créer les cgroups pour chaque nœud
    echo "Création des cgroups pour les nœuds..."
    mkdir -p /sys/fs/cgroup/cpuset/node0
    mkdir -p /sys/fs/cgroup/cpuset/node1

    # Configurer les cgroups avec les plages de cœurs correctes
    echo "0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62" > /sys/fs/cgroup/cpuset/node0/cpuset.cpus
    echo "0" > /sys/fs/cgroup/cpuset/node0/cpuset.mems

    echo "1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63" > /sys/fs/cgroup/cpuset/node1/cpuset.cpus
    echo "0" > /sys/fs/cgroup/cpuset/node1/cpuset.mems

    # Assurez-vous que les cgroups sont exclusifs
    echo 1 > /sys/fs/cgroup/cpuset/cpuset.cpu_exclusive
    echo 1 > /sys/fs/cgroup/cpuset/cpuset.mem_exclusive

    echo "Les cgroups ont été configurés correctement."
}

# Fonction pour compiler le noyau
compile_kernel() {
    CGROUP=$1
    LOGFILE=$2

    echo "Compilation du noyau sur le cgroup $CGROUP..."
    cd "$DIR_NAME" || {
        echo "Impossible de se déplacer dans le répertoire $DIR_NAME."
        exit 1
    }

    # Nettoyer avant la compilation
    make clean || {
        echo "Erreur lors du nettoyage dans $DIR_NAME."
        exit 1
    }

    # Chronométrer la compilation
    { time cgexec -g cpuset:$CGROUP make -j$(nproc); } 2>> "$LOGFILE"

    cd .. || {
        echo "Impossible de revenir au répertoire parent."
        exit 1
    }
}

# Préparer l'environnement
setup_kernel_sources
setup_cgroups

for i in {1..10}; do
    echo "Itération $i..."

    # Vider le cache des pages
    echo "Vidage du cache des pages..."
    sync
    echo 1 > /proc/sys/vm/drop_caches

    echo "Compilation et chronométrage..."

    # Compilation et chronométrage sur le nœud 0
    compile_kernel "node0" "../load_1"
    compile_kernel "node1" "../make_1"
done
