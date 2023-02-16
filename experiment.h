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
  hitch_proxy,
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
  int request_time; // 任务到达时间
  int arrive_time; // 任务传输到服务器的时间
  int processing_time; // 任务开始处理的时间
  int complete_time; // 任务处理完成的时间
  int respond_time; // 响应时间
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

const int TASK_NUMS = 1000; // 生成任务的数量
const int TASK_CATEGORY_NUMS = mapped_task_nums * 10; // 生成任务的种类
const int EDGE_NODE_NUMS = mapped_node_nums * 2; // 边缘端节点个数
const int CLOUD_NODE_INDEX = EDGE_NODE_NUMS; // cloud 服务器编号

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
  std::vector<double> real_resource_utilization; // 平均实际资源利用率
  std::vector<double> expected_resource_utilization; // 平均期望资源利用率
public:
  virtual void run() = 0;

  virtual void deployment() = 0;

  double cal_resource_utilization_helper(source_categories &sc, int i) {
    auto &node_total_source = node_statistics[TO_MAPPED_NODE_TYPE(i)].total_source;
    double cpu = (double) sc.cpu / node_total_source.cpu;
    double memory = (double) sc.memory / node_total_source.memory;
    if (node_total_source.gpu == 0) {
      return (cpu + memory) / 2;
    } else {
      double gpu = (double) sc.gpu / node_total_source.gpu;
      double gpu_memory = (double) sc.gpu_memory / node_total_source.gpu_memory;
      return (cpu + memory + gpu + gpu_memory) / 4;
    }
  }

  // 计算一帧的平均资源利用率
  void cal_resource_utilization() {
    std::vector<double> per_node_real_resource_utilization(CLOUD_NODE_INDEX);
    std::vector<double> per_node_expected_resource_utilization(CLOUD_NODE_INDEX);
    for (int i = 0; i < CLOUD_NODE_INDEX; i++) {
      // expected_resource_utilization = remain/total
      if (nodes[i].deploy_service.empty())
        per_node_expected_resource_utilization[i] = 0;
      else
        per_node_expected_resource_utilization[i] = 1.0 - cal_resource_utilization_helper(nodes[i].remain_source, i);

      source_categories real_sc{0, 0, 0, 0};
      for (auto s: nodes[i].deploy_service) {
        // real_resource_utilization = sum(real_use)/total
        if (services[s].waiting_tasks.empty()) {
          real_sc.cpu += get_per_node_service_date(s, i).free_source.cpu;
          real_sc.memory += get_per_node_service_date(s, i).free_source.memory;
          real_sc.gpu += get_per_node_service_date(s, i).free_source.gpu;
          real_sc.gpu_memory += get_per_node_service_date(s, i).free_source.gpu_memory;
        } else {
          real_sc.cpu += get_per_node_service_date(s, i).busy_source.cpu;
          real_sc.memory += get_per_node_service_date(s, i).busy_source.memory;
          real_sc.gpu += get_per_node_service_date(s, i).busy_source.gpu;
          real_sc.gpu_memory += get_per_node_service_date(s, i).busy_source.gpu_memory;
        }
      }
      per_node_real_resource_utilization[i] = cal_resource_utilization_helper(real_sc, i);
    }
    expected_resource_utilization.push_back(std::accumulate(per_node_expected_resource_utilization.begin(),
                                                            per_node_expected_resource_utilization.end(), 0.0) /
                                            CLOUD_NODE_INDEX);
    real_resource_utilization.push_back(std::accumulate(per_node_real_resource_utilization.begin(),
                                                        per_node_real_resource_utilization.end(), 0.0) /
                                        CLOUD_NODE_INDEX);
  }

// 对模拟结果的分析
  void analysis() {
    double total_time = 0.0;
    // 平均任务响应时间
    for (auto &t: tasks) {
      if (!t.is_transmission_task) {
        total_time += t.respond_time - t.request_time;
      }
    }
    std::cout << "平均任务响应时间：" << total_time / TASK_NUMS << std::endl;

    // 边缘节点的期望资源利用率和实际资源利用率
    std::cout << "（10ms周期）平均期望资源利用率："
              << std::accumulate(expected_resource_utilization.begin(), expected_resource_utilization.end(), 0.0) /
                 expected_resource_utilization.size()
              << std::endl
              << "（10ms周期）平均实际资源利用率："
              << std::accumulate(real_resource_utilization.begin(), real_resource_utilization.end(), 0.0) /
                 real_resource_utilization.size()
              << std::endl;

  }
};

#endif