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

def differences(tableau1, tableau2):
    taille_min = len(tableau1)
    differences = [abs(a - b) for a, b in zip(tableau1, tableau2[:taille_min])]
    return differences

def plot_bar_5_with_std(values1, values2, values3, values4, values5):
    # Calcul de la moyenne et de l'écart type
    moyenne1 = np.mean(values1)
    moyenne2 = np.mean(values2)
    moyenne3 = np.mean(values3)
    moyenne4 = np.mean(values4)
    moyenne5 = np.mean(values5)

    std_dev1 = np.std(values1)
    std_dev2 = np.std(values2)
    std_dev3 = np.std(values3)
    std_dev4 = np.std(values4)
    std_dev5 = np.std(values5)
    
    x = np.arange(5)
    largeur = 0.2
    fig, ax = plt.subplots()
    rects1 = ax.bar(x[0], moyenne1, largeur, yerr=std_dev1, capsize=5, label='Témoin')
    rects2 = ax.bar(x[1], moyenne2, largeur, yerr=std_dev2, capsize=5, label='Open Force')
    rects3 = ax.bar(x[2], moyenne3, largeur, yerr=std_dev3, capsize=5, label='Open Try')
    rects4 = ax.bar(x[3], moyenne4, largeur, yerr=std_dev4, capsize=5, label='Exec Force')
    rects5 = ax.bar(x[4], moyenne5, largeur, yerr=std_dev5, capsize=5, label='Exec Try')

    # Étiquettes et légendes
    ax.set_ylabel('Temps en seconde')
    ax.set_title('Difference entre le temps de compilation avec écart type')
    ax.set_xticks(x)
    ax.set_xticklabels(['Témoin', 'Force', 'Open Try', 'Exec Force', 'Exec Try'])
    plt.savefig("media/graph/make.png") # Sauvegarder le graphique dans un fichier
    plt.close()

def main():
    # Témoin
    load_0 = extract_first_real_time("media/data/make/load_0")
    make_0 = extract_first_real_time("media/data/make/make_0")
    val_0 = differences(load_0, make_0)

    # Open
    # Force
    load_1 = extract_first_real_time("media/data/make/load_1")
    make_1 = extract_first_real_time("media/data/make/make_1")
    val_1 = differences(load_1, make_1)
    
    # Try
    load_2 = extract_first_real_time("media/data/make/load_2")
    make_2 = extract_first_real_time("media/data/make/make_2")
    val_2 = differences(load_2, make_2)

    # Exec
    # Force
    load_3 = extract_first_real_time("media/data/make/load_3")
    make_3 = extract_first_real_time("media/data/make/make_3")
    val_3 = differences(load_3, make_3)
    
    # Try
    load_4 = extract_first_real_time("media/data/make/load_4")
    make_4 = extract_first_real_time("media/data/make/make_4")
    val_4 = differences(load_4, make_4)

    plot_bar_5_with_std(val_0, val_1, val_2, val_3, val_4)

if __name__ == "__main__":
    main()