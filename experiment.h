#ifndef __EXPERIMENT_H__
#define __EXPERIMENT_H__

#include <vector>
#include <queue>
#include <list>
#include <numeric>
#include <iostream>

enum task_type {
//  dlib_face_detect,
  yolov5s_onnx,
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
  double initial_weight; // 初始权重
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
  int process_node; // 被处理的节点
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
const int TASK_CATEGORY_NUMS = mapped_task_nums * 20; // 生成任务（服务）的种类
int EDGE_NODE_NUMS = mapped_node_nums * 5; // 边缘端节点个数
int CLOUD_NODE_INDEX = EDGE_NODE_NUMS; // cloud 服务器编号
int CLOCK_TICK = 100; // 帧长度
// 生成任务序列的参数
double exponential_distribution_lambda = 3; // 任务时间间隔，指数分布
double normal_distribution_mean = TASK_CATEGORY_NUMS / 2.0; // 任务类型，正态分布均值
double normal_distribution_stddev = TASK_CATEGORY_NUMS / 7.0; // 任务类型，正态分布方差

static inline int TO_MAPPED_TASK_TYPE(int t) {
  return (task_type)((t) % mapped_task_nums);
}
static inline int TO_MAPPED_NODE_TYPE(int t) {
  return (node_type)((t) % mapped_node_nums);
}

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
         && usage.gpu <= n.remain_source.gpu;
}

static inline void doDeploy(const source_categories &usage, node &n) {
  n.remain_source.cpu -= usage.cpu;
  n.remain_source.memory -= usage.memory;
  n.remain_source.gpu -= usage.gpu;
}

static inline void chancelDeploy(const source_categories &usage, node &n) {
  n.remain_source.cpu += usage.cpu;
  n.remain_source.memory += usage.memory;
  n.remain_source.gpu += usage.gpu;
}

double cal_resource_utilization_helper(source_categories &sc, int i) {
  auto &node_total_source = node_statistics[TO_MAPPED_NODE_TYPE(i)].total_source;
  double cpu = (double) sc.cpu / node_total_source.cpu;
  double memory = (double) sc.memory / node_total_source.memory;
  if (node_total_source.gpu == 0) {
    return (cpu + memory) / 2;
  } else {
    double gpu = (double) sc.gpu / node_total_source.gpu;
    return (cpu + memory + gpu) / 3;
  }
}

double cal_speedup_helper(int service_index, int node_index) {
  auto &cur_sd = get_per_node_service_date(service_index, node_index);
  auto &cloud_sd = get_per_node_service_date(service_index, CLOUD_NODE_INDEX);
  return (double) (cloud_sd.processing_time + 2 * cloud_sd.transmission_time) /
         (cur_sd.processing_time + 2 * cur_sd.transmission_time);
}

double cal_speedup_avg_helper(int service_index) {
  double speedup_sum = 0.0;
  for (int i = 0; i < mapped_node_nums; i++) {
    speedup_sum += cal_speedup_helper(service_index, i);
  }
  return speedup_sum / mapped_node_nums;
}

class Deployer {
public:
  virtual void deployment() = 0;
  virtual void clock_tick_handler(long long clock) = 0;
  virtual void new_task_handler(service& s, int t) = 0;
  virtual void task_process_handler(service& s, long long running_time) = 0;
};

#endif