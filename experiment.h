#ifndef __EXPERIMENT_H__
#define __EXPERIMENT_H__

#include <vector>
#include <queue>
#include <list>
#include <numeric>
#include <iostream>

enum task_type {
  mobileNet_tf_lite,
  PoseEstimation,
  yolov3_tiny,
  UNetSegmentation, // 4b, nano, nx
  MobileNetSSD, // nano, nx
  redis_server,
  nodejs_http_server,
  mapped_task_nums
};

// 资源类别 CPU GPU MEM DISK
struct source_categories {
  int cpu;
  int gpu;
  int gpu_memory;
  int memory;
};

struct per_node_service_date {
  int processing_time; // 边缘端任务处理耗时
  int transmission_time = 15; // 边缘端任务传输耗时
  int redeploy_time; // 重新部署到边缘端耗时

  source_categories free_source;
  source_categories busy_source;
};

struct service_data {
  int initial_weight; // 初始权重
  // 该服务部署到每一个节点上的资源与性能参数
  std::vector<per_node_service_date> per_node_service_date;
};

struct service {
  int task_type; // 服务类型
  int node_index; // 部署的节点编号，-1 表示部署在 cloud 上
  double weight; // 权重
  std::queue<int> waiting_tasks; // 等待执行的任务

  explicit service(int t) : task_type(t) {}
};

/**
 * 任务的处理过程
 * |****传输到服务器****|***可能需要排队等待***|********处理********|***返回响应给用户***|
 * ^                  ^                   ^                   ^                 ^
 * |request_time      |arrive_time        |processing_time    |complete_time    | respond_time
 */
struct task {
  int task_type; // 任务类型，即对应的服务类型
  bool is_transmission_task = false; // 是否是临时插入的代表服务云边调整的任务
  long long request_time; // 任务到达时间
  long long arrive_time; // 任务传输到服务器的时间
  long long processing_time; // 任务开始处理的时间
  long long complete_time; // 任务处理完成的时间
  long long respond_time; // 响应时间
};

enum node_type {
  edge_Raspberry3,
  edge_Raspberry4b,
  edge_JetsonNano,
  edge_XavierNX,
  mapped_node_nums // cloud
};

struct node_data {
  source_categories total_source; // 服务器节点的资源总量
};

struct node {
  int node_type;
  source_categories remain_source; // 当前还剩余多少资源
  std::list<int> deploy_service; // 已部署的服务
  node(int t) : node_type(t) {}
};

std::vector<service_data> service_statistics; // 各个服务的基本信息
std::vector<service> services; // 生成的服务
std::vector<task> tasks; // 时间轴上将要到达的任务序列
std::vector<node_data> node_statistics; // 各个节点的基本信息
std::vector<node> nodes; // 生成的节点

const int TASK_NUMS = 10000; // 生成任务的数量
const int TASK_CATEGORY_NUMS = mapped_task_nums * 10; // 生成任务（服务）的种类
const int EDGE_NODE_NUMS = mapped_node_nums * 4; // 边缘端节点个数
const int CLOUD_NODE_INDEX = EDGE_NODE_NUMS; // cloud 服务器编号
int CLOCK_TICK = 100; // 帧长度
// 生成任务序列的参数
double exponential_distribution_lambda = 0.5; // 任务时间间隔，指数分布
double normal_distribution_mean = TASK_CATEGORY_NUMS / 2.0; // 任务类型，正态分布均值
double normal_distribution_stddev = TASK_CATEGORY_NUMS / 7.0; // 任务类型，正态分布方差


#define TO_MAPPED_TASK_TYPE(t) (task_type)((t) % mapped_task_nums)
#define TO_MAPPED_NODE_TYPE(t) (node_type)((t) % mapped_node_nums)

static inline per_node_service_date &get_per_node_service_date(int service_index, int node_index) {
  if (node_index == CLOUD_NODE_INDEX)
    return service_statistics[TO_MAPPED_TASK_TYPE(service_index)].per_node_service_date[mapped_node_nums];
  else
    return service_statistics[TO_MAPPED_TASK_TYPE(service_index)].per_node_service_date[TO_MAPPED_NODE_TYPE(
            node_index)];
}

static inline source_categories &get_source_categories(int service_index, int node_index) {
  return get_per_node_service_date(service_index, node_index).busy_source;
}

static inline bool checkDeploy(const source_categories &usage, const node &n) {
  return usage.cpu <= n.remain_source.cpu
         && usage.memory <= n.remain_source.memory
         && usage.gpu_memory <= n.remain_source.gpu_memory
         && usage.gpu <= n.remain_source.gpu;
}

static inline void doDeploy(const source_categories &usage, node &n) {
  n.remain_source.cpu -= usage.cpu;
  n.remain_source.memory -= usage.memory;
  n.remain_source.gpu_memory -= usage.gpu_memory;
  n.remain_source.gpu -= usage.gpu;
}

static inline void chancelDeploy(const source_categories &usage, node &n) {
  n.remain_source.cpu += usage.cpu;
  n.remain_source.memory += usage.memory;
  n.remain_source.gpu_memory += usage.gpu_memory;
  n.remain_source.gpu += usage.gpu;
}

class Deployer {
public:
  virtual void deployment() = 0;
  virtual void clock_tick_handler() = 0;
  virtual void new_task_handler(service& s, int t) = 0;
  virtual void task_process_handler(service& s, long long running_time) = 0;
};

#endif