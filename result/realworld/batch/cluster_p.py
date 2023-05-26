import matplotlib.pyplot as plt
import numpy as np

#cfs_seq p50 11, p95 32, p99 49, p999 97
#cfs_cwd p50 10, p95 25, p99 37, p999 62
#n19_seq p50 10, p95 27, p99 46, p999 136
#n19_cwd p50 10, p95 24, p99 37, p999 54_
#bt_seq  p50 9, p95 24, p99 35, p999 51
#bt_cwd  p50 10, p95 22, p99 33, p999 48

# seq
cfs =    [11, 32, 49, 97]
nice19 = [10, 27, 46, 136]
bt =     [9, 24, 35, 51]
load0 = [9, 22, 33, 47]

# cwd
# cfs = [10, 25, 37, 62]
# nice19 = [10, 24, 37, 54]
# bt = [10, 22, 33, 48]
# load0 = [9, 22, 33, 47]

if __name__ == '__main__':
    fig, ax1 = plt.subplots()

    # 绘制第一组柱状图
    labels = ['p50', 'p95', 'p99', 'p999']
    bar_width = 0.18
    x = np.arange(0, 4)
    plt.bar(x, load0, bar_width, label = 'Load0', hatch = 'x', color = 'white', edgecolor='black')
    plt.bar(x + bar_width, bt, bar_width, label = 'Batch', hatch = '-', edgecolor='black')
    plt.bar(x + bar_width*2, nice19, bar_width, label = 'Nice19', hatch = '/', color = 'pink', edgecolor='black')
    plt.bar(x + bar_width * 3, cfs, bar_width, label = 'Cfs', hatch = '+', color = 'grey', edgecolor='black')

    plt.ylabel('时延(ms)')
    plt.legend()
    plt.xticks(x + bar_width*1.5, labels)
    plt.savefig('./cluster_p_seq.svg', format="svg")

    plt.show()