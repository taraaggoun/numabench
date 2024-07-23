import matplotlib.pyplot as plt
import numpy as np

def get_dict(filename):
    data_dict = []
    with open(filename, 'r') as file:
        for line in file:
            tokens = line.strip().split()
            data_dict.append(float(tokens[0]))
    return data_dict

def plot_bar_4_with_std(values1, values2, values3, values4, config):
    # Calcul de la moyenne et de l'écart type
    moyenne1 = np.mean(values1)
    moyenne2 = np.mean(values2)
    moyenne3 = np.mean(values3)
    moyenne4 = np.mean(values4)

    std_dev1 = np.std(values1)
    std_dev2 = np.std(values2)
    std_dev3 = np.std(values3)
    std_dev4 = np.std(values4)
    
    x = np.arange(4)
    largeur = 0.2
    fig, ax = plt.subplots()
    rects1 = ax.bar(x[0], moyenne1, largeur, yerr=std_dev1, capsize=5, label='LL')
    rects2 = ax.bar(x[1], moyenne2, largeur, yerr=std_dev2, capsize=5, label='DL')
    rects3 = ax.bar(x[2], moyenne3, largeur, yerr=std_dev3, capsize=5, label='LD')
    rects4 = ax.bar(x[3], moyenne4, largeur, yerr=std_dev4, capsize=5, label='DD')

    # Étiquettes et légendes
    ax.set_ylabel('Temps en milliseconde')
    ax.set_title('Temps moyen de lecture randomn avec écart type')
    ax.set_xticks(x)
    ax.set_xticklabels(['LL', 'DL', 'LD', 'DD'])
    plt.savefig("media/graph/read_" + config + ".png") # Sauvegarder le graphique dans un fichier
    plt.close()

def plot_bar_16_with_std(values_list):
    if len(values_list) != 16:
        raise ValueError("values_list must contain exactly 16 arrays of values")

    # Compute means and standard deviations
    means = [np.mean(values) for values in values_list]
    std_devs = [np.std(values) for values in values_list]

    x = np.arange(4)  # 4 groups
    width = 0.2  # width of the bars

    fig, ax = plt.subplots()

    # Create bars for each set of values
    rects1 = ax.bar(x - (1.5 * width), means[:4], width, yerr=std_devs[:4], capsize=5, label='Témoin')
    rects2 = ax.bar(x - (0.5 * width), means[4:8], width, yerr=std_devs[4:8], capsize=5, label='Force')
    rects3 = ax.bar(x + (0.5 * width), means[8:12], width, yerr=std_devs[8:12], capsize=5, label='Try')
    rects3 = ax.bar(x + (1.5 * width), means[12:], width, yerr=std_devs[12:], capsize=5, label='Exec')

    # Labels and titles
    ax.set_ylabel('Temps en milliseconde')
    ax.set_title("Temps moyen de lecture randomn avec écart type")
    ax.set_xticks(x)
    ax.set_xticklabels(['LL', 'DL', 'LD', 'DD'])
    ax.legend()

    # Save the plot
    plt.savefig("media/graph/read_all.png")
    plt.close()

def main():
    # Temoin
    tall_ll_0 = get_dict("media/data/LL_0")
    tall_dl_0 = get_dict("media/data/DL_0")
    tall_ld_0 = get_dict("media/data/LD_0")
    tall_dd_0 = get_dict("media/data/DD_0")
    plot_bar_4_with_std(tall_ll_0, tall_dl_0, tall_ld_0, tall_dd_0, "0")

    # Force
    tall_ll_1 = get_dict("media/data/LL_1")
    tall_dl_1 = get_dict("media/data/DL_1")
    tall_ld_1 = get_dict("media/data/LD_1")
    tall_dd_1 = get_dict("media/data/DD_1")
    plot_bar_4_with_std(tall_ll_1, tall_dl_1, tall_ld_1, tall_dd_1, "1")
    
    # Try
    tall_ll_2 = get_dict("media/data/LL_2")
    tall_dl_2 = get_dict("media/data/DL_2")
    tall_ld_2 = get_dict("media/data/LD_2")
    tall_dd_2 = get_dict("media/data/DD_2")
    plot_bar_4_with_std(tall_ll_2, tall_dl_2, tall_ld_2, tall_dd_2, "2")

    # Exec
    tall_ll_3 = get_dict("media/data/LL_3")
    tall_dl_3 = get_dict("media/data/DL_3")
    tall_ld_3 = get_dict("media/data/LD_3")
    tall_dd_3 = get_dict("media/data/DD_3")
    plot_bar_4_with_std(tall_ll_3, tall_dl_3, tall_ld_3, tall_dd_3, "3")
    
    values  = [tall_ll_0, tall_dl_0, tall_ld_0, tall_dd_0]
    values += [tall_ll_1, tall_dl_1, tall_ld_1, tall_dd_1]
    values += [tall_ll_2, tall_dl_2, tall_ld_2, tall_dd_2]
    # values += [[0], [0], [0], [0]]
    values += [tall_ll_3, tall_dl_3, tall_ld_3, tall_dd_3]
    plot_bar_16_with_std(values)

if __name__ == "__main__":
    main()
