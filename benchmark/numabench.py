import matplotlib.pyplot as plt
import numpy as np

def get_dict(filename):
    data_dict = []
    with open(filename, 'r') as file:
        for line in file:
            tokens = line.strip().split()
            data_dict.append(float(tokens[0]))
    return data_dict

def plot_bar_2_with_std(values1, values2, configuration, type):
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

    ax.set_ylabel('Temps en milliseconde')
    if type == 'read':
        ax.set_title('Temps moyen de lecture randomn avec écart type')
    else :
        ax.set_title('Temps moyen d\'écriture randomn avec écart type')
    ax.set_xticks(x)
    ax.set_xticklabels(['Locale', 'Distant'])
    ax.legend()
    plt.savefig("media/graph/" + type + "_" + configuration + ".png") # Sauvegarder le graphique dans un fichier
    plt.close()

def plot_bar_4_with_std(values1, values2, values3, values4, config, type):
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
    if type == 'read':
        ax.set_title('Temps moyen de lecture randomn avec écart type')
    else :
        ax.set_title('Temps moyen d\'écriture randomn avec écart type')
    ax.set_xticks(x)
    ax.set_xticklabels(['LL', 'DL', 'LD', 'DD'])
    plt.savefig("media/graph/" + type + "_" + config + ".png") # Sauvegarder le graphique dans un fichier
    plt.close()

def plot_bar_20_with_std(values_list, type):
    if len(values_list) != 20:
        raise ValueError("values_list must contain exactly 20 arrays of values")

    means = [np.mean(values) for values in values_list]
    std_devs = [np.std(values) for values in values_list]

    x = np.arange(4)  # 4 groups (LL, DL, LD, DD)
    width = 0.15  # width of the bars
    offsets = [-2 * width, -width, 0, width, 2 * width]  # offsets for centering bars

    fig, ax = plt.subplots()

    colors = ['#7F7F7F', '#0a3d62', '#6baed6', '#d62728', '#ff5f57']
    labels = ['Témoin', 'Open Force', 'Open Try', 'Exec Force', 'Exec Try']
    for i, label in enumerate(labels):
        start_index = i * 4
        end_index = start_index + 4
        ax.bar(x + offsets[i], means[start_index:end_index], width, yerr=std_devs[start_index:end_index], capsize=5, color=colors[i], label=label)

    ax.set_ylabel('Temps en milliseconde')
    if type == 'read':
        ax.set_title('Temps moyen de lecture randomn avec écart type')
    else :
        ax.set_title('Temps moyen d\'écriture randomn avec écart type')
    ax.set_xticks(x)
    ax.set_xticklabels(['LL', 'DL', 'LD', 'DD'])
    ax.legend()

    plt.savefig("media/graph/" + type + "_all.png")
    plt.close()

def main():
    # Temoin
    read_ll_0 = get_dict("media/data/numabench/read_LL_0")
    read_dl_0 = get_dict("media/data/numabench/read_DL_0")
    read_ld_0 = get_dict("media/data/numabench/read_LD_0")
    read_dd_0 = get_dict("media/data/numabench/read_DD_0")
    plot_bar_4_with_std(read_ll_0, read_dl_0, read_ld_0, read_dd_0, "0", "read")

    write_ll_0 = get_dict("media/data/numabench/write_LL_0")
    write_dl_0 = get_dict("media/data/numabench/write_DL_0")
    write_ld_0 = get_dict("media/data/numabench/write_LD_0")
    write_dd_0 = get_dict("media/data/numabench/write_DD_0")
    plot_bar_4_with_std(write_ll_0, write_dl_0, write_ld_0, write_dd_0, "0", "write")

    # Open
    # Force
    read_ll_1 = get_dict("media/data/numabench/read_LL_1")
    read_dl_1 = get_dict("media/data/numabench/read_DL_1")
    read_ld_1 = get_dict("media/data/numabench/read_LD_1")
    read_dd_1 = get_dict("media/data/numabench/read_DD_1")
    plot_bar_4_with_std(read_ll_1, read_dl_1, read_ld_1, read_dd_1, "1", "read")

    write_ll_1 = get_dict("media/data/numabench/write_LL_1")
    write_dl_1 = get_dict("media/data/numabench/write_DL_1")
    write_ld_1 = get_dict("media/data/numabench/write_LD_1")
    write_dd_1 = get_dict("media/data/numabench/write_DD_1")
    plot_bar_4_with_std(write_ll_1, write_dl_1, write_ld_1, write_dd_1, "1", "write")
    
    # Try
    read_ll_2 = get_dict("media/data/numabench/read_LL_2")
    read_dl_2 = get_dict("media/data/numabench/read_DL_2")
    read_ld_2 = get_dict("media/data/numabench/read_LD_2")
    read_dd_2 = get_dict("media/data/numabench/read_DD_2")
    plot_bar_4_with_std(read_ll_2, read_dl_2, read_ld_2, read_dd_2, "2", "read")
    
    write_ll_2 = get_dict("media/data/numabench/write_LL_2")
    write_dl_2 = get_dict("media/data/numabench/write_DL_2")
    write_ld_2 = get_dict("media/data/numabench/write_LD_2")
    write_dd_2 = get_dict("media/data/numabench/write_DD_2")
    plot_bar_4_with_std(write_ll_2, write_dl_2, write_ld_2, write_dd_2, "2", "write")

    # Exec
    # Try
    read_ll_3 = get_dict("media/data/numabench/read_LL_3")
    read_dl_3 = get_dict("media/data/numabench/read_DL_3")
    plot_bar_2_with_std(read_ll_3, read_dl_3, "3", "read")

    write_ll_3 = get_dict("media/data/numabench/write_LL_3")
    write_dl_3 = get_dict("media/data/numabench/write_DL_3")
    plot_bar_2_with_std(write_ll_3, write_dl_3, "3", "write")
    
    # Force
    read_ll_4 = get_dict("media/data/numabench/read_LL_4")
    read_dl_4 = get_dict("media/data/numabench/read_DL_4")
    plot_bar_2_with_std(read_ll_4, read_dl_4, "4", "read")

    write_ll_4 = get_dict("media/data/numabench/write_LL_4")
    write_dl_4 = get_dict("media/data/numabench/write_DL_4")
    plot_bar_2_with_std(write_ll_4, write_dl_4, "4", "write")

    # V2
    # Force
    read_ll_5 = get_dict("media/data/numabench/read_LL_5")
    read_dl_5 = get_dict("media/data/numabench/read_DL_5")
    plot_bar_2_with_std(read_ll_5, read_dl_5, "5", "read")
    # plot_bar_4_with_std(read_ll_4, read_dl_4, read_ll_5, read_dl_5, "7", "read")

    
    # Try
    read_ll_6 = get_dict("media/data/numabench/read_LL_6")
    read_dl_6 = get_dict("media/data/numabench/read_DL_6")
    plot_bar_2_with_std(read_ll_6, read_dl_6, "6", "read")
    # plot_bar_4_with_std(read_ll_3, read_dl_3, read_ll_6, read_dl_6, "8", "read")

    values  = [read_ll_0, read_dl_0, read_ld_0, read_dd_0]
    values += [read_ll_1, read_dl_1, read_ld_1, read_dd_1]
    values += [read_ll_2, read_dl_2, read_ld_2, read_dd_2]
    values += [read_ll_5, read_dl_5, [0], [0]]
    values += [read_ll_6, read_dl_6, [0], [0]]
    plot_bar_20_with_std(values, "read")

    values  = [write_ll_0, write_dl_0, write_ld_0, write_dd_0]
    values += [write_ll_1, write_dl_1, write_ld_1, write_dd_1]
    values += [write_ll_2, write_dl_2, write_ld_2, write_dd_2]
    values += [write_ll_4, write_dl_4, [0], [0]]
    values += [write_ll_3, write_dl_3, [0], [0]]
    plot_bar_20_with_std(values, "write")

if __name__ == "__main__":
    main()
