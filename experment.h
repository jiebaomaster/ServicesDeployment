#ifndef __EXPERMENT_H__
#define __EXPERMENT_H__

#include <vector>
#include <queue>
#include <iostream>

enum task_type {
  mobileNet_tf_lite,
  PoseEstimation,
  yolov3_tiny,
  UNetSegmentation,
  MobileNetSSD,
  mongoose_socks5,
  mongoose_http_server,
  mapped_task_nums
};

// 资源类别 CPU GPU MEM DISK
struct source_categories {
  int cpu;
  int gpu;
  int memory;
  int disk;
};

struct service_data {
  double edge_processing_time; // 边缘端任务处理耗时
  double cloud_processing_time; // 云端任务处理耗时
  double edge_transmission_time = 15; // 边缘端任务传输耗时
  double cloud_transmission_time = 69; // 云端任务传输耗时
  double acceleration_ratio; // 加速比
  double edge_reDeploy_time; // 重新部署到边缘端耗时
  double cloud_reDeploy_time; // 重新部署到服务端耗时

  source_categories free_source;
  source_categories busy_source;
};

struct service {
  int task_type;
  int node_index = -1; // 部署的节点编号，-1 表示部署在 cloud 上
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
  double remain_time; // 剩余需要处理时间长度
  double request_time; // 任务到达时间
  double arrive_time; // 任务传输到服务器的时间
  double processing_time; // 任务开始处理的时间
  double complete_time; // 任务处理完成的时间
  double respond_time; // 响应时间
};

enum node_type {
  edge_Raspberry3,
  edge_Raspberry4b,
  edge_JetsonNano,
  edge_XavierNX,
  mapped_node_nums,
  cloud
};

struct node_data {
  source_categories total_source;
};

struct node {
  int node_type;
  source_categories remain_source; // 当前还剩余多少资源
  node(int t) : node_type(t) {}
};

static inline bool checkDeploy(const source_categories& usage, const node& n) {
  return usage.cpu <= n.remain_source.cpu
      && usage.memory <= n.remain_source.memory
      && usage.disk <= n.remain_source.disk
      && usage.gpu <= n.remain_source.gpu;
}

static inline void doDeploy(const source_categories& usage, node& n) {
  n.remain_source.cpu -= usage.cpu;
  n.remain_source.memory -= usage.memory;
  n.remain_source.disk -= usage.disk;
  n.remain_source.gpu -= usage.gpu;
}

static inline void chancelDeploy(const source_categories& usage, node& n) {
  n.remain_source.cpu += usage.cpu;
  n.remain_source.memory += usage.memory;
  n.remain_source.disk += usage.disk;
  n.remain_source.gpu += usage.gpu;
}

std::vector<service_data> service_statistics; // 各个服务的基本信息
std::vector<service> services; // 生成的服务
std::vector<task> tasks; // 时间轴上将要到达的任务序列
std::vector<node_data> node_statistics; // 各个节点的基本信息
std::vector<node> nodes; // 生成的节点

const int TASK_NUMS = 1000; // 生成任务的数量
const int TASK_CATEGORY_NUMS = mapped_task_nums * 10; // 生成任务的种类
const int EDGE_NODE_NUMS = 8; // 边缘端节点个数

#define TO_MAPPED_TASK_TYPE(t) (task_type)(t % mapped_task_nums)
#define TO_MAPPED_NODE_TYPE(t) (node_type)(t % mapped_node_nums)

class Deployer {
public:
  virtual void run() = 0;
  virtual void deployment() = 0;
  // 对模拟结果的分析
  void analysis() {
    double total = 0.0;
    // 平均任务响应时间
    for(auto &t : tasks) {
      if(!t.is_transmission_task) {
        total += t.respond_time - t.request_time;
      }
    }
    std::cout << "平均任务响应时间：" << total / TASK_NUMS << std::endl;

    // 边缘节点的实际资源利用率
  }
};

#endif