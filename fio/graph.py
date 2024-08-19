import re
import matplotlib.pyplot as plt
import numpy as np

def extract_bandwidth(filename):
    """
    Extrait les valeurs de bande passante (bw) de plusieurs blocs dans un fichier de résultats fio.
    """
    bandwidths = []
    with open(filename, 'r') as file:
        content = file.read()
        # Trouver tous les blocs contenant la bande passante
        matches = re.findall(r'read:.*?bw=(\d+(\.\d+)?)MiB/s', content, re.DOTALL)
        for match in matches:
            bandwidths.append(float(match[0]))
    return bandwidths

def create_bandwidth_graph(local_file, remote_file, output_file):
    """
    Crée un graphique comparant les bandes passantes locales et distantes, incluant les moyennes et les écarts-types.
    """
    # Extraire les valeurs de bande passante
    local_bandwidths = extract_bandwidth(local_file)
    remote_bandwidths = extract_bandwidth(remote_file)
    
    # Calculer la moyenne et l'écart-type
    local_mean = np.mean(local_bandwidths) if local_bandwidths else 0
    local_std = np.std(local_bandwidths) if local_bandwidths else 0
    remote_mean = np.mean(remote_bandwidths) if remote_bandwidths else 0
    remote_std = np.std(remote_bandwidths) if remote_bandwidths else 0

    # Préparer les données pour le graphique
    tests = ['Local', 'Remote']
    means = [local_mean, remote_mean]
    std_devs = [local_std, remote_std]

    # Créer le graphique
    plt.figure(figsize=(12, 8))
    
    # Bar plot
    plt.bar(tests, means, yerr=std_devs, capsize=5, color=['blue', 'orange'], alpha=0.7)
    
    # Ajouter des labels et titre
    plt.xlabel('Type de Test')
    plt.ylabel('Bande Passante (MiB/s)')
    plt.title('Performance de Lecture - Local vs Distant')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    
    # Sauvegarder le graphique dans un fichier
    plt.savefig(output_file)
    plt.show()

def main():
    create_bandwidth_graph('fio/media/local_0', 'fio/media/remote_0', 'fio/media/results_0.png')
    # create_bandwidth_graph('fio/media/local_0', 'fio/media/remote_0', 'fio/media/results_0.png')
    # create_bandwidth_graph('fio/media/local_0', 'fio/media/remote_0', 'fio/media/results_0.png')
    # create_bandwidth_graph('fio/media/local_0', 'fio/media/remote_0', 'fio/media/results_0.png')
    # create_bandwidth_graph('fio/media/local_0', 'fio/media/remote_0', 'fio/media/results_0.png')

if __name__ == "__main__":
    main()