import matplotlib.pyplot as plt
plt.rcParams['font.family'] = ['Arial Unicode MS', 'STHeiti', 'STSong', 'SimSun', 'Microsoft YaHei', 'Heiti TC', 'STKaiti', 'LiSu', 'Apple LiGothic', 'Hiragino Sans GB', 'Noto Sans CJK SC', 'Source Han Sans SC']

# 请求密度 实际资源消耗

x = [4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42]
y1 =[0.8155, 0.8091, 0.7253, 0.7631, 0.7014, 0.6705, 0.6498, 0.6294, 0.6469, 0.6427, 0.6233, 0.5962, 0.6212, 0.5765, 0.5863, 0.5844, 0.5835, 0.5598, 0.5627, 0.5641]
y2 =[0.5037, 0.5014, 0.4977, 0.5, 0.4986, 0.4982, 0.4972, 0.4957, 0.4871, 0.5047, 0.4967, 0.4941, 0.5007, 0.502, 0.4989, 0.4971, 0.4961, 0.5, 0.5001, 0.5039]
y3 =[0.8091, 0.747, 0.7064, 0.6168, 0.6231, 0.5677, 0.5623, 0.5595, 0.5124, 0.5514, 0.5401, 0.5238, 0.5237, 0.5292, 0.5047, 0.5006, 0.5039, 0.5042, 0.4923, 0.4866]
y4 =[0.4828, 0.496, 0.4963, 0.4994, 0.4983, 0.4979, 0.4975, 0.4986, 0.4904, 0.5108, 0.5056, 0.5064, 0.5149, 0.521, 0.5181, 0.5204, 0.5235, 0.5291, 0.528, 0.5368]
y5 =[0.7882, 0.7386, 0.7143, 0.6361, 0.6264, 0.5681, 0.5615, 0.5591, 0.5523, 0.5527, 0.5437, 0.5463, 0.5366, 0.5407, 0.5263, 0.512, 0.5036, 0.5354, 0.5306, 0.4993]

# 绘制折线图
plt.plot(x, y1, color='red',  linewidth=2, marker='o', label='SWD-AD')
plt.plot(x, y2, color='blue',  linewidth=2, marker='s', label='Swarm')
plt.plot(x, y3, color='green',  linewidth=2, marker='^', label='Swarm-AD')
plt.plot(x, y4, color='orange',  linewidth=2, marker='D', label='K8s')
plt.plot(x, y5, color='purple',  linewidth=2, marker='*', label='K8s-AD')

# 添加网格线
plt.grid(True, linestyle='--', linewidth=0.5)

# 设置图例
plt.legend()

# 设置 x 轴标签
# plt.xlabel("任务到达密集程度")
# plt.xlabel("边缘节点个数(台)")
plt.xlabel("任务类型均衡程度")


# 设置 y 轴标签
# plt.ylabel("平均响应时间(s)")
plt.ylabel("任务处理受益率")

# 显示图表
# plt.show()
# plt.savefig("exponential_distribution_lambda_benefitRate.svg", format="svg")
# plt.savefig("EDGE_NODE_NUMS_benefitRate.svg", format="svg")
plt.savefig("normal_distribution_stddev_benefitRate.svg", format="svg")
