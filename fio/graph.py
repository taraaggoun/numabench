import re
import matplotlib.pyplot as plt
import numpy as np

def extract_bandwidth(filename):
    bandwidths = []
    with open(filename, 'r') as file:
        content = file.read()
        matches = re.findall(r'read:.*?bw=(\d+(\.\d+)?)MiB/s', content, re.DOTALL)
        for match in matches:
            bandwidths.append(float(match[0]))
    return bandwidths

def create_bandwidth_graph(local_bandwidths, remote_bandwidths, config):
    local_mean = np.mean(local_bandwidths) if local_bandwidths else 0
    local_std = np.std(local_bandwidths) if local_bandwidths else 0
    remote_mean = np.mean(remote_bandwidths) if remote_bandwidths else 0
    remote_std = np.std(remote_bandwidths) if remote_bandwidths else 0

    tests = ['Local', 'Remote']
    means = [local_mean, remote_mean]
    std_devs = [local_std, remote_std]

    plt.figure(figsize=(12, 8))
    
    plt.bar(tests, means, yerr=std_devs, capsize=5, color=['blue', 'orange'], alpha=0.7)
    
    plt.ylabel('Bande Passante (MiB/s)')
    plt.title('Execution de fio en lecture d\'un fichier de 7Go')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    
    plt.savefig('fio/media/graph_' + config + '.png')
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

    plt.ylabel('Bande Passante (MiB/s)')
    plt.title('Exécution de fio en lecture d\'un fichier de 7Go')
    plt.xticks(x, ['Local', 'Remote'])
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.legend(labels)
    plt.savefig('fio/media/graph_all.png')
    plt.show()


def main():
    local_bandwidths_0 = extract_bandwidth('fio/media/local_0')
    remote_bandwidths_0 = extract_bandwidth('fio/media/remote_0')
    create_bandwidth_graph(local_bandwidths_0, remote_bandwidths_0, '0')

    local_bandwidths_1 = extract_bandwidth('fio/media/local_1')
    remote_bandwidths_1 = extract_bandwidth('fio/media/remote_1')
    create_bandwidth_graph(local_bandwidths_1, remote_bandwidths_1, '1')

    local_bandwidths_2 = extract_bandwidth('fio/media/local_2')
    remote_bandwidths_2 = extract_bandwidth('fio/media/remote_2')
    create_bandwidth_graph(local_bandwidths_2, remote_bandwidths_2, '2')

    local_bandwidths_3 = extract_bandwidth('fio/media/local_3')
    remote_bandwidths_3 = extract_bandwidth('fio/media/remote_3')
    create_bandwidth_graph(local_bandwidths_3, remote_bandwidths_3, '3')

    local_bandwidths_4 = extract_bandwidth('fio/media/local_4')
    remote_bandwidths_4 = extract_bandwidth('fio/media/remote_4')
    create_bandwidth_graph(local_bandwidths_4, remote_bandwidths_4, '4')

    local_bandwidths_5 = extract_bandwidth('fio/media/local_5')
    remote_bandwidths_5 = extract_bandwidth('fio/media/remote_5')
    create_bandwidth_graph(local_bandwidths_5, remote_bandwidths_5, '5')

    locals = [local_bandwidths_0, local_bandwidths_1, local_bandwidths_2, local_bandwidths_3, local_bandwidths_4, local_bandwidths_5]
    remotes = [remote_bandwidths_0, remote_bandwidths_1, remote_bandwidths_2, remote_bandwidths_3, remote_bandwidths_4, remote_bandwidths_5]
    create_bandwidth_6_graph(locals, remotes)

if __name__ == "__main__":
    main()