import matplotlib.pyplot as plt
plt.rcParams['font.family'] = ['Arial Unicode MS', 'STHeiti', 'STSong', 'SimSun', 'Microsoft YaHei', 'Heiti TC', 'STKaiti', 'LiSu', 'Apple LiGothic', 'Hiragino Sans GB', 'Noto Sans CJK SC', 'Source Han Sans SC']

# 请求密度 实际资源消耗

x = [6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42]
# 无效云边调整次数
# y1 = [0, 9, 4, 17, 31, 41, 25, 37, 60, 43, 75, 127, 139, 85, 59, 224, 239, 242, 229]
# y3 = [2, 26, 39, 51, 48, 135, 71, 83, 51, 90, 127, 130, 53, 245, 92, 179, 161, 219, 188]
# y5 = [4, 22, 41, 53, 44, 92, 80, 114, 57, 80, 85, 94, 75, 100, 53, 84, 64, 71, 141]
y1 = [0, 0.209302, 0.111111, 0.361702, 0.508197, 0.554054, 0.462963, 0.560606, 0.652174, 0.605634, 0.735294, 0.808917, 0.852761, 0.794393, 0.746835, 0.896, 0.915709, 0.909774, 0.905138]
y3 = [0.0526316, 0.42623, 0.534247, 0.621951, 0.592593, 0.798817, 0.68932, 0.761468, 0.614458, 0.769231, 0.819355, 0.822785, 0.697368, 0.900735, 0.814159, 0.877451, 0.884615, 0.904959, 0.890995]
y5 = [0.0851064, 0.360656, 0.525641, 0.595506, 0.556962, 0.730159, 0.714286, 0.786207, 0.640449, 0.727273, 0.745614, 0.803419, 0.742574, 0.806452, 0.688312, 0.777778, 0.771084, 0.763441, 0.859756]


# 绘制折线图
plt.plot(x, y1, color='red',  linewidth=2, marker='o', label='SWD-AD')
# plt.plot(x, y2, color='blue',  linewidth=2, marker='s', label='Swarm')
plt.plot(x, y3, color='green',  linewidth=2, marker='^', label='Swarm-AD')
# plt.plot(x, y4, color='orange',  linewidth=2, marker='D', label='K8s')
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
# plt.ylabel("平均响应时间(ms)")
# plt.ylabel("任务处理受益率")
plt.ylabel("无效云边服务调整所占比例")

# 显示图表
plt.show()
# plt.savefig("exponential_distribution_lambda.svg", format="svg")
# plt.savefig("EDGE_NODE_NUMS.svg", format="svg")
# plt.savefig("normal_distribution_stddev_useless_adjust_percentage.svg", format="svg")
