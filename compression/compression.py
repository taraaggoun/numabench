import matplotlib.pyplot as plt
import numpy as np

def get_dict(filename):
    data_dict = []
    with open(filename, 'r') as file:
        for line in file:
            tokens = line.strip().split()
            data_dict.append(float(tokens[0]) / 1000)
    return data_dict

def plot_bar_2_with_std(values1, values2, configuration):
    # Calcul de la moyenne et de l'écart type
    moyenne1 = np.mean(values1)
    moyenne2 = np.mean(values2)
    std_dev1 = np.std(values1)
    std_dev2 = np.std(values2)
    
    x = np.arange(2)
    largeur = 0.35
    fig, ax = plt.subplots()
    rects1 = ax.bar(x[0], moyenne1, largeur, yerr=std_dev1, capsize=5, label='Locale')
    rects2 = ax.bar(x[1], moyenne2, largeur, yerr=std_dev2, capsize=5, label='Distant')

    ax.set_ylabel('Temps en seconde')
    ax.set_xticks(x)
    ax.set_xticklabels(['Locale', 'Distant'])
    ax.legend()
    plt.savefig("numabench/media/graph/compression_" + configuration + ".png") # Sauvegarder le graphique dans un fichier
    plt.close()

def main():
    local_0 = get_dict("compression/media/data/local_0")
    remote_0 = get_dict("compression/media/data/remote_0")
    plot_bar_2_with_std(local_0, remote_0, "0")

if __name__ == "__main__":
    main()