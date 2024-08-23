import re
import matplotlib.pyplot as plt
import numpy as np

def get_time(filename):
    data_dict = []
    with open(filename, 'r') as file:
        for line in file:
            tokens = line.strip().split()
            data_dict.append(float(tokens[0]))
    return data_dict

def create_graph(local, remote, config):
    local_mean = np.mean(local)
    local_std = np.std(local)
    remote_mean = np.mean(remote)
    remote_std = np.std(remote)

    tests = ['Local', 'Remote']
    means = [local_mean, remote_mean]
    std_devs = [local_std, remote_std]

    plt.figure(figsize=(12, 8))
    
    plt.bar(tests, means, yerr=std_devs, color=['blue', 'orange'], capsize=5, alpha=0.7)
    
    plt.ylabel('Temps en milliseconde')
    plt.title('Execution de grep sur un fichier de 10Go')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    
    plt.savefig('grep/media/graph_' + config + '.png')
    plt.show()

def create_bandwidth_6_graph(local_bandwidths, remote_bandwidths):
    labels = ['Témoin', 'Linux', 'Open Force', 'Open Try', 'Exec Force', 'Exec Try']
    colors = ['#7F7F7F', '#d3d3d3', '#0a3d62', '#6baed6', '#d62728', '#ff5f57']

    local_means = [np.mean(values) for values in local_bandwidths]
    local_stds = [np.std(values) for values in local_bandwidths]
    remote_means = [np.mean(values) for values in remote_bandwidths]
    remote_stds = [np.std(values) for values in remote_bandwidths]

    x = np.arange(2)
    width = 0.15
    plt.figure(figsize=(14, 8))

    for i in range(len(labels)):
        plt.bar(x[0] - width*2.5 + i*width, local_means[i], width, yerr=local_stds[i], capsize=5, color=colors[i], alpha=0.7)
    for i in range(len(labels)):
        plt.bar(x[1] - width*2.5 + i*width, remote_means[i], width, yerr=remote_stds[i], capsize=5, color=colors[i], alpha=0.7)

    plt.ylabel('Temps en milliseconde')
    plt.title('Exécution de grep sur un fichier de 10Go')
    plt.xticks(x, ['Local', 'Remote'])
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.legend(labels)
    plt.savefig('grep/media/graph_all.png')
    plt.show()


def main():
    local_0 = get_time('grep/media/local_0')
    remote_0 = get_time('grep/media/remote_0')
    create_graph(local_0, remote_0, '0')

    local_1 = get_time('grep/media/local_1')
    remote_1 = get_time('grep/media/remote_1')
    create_graph(local_1, remote_1, '1')

    local_2 = get_time('grep/media/local_2')
    remote_2 = get_time('grep/media/remote_2')
    create_graph(local_2, remote_2, '2')

    local_3 = get_time('grep/media/local_3')
    remote_3 = get_time('grep/media/remote_3')
    create_graph(local_3, remote_3, '3')

    local_4 = get_time('grep/media/local_4')
    remote_4 = get_time('grep/media/remote_4')
    create_graph(local_4, remote_4, '4')

    local_5 = get_time('grep/media/local_5')
    remote_5 = get_time('grep/media/remote_5')
    create_graph(local_5, remote_5, '5')

    locals = [local_0, local_1, local_2, local_3, local_4, local_5]
    remotes = [remote_0, remote_1, remote_2, remote_3, remote_4, remote_5]
    create_bandwidth_6_graph(locals, remotes)

if __name__ == "__main__":
    main()