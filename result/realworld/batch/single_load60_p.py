import matplotlib.pyplot as plt
import numpy as np

cfs = [11.103, 16.103, 26.783, 47.103, 56.127]
nice19 = [9.863, 12.519, 16.543, 24.111, 51.103]
bt = [9.799, 11.439, 11.895, 15.103, 28.111]
load0 = [9.683, 10.143, 11.415, 14.103, 25.103]

if __name__ == '__main__':
    fig, ax1 = plt.subplots()

    # 绘制第一组柱状图
    labels = ['p50', 'p95', 'p99', 'p999', 'p9999']
    bar_width = 0.18
    x = np.arange(0, 5)
    plt.bar(x, load0, bar_width, label = 'Load0', hatch = 'x', color = 'white', edgecolor='black')
    plt.bar(x + bar_width, bt, bar_width, label = 'Batch', hatch = '-', edgecolor='black')
    plt.bar(x + bar_width*2, nice19, bar_width, label = 'Nice19', hatch = '/', color = 'pink', edgecolor='black')
    plt.bar(x + bar_width * 3, cfs, bar_width, label = 'Cfs', hatch = '+', color = 'grey', edgecolor='black')

    plt.ylabel('时延(ms)')
    plt.legend()
    plt.xticks(x + bar_width*1.5, labels)
    plt.savefig('./load60_p.svg', format="svg")

    plt.show()