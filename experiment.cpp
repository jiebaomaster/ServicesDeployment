#include "experiment.h"
#include "PdDeployer.h"
#include "Simulator.h"
#include <random>
#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

void initNodes() {
  // 初始化各节点的指标，实际只有5种类型
  node_statistics = {
          {.total_source = { // pi3
                  400, 0, 100
          }},
          {.total_source = { // pi4b
                  400, 0, 100
          }},
          {.total_source = { // jetson nano
                  400, 100, 100
          }},
          {.total_source = { // xavier nx
                  600, 100, 100
          }},
          {.total_source = { // cloud，资源无限
                  INT32_MAX, INT32_MAX, INT32_MAX
          }}
  };
  // EDGE_NODE_NUMS 个 edge node
  for (int i = 0; i < EDGE_NODE_NUMS; i++) {
    nodes.emplace_back(i);
    nodes.back().remain_source = node_statistics[TO_MAPPED_NODE_TYPE(i)].total_source;
  }
  // 1 cloud
  nodes.emplace_back(CLOUD_NODE_INDEX);
  nodes.back().remain_source = node_statistics[mapped_node_nums].total_source;
}

void initServices() {
  // todo 初始化各服务的性能指标，实际有 7 种类型
  service_statistics = {
//          { // googlenet
//                  .initial_weight = 1,
//                  .per_node_service_date = {
//                          { // pi3
//                                  303, 870, 10,
//                                  {0, 0, 1},
//                                  {26, 0, 1},
//                          },
//                          { // pi4b
//                                  145, 870, 10,
//                                  {1, 0, 1},
//                                  {1,  0, 1},
//                          },
//                          { // jetson nano
//                                  153, 870, 10,
//                                  {1, 1, 1},
//                                  {1,  1, 1},
//                          },
//                          { // xavier nx ✅
//                                  12, 870, 7602,
//                                  {0, 0, 340},
//                                  {96,  76, 1120},
//                          },
//                          { // cloud
//                                  10,  870, 10,
//                                  {0, 0, 1},
//                                  {1, 1, 1},
//                          }
//                  }},
//          { // dlib_face_detect 全 cpu
//                  .initial_weight = 1,
//                  .per_node_service_date = {
//                          { // pi3 ✅
//                                  10180, 431, 12678,
//                                  {0, 0, 17},
//                                  {100, 0, 31},
//                          },
//                          { // pi4b ✅
//                                  7790, 431, 10336,
//                                  {0, 0, 2},
//                                  {99, 0, 4},
//                          },
//                          { // jetson nano ✅
//                                  5230, 431, 6588,
//                                  {0, 0, 5},
//                                  {99, 0, 7},
//                          },
//                          { // xavier nx ✅
//                                  2340, 431, 4488,
//                                  {0, 0, 1},
//                                  {97, 0, 4},
//                          },
//                          { // cloud ✅
//                                  1082,  2420, 2444,
//                                  {0, 0, 1},
//                                  {49, 0, 2},
//                          }
//                  }
//          },
//          { // yolov5s-onnx 全 cpu
//                  .initial_weight = 1,
//                  .per_node_service_date = {
//                          { // pi3 ❎
//                                  INT32_MAX, 431, 12678,
//                                  {INT32_MAX, 0, 0},
//                                  {INT32_MAX, 0,  0},
//                          },
//                          { // pi4b ✅
//                                  3350, 431, 1490,
//                                  {0, 0, 2},
//                                  {200, 0, 3},
//                          },
//                          { // jetson nano ✅
//                                  1878, 431, 1092,
//                                  {0, 0, 4},
//                                  {143, 0, 5},
//                          },
//                          { // xavier nx ✅
//                                  2261, 431, 1751,
//                                  {0, 0, 2},
//                                  {77, 0, 2},
//                          },
//                          { // cloud ✅
//                                  660,  2420, 563,
//                                  {0, 0, 1},
//                                  {15, 0, 2},
//                          }
//                  }
//          },
          { // mobileNet_tf_lite
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3
                                  303, 431,  3500,
                                  {0, 0, 5},
                                  {27, 0, 10},
                          },
                          { // pi4b
                                  145, 431,  3500,
                                  {0, 0, 2},
                                  {30, 0, 8},
                          },
                          { // jetson nano
                                  153, 431,  3500,
                                  {0, 0, 2},
                                  {31, 0, 5},
                          },
                          { // xavier nx
                                  214, 431,  3500,
                                  {0, 0, 1},
                                  {24, 0, 3},
                          },
                          { // cloud
                                  120, 2420, 3500,
                                  {0, 0, 0},
                                  {0,  0, 0},
                          }
                  }},
          {// PoseEstimation
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ❎
                                  INT32_MAX, 0,    0,
                                  {INT32_MAX, 0, 0},
                                  {INT32_MAX, 0,  0}
                          },
                          { // pi4b
                                  971,       300,  7000,
                                  {0,         0, 25},
                                  {25,        0,  37},
                          },
                          { // jetson nano
                                  37,        300,  7000,
                                  {0,         0, 40},
                                  {29,        91, 63},
                          },
                          { // xavier nx
                                  29,        300,  7000,
                                  {0,         0, 30},
                                  {29,        83, 70},
                          },
                          { // cloud
                                  33,        2420, 7000,
                                  {0,         0, 0},
                                  {0,         0,  0},
                          }
                  }},
          {// yolov3_tiny
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ❎
                                  INT32_MAX, 0,    0,
                                  {INT32_MAX, 0, 0},
                                  {INT32_MAX, 0,  0}
                          },
                          { // pi4b
                                  5000,      431,  7000,
                                  {0,         0, 40},
                                  {25,        0,  47},
                          },
                          { // jetson nano
                                  43,        431,  7000,
                                  {0,         0, 50},
                                  {27,        83, 56},
                          },
                          { // xavier nx
                                  29,        431,  615,
                                  {0,         0, 70},
                                  {32,        89, 73},
                          },
                          { // cloud
                                  283,       2420, 1916,
                                  {0,         0, 0},
                                  {80,        0,  4},
                          }
                  }},
          {// UNetSegmentation
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ❎
                                  INT32_MAX, 0,    0,
                                  {INT32_MAX, 0, 0},
                                  {INT32_MAX, 0,  0},
                          },
                          { // pi4b
                                  588,       431,  7000,
                                  {0,         0, 89},
                                  {95,        0,  89},
                          },
                          { // jetson nano
                                  89,        431,  7000,
                                  {0,         0, 55},
                                  {43,        88, 62},
                          },
                          { // xavier nx
                                  18,        431,  7000,
                                  {0,         0, 50},
                                  {49,        97, 69},
                          },
                          { // cloud
                                  10,        2420, 3500,
                                  {0,         0, 0},
                                  {0,         0,  0},
                          }
                  }},
          {// MobileNetSSD
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ❎
                                  INT32_MAX, 0,    0,
                                  {INT32_MAX, 0, 0},
                                  {INT32_MAX, 0,  0},
                          },
                          { // pi4b ❎
                                  INT32_MAX, 0,    0,
                                  {INT32_MAX, 0, 0},
                                  {INT32_MAX, 0,  0},
                          },
                          { // jetson nano
                                  56,        431,  7000,
                                  {0,         0, 60},
                                  {36,        83, 65},
                          },
                          { // xavier nx
                                  133,       431,  7000,
                                  {0,         0, 55},
                                  {50,        99, 76},
                          },
                          { // cloud
                                  45,        2420, 6000,
                                  {0,         0, 0},
                                  {0,         0,  0},
                          }
                  }},
          {// redis_server
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ✅
                                  4, 10, 1914,
                                  {1, 0, 50},
                                  {82, 0, 50},
                          },
                          { // pi4b ✅
                                  3, 10, 2254,
                                  {1, 0, 7},
                                  {79, 0, 7},
                          },
                          { // jetson nano ✅
                                  1, 10, 1380,
                                  {1, 0, 13},
                                  {75, 0, 13},
                          },
                          { // xavier nx ✅
                                  1, 10, 845,
                                  {1, 0, 7},
                                  {55, 0, 7},
                          },
                          { // cloud
                                  1, 50, 845,
                                  {0, 0, 0},
                                  {0,  0, 0},
                          }
                  }},
          {// nodejs_http_server
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ✅
                                  55, 351,  1961,
                                  {0, 0, 11},
                                  {121, 0, 13},
                          },
                          { // pi4b ✅
                                  11, 351,  1718,
                                  {0, 0, 1},
                                  {106, 0, 2},
                          },
                          { // jetson nano ✅
                                  15, 351,  989,
                                  {0, 0, 2},
                                  {116, 0, 5},
                          },
                          { // xavier nx ✅
                                  14, 351,  794,
                                  {0, 0, 2},
                                  {114, 0, 2},
                          },
                          { // cloud
                                  10, 1015, 701,
                                  {0, 0, 0},
                                  {0, 0, 0},
                          }
                  }}
  };

  // TASK_CATEGORY_NUMS 种服务，每种服务都对应一种任务类型 TO_MAPPED_TASK_TYPE(i)
  for (int i = 0; i < TASK_CATEGORY_NUMS; i++) {
    services.emplace_back(i);
  }
}

void initTasks() {
  tasks.resize(TASK_NUMS);
  double cur = 0.0;

  // 任务之间的时间间隔为指数分布
  mt19937 egen(1701);
  exponential_distribution<> ed(exponential_distribution_lambda);

  mt19937 ngen{1996};
  normal_distribution<> nd{normal_distribution_mean, normal_distribution_stddev};

  // 生成 TASK_NUMS 个按指数分布到达的随机任务，任务的种类服从随机分布
  for (int i = 0; i < TASK_NUMS; i++) {
    // 任务种类
    int t = -1;
    while (t < 0 || t >= TASK_CATEGORY_NUMS) {
      t = round(nd(ngen));
    }
    tasks[i].task_type = t;
    // 到达时间
    cur += ed(egen) * 1000;
    tasks[i].request_time = (long long) cur;

  }

  vector<int> task_times(TASK_CATEGORY_NUMS);
  for (int i = 0; i < TASK_NUMS; i++) {
    task_times[tasks[i].task_type]++;
//    cout << "<" << tasks[i].task_type << ", " << tasks[i].request_time << "> ";
  }
//  cout << endl;
  for (auto t: task_times) {
//    cout << t << ", ";
  }
//  cout << endl;
}

// 进行一轮所有算法的模拟
void simulate_one_turn(vector<vector<double>> &avg_wait_time_table,
                       vector<vector<double>> &avg_real_resource_utilization_table) {
  auto s = new Simulator();

  nodes.clear();
  services.clear();
  tasks.clear();
  initNodes();
  initServices();
  initTasks();

  std::vector<service> services_back = services; // 生成的服务
  std::vector<task> tasks_back = tasks; // 时间轴上将要到达的任务序列
  std::vector<node> nodes_back = nodes; // 生成的节点

  cout << "pd ";
  PdDeployer d;
  d.deployment();
  s->run(d, true);
  s->analysis(avg_wait_time_table[0], avg_real_resource_utilization_table[0]);

  // swarm spread unadaptive
  cout << "su ";
  services = services_back;
  tasks = tasks_back;
  nodes = nodes_back;
  d.deployment_swarm_spread();
  s->run(d, false);
  s->analysis(avg_wait_time_table[1], avg_real_resource_utilization_table[1]);

  // swarm spread adaptive
  cout << "sa ";
  services = services_back;
  tasks = tasks_back;
  nodes = nodes_back;
  d.deployment_swarm_spread();
  s->run(d, true);
  s->analysis(avg_wait_time_table[2], avg_real_resource_utilization_table[2]);

  // k8s unadaptive
  cout << "ku ";
  services = services_back;
  tasks = tasks_back;
  nodes = nodes_back;
  d.deployment_k8s_NodeResourcesBalancedAllocation();
  s->run(d, false);
  s->analysis(avg_wait_time_table[3], avg_real_resource_utilization_table[3]);

  // k8s adaptive
  cout << "ka ";
  services = services_back;
  tasks = tasks_back;
  nodes = nodes_back;
  d.deployment_k8s_NodeResourcesBalancedAllocation();
  s->run(d, true);
  s->analysis(avg_wait_time_table[4], avg_real_resource_utilization_table[4]);
}

// 重置全局参数
void param_reset() {
  EDGE_NODE_NUMS = mapped_node_nums * 4; // 边缘端节点个数
  CLOUD_NODE_INDEX = EDGE_NODE_NUMS; // cloud 服务器编号
// 生成任务序列的参数
  exponential_distribution_lambda = 3; // 任务时间间隔，指数分布
  normal_distribution_mean = TASK_CATEGORY_NUMS / 2.0; // 任务类型，正态分布均值
  normal_distribution_stddev = TASK_CATEGORY_NUMS / 7.0; // 任务类型，正态分布方差
}

// 按算法名称归类显示数据
void
show_result(vector<vector<double>> &avg_wait_time_table, vector<vector<double>> &avg_real_resource_utilization_table) {
  vector<string> name{"pd", "su", "sa", "ku", "ka"};
  cout << "name\tavg_wait_time" << endl;
  for (int i = 0; i < 5; i++) {
    cout << name[i] << "\t";
    for (auto t: avg_wait_time_table[i])
      cout << t << ", ";
    cout << endl;
  }
  cout << "name\tavg_real_resource_utilization" << endl;
  for (int i = 0; i < 5; i++) {
    cout << name[i] << "\t";
    for (auto t: avg_real_resource_utilization_table[i])
      cout << t << ", ";
    cout << endl;
  }
}

// 指标随任务密度变化
void test_exponential_distribution_lambda() {
  param_reset();
  vector<vector<double>> avg_wait_time_table(5);
  vector<vector<double>> avg_real_resource_utilization_table(5);
  for (exponential_distribution_lambda = 1;
       exponential_distribution_lambda < 16; exponential_distribution_lambda += 1) {
    cout << ">>>>> exponential_distribution_lambda = " << exponential_distribution_lambda << endl;
    simulate_one_turn(avg_wait_time_table, avg_real_resource_utilization_table);
  }
  show_result(avg_wait_time_table, avg_real_resource_utilization_table);
}

// 指标随边缘节点个数变化
void test_EDGE_NODE_NUMS() {
  param_reset();
  vector<vector<double>> avg_wait_time_table(5);
  vector<vector<double>> avg_real_resource_utilization_table(5);
  for (EDGE_NODE_NUMS = mapped_node_nums; EDGE_NODE_NUMS < mapped_node_nums * 16; EDGE_NODE_NUMS += mapped_node_nums) {
    cout << ">>>>> EDGE_NODE_NUMS = " << EDGE_NODE_NUMS << endl;
    CLOUD_NODE_INDEX = EDGE_NODE_NUMS; // cloud 服务器编号
    simulate_one_turn(avg_wait_time_table, avg_real_resource_utilization_table);
  }
  show_result(avg_wait_time_table, avg_real_resource_utilization_table);
}

// 指标随任务类型聚集程度变化
void test_normal_distribution_stddev() {
  param_reset();
  vector<vector<double>> avg_wait_time_table(5);
  vector<vector<double>> avg_real_resource_utilization_table(5);
  for (normal_distribution_stddev = 2; normal_distribution_stddev < 32; normal_distribution_stddev += 2) {
    cout << ">>>>> normal_distribution_stddev = " << normal_distribution_stddev << endl;
    simulate_one_turn(avg_wait_time_table, avg_real_resource_utilization_table);
  }
  show_result(avg_wait_time_table, avg_real_resource_utilization_table);
}

int main() {
  test_exponential_distribution_lambda();
  test_EDGE_NODE_NUMS();
  test_normal_distribution_stddev();
}