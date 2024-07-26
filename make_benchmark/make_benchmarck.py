import matplotlib.pyplot as plt
import numpy as np

def convert_to_seconds(time_str):
    # Extraire les minutes et les secondes
    parts = time_str.strip().replace('s', '').split('m')
    minutes = int(parts[0])
    seconds = float(parts[1].replace(',', '.'))
    
    # Convertir le tout en secondes
    total_seconds = minutes * 60 + seconds
    return total_seconds

def extract_first_real_time(filename):
    with open(filename, 'r') as file:
        lines = file.readlines()

    real_times = []
    
    for line in lines:
        if line.startswith("real"):
            real_time = line.split()[1]
            real_times.append(convert_to_seconds(real_time))

    return real_times

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
    ax.set_title('Temps moyen de compilation avec écart type')
    ax.set_xticks(x)
    ax.set_xticklabels(['Locale', 'Distant'])
    ax.legend()
    plt.savefig("media/graph/make_" + configuration + ".png") # Sauvegarder le graphique dans un fichier
    plt.close()

def plot_bar_8_with_std(values_list):
    if len(values_list) != 8:
        raise ValueError("values_list must contain exactly 8 arrays of values")

    # Calcule les moyennes et les écarts-types
    means = [np.mean(values) for values in values_list]
    std_devs = [np.std(values) for values in values_list]

    x = np.arange(2)  # 2 groupes : 'Locale' et 'Distant'
    width = 0.2  # Largeur des barres

    fig, ax = plt.subplots()

    ax.bar(x - 1.5 * width, means[:2], width, yerr=std_devs[:2], capsize=5, label='Témoin')
    ax.bar(x - 0.5 * width, means[2:4], width, yerr=std_devs[2:4], capsize=5, label='Force')
    ax.bar(x + 0.5 * width, means[4:6], width, yerr=std_devs[4:6], capsize=5, label='Try')
    ax.bar(x + 1.5 * width, means[6:], width, yerr=std_devs[6:], capsize=5, label='Exec')

    ax.set_ylabel('Temps en secondes')
    ax.set_title('Temps moyen de compilation avec écart type')
    ax.set_xticks(x)
    ax.set_xticklabels(['Locale', 'Distant'])
    ax.legend()

    plt.savefig("media/graph/make_all.png")
    plt.close()


def main():
    # Témoin
    local_0   = extract_first_real_time("media/data/make/local_1_0")
    local_0  += extract_first_real_time("media/data/make/local_2_0")
    remote_0  = extract_first_real_time("media/data/make/remote_1_0")
    remote_0 += extract_first_real_time("media/data/make/remote_2_0")
    plot_bar_2_with_std(local_0, remote_0, "0")

    # Force
    local_1   = extract_first_real_time("media/data/make/local_1_1")
    local_1  += extract_first_real_time("media/data/make/local_2_1")
    remote_1  = extract_first_real_time("media/data/make/remote_1_1")
    remote_1 += extract_first_real_time("media/data/make/remote_2_1")
    plot_bar_2_with_std(local_1, remote_1, "1")

    # Try
    local_2   = extract_first_real_time("media/data/make/local_1_2")
    local_2  += extract_first_real_time("media/data/make/local_2_2")
    remote_2  = extract_first_real_time("media/data/make/remote_1_2")
    remote_2 += extract_first_real_time("media/data/make/remote_2_2")
    plot_bar_2_with_std(local_2, remote_2, "2")

    # Exec
    local_3   = extract_first_real_time("media/data/make/local_1_3")
    local_3  += extract_first_real_time("media/data/make/local_2_3")
    remote_3  = extract_first_real_time("media/data/make/remote_1_3")
    remote_3 += extract_first_real_time("media/data/make/remote_2_3")
    plot_bar_2_with_std(local_3, remote_3, "3")

    values = [local_0, remote_0, local_1, remote_1, local_2, remote_2, local_3, remote_3]
    plot_bar_8_with_std(values)

if __name__ == "__main__":
    main()