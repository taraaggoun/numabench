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

## Création des cgroups
echo "Vérification des librairies numactl et cgroup-tools..."
check_and_install "numactl"
check_and_install "cgroup-tools"

if [ ! -d /sys/fs/cgroup ]; then
    echo "Création du répertoire /sys/fs/cgroup..."
    mkdir -p /sys/fs/cgroup
fi

if [ ! -d /sys/fs/cgroup/cpuset ]; then
    echo "Création du répertoire /sys/fs/cgroup/cpuset..."
    mkdir -p /sys/fs/cgroup/cpuset
fi

if ! mountpoint -q /sys/fs/cgroup/cpuset; then
    echo "Montage du cgroup cpuset..."
    mount -t cgroup -o cpuset cpuset /sys/fs/cgroup/cpuset || {
        echo "Erreur lors du montage du cgroup cpuset."
        exit 1
    }
fi

echo "Création des cgroups pour les nœuds NUMA..."
mkdir -p /sys/fs/cgroup/cpuset/node0
mkdir -p /sys/fs/cgroup/cpuset/node1

echo "0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62" > /sys/fs/cgroup/cpuset/node0/cpuset.cpus
echo "0" > /sys/fs/cgroup/cpuset/node0/cpuset.mems
echo "1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63" > /sys/fs/cgroup/cpuset/node1/cpuset.cpus
echo "1" > /sys/fs/cgroup/cpuset/node1/cpuset.mems

echo 1 > /sys/fs/cgroup/cpuset/cpuset.cpu_exclusive
echo 1 > /sys/fs/cgroup/cpuset/cpuset.mem_exclusive

