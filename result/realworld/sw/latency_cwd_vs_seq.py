import matplotlib.pyplot as plt
import numpy as np

#cfs_seq p50 11, p95 32, p99 49, p999 97
#cfs_cwd p50 10, p95 25, p99 37, p999 62
#n19_seq p50 10, p95 27, p99 46, p999 136
#n19_cwd p50 10, p95 24, p99 37, p999 54_
#bt_seq  p50 9, p95 24, p99 35, p999 51
#bt_cwd  p50 10, p95 22, p99 33, p999 48

# cfs-p95 cfs-p99 cfs-p999 nice19-p95 nice19-p99 nice19-p999 batch-p95 batch-p99 batch-p999
seq = [32, 49, 97, 27, 46, 136, 24, 35, 51]
cwd = [25, 37, 62, 24, 37, 54, 22, 33, 48]

if __name__ == '__main__':
    fig = plt.figure(figsize=(10, 4))
    # 绘制第一组柱状图
    labels = ['Cfs-p95', 'Cfs-p99', 'Cfs-p999' ,'Nice19-p95' ,'Nice19-p99' ,'Nice19-p999' ,'Batch-p95' ,'Batch-p99' ,'Batch-p999']
    bar_width = 0.3
    x = np.arange(0, 9)
    plt.bar(x, cwd, bar_width, label = 'CWD', hatch = '-', edgecolor='black')
    plt.bar(x + bar_width, seq, bar_width, label = 'sequence', hatch = 'x', edgecolor='black', color = 'pink')

    plt.ylabel('时延(ms)')
    plt.legend()
    plt.xticks(x + bar_width*0.5, labels, rotation=15)
    fig.savefig('./latency_cwd_vs_seq.svg', format="svg")

    plt.show()