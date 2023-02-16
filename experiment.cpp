#include "experiment.h"
#include "PdDeployer.h"
#include <random>
#include <cstdlib>
#include <iostream>
#include <cmath>

using namespace std;

void initNodes() {
  // 初始化各节点的指标，实际只有5种类型
  node_statistics = {
          {.total_source = { // pi3
                  400, 0, 0, 1024
          }},
          {.total_source = { // pi4b
                  400, 0, 0, 8192
          }},
          {.total_source = { // jetson nano
                  400, 100, 4096, 4096
          }},
          {.total_source = { // xavier nx
                  400, 100, 8192, 8192
          }},
          {.total_source = { // cloud，资源无限
                  INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX
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
  // 初始化各服务的性能指标，实际有 7 种类型
  service_statistics = {
          { // mobileNet_tf_lite
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3
                                  303, 870, 10,
                                  {0, 0, 0, 1},
                                  {26, 0, 1, 1},
                          },
                          { // pi4b
                                  145, 870, 10,
                                  {1, 0, 1, 1},
                                  {1,  0, 1, 1},
                          },
                          { // jetson nano
                                  153, 870, 10,
                                  {1, 1, 1, 1},
                                  {1,  1, 1, 1},
                          },
                          { // xavier nx
                                  214, 870, 10,
                                  {0, 0, 0, 1},
                                  {1,  1, 1, 1},
                          },
                          { // cloud
                                  10,  870, 10,
                                  {0, 0, 0, 1},
                                  {1,  1, 1, 1},
                          }
                  }},
          {// PoseEstimation
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ✅
                                  INT32_MAX, 0,   0,
                                  {INT32_MAX, 0, 0, 0},
                                  {INT32_MAX, 0, 0, 0}
                          },
                          { // pi4b
                                  971,       98,  10,
                                  {1,         0, 1, 1},
                                  {1,         0, 1, 1},
                          },
                          { // jetson nano
                                  37,        98,  10,
                                  {1,         1, 1, 1},
                                  {1,         1, 1, 1},
                          },
                          { // xavier nx
                                  29,        98,  10,
                                  {1,         1, 1, 1},
                                  {1,         1, 1, 1},
                          },
                          { // cloud
                                  33,        158, 10,
                                  {1,         1, 1, 1},
                                  {1,         1, 1, 1},
                          }
                  }},
          {// yolov3_tiny
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3
                                  303, 98, 2113,
                                  {0, 0, 0, 0},
                                  {0, 0, 0, 0},
                          },
                          { // pi4b
                                  145, 98, 10,
                                  {0, 0, 0, 0},
                                  {0, 0, 0, 0},
                          },
                          { // jetson nano
                                  153, 98, 10,
                                  {0, 0, 0, 0},
                                  {0, 0, 0, 0},
                          },
                          { // xavier nx
                                  214, 98, 10,
                                  {0, 0, 0, 0},
                                  {0, 0, 0, 0},
                          },
                          { // cloud
                                  10,  98, 10,
                                  {0, 0, 0, 0},
                                  {0, 0, 0, 0},
                          }
                  }},
          {// UNetSegmentation
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ✅
                                  INT32_MAX, 0,  0,
                                  {INT32_MAX, 0, 0, 0},
                                  {INT32_MAX, 0,  0, 0},
                          },
                          { // pi4b
                                  588,       98, 10,
                                  {0,         0, 0, 0},
                                  {95,        0,  0, 0},
                          },
                          { // jetson nano
                                  89,        98, 10,
                                  {0,         0, 0, 0},
                                  {43,        88, 0, 0},
                          },
                          { // xavier nx
                                  18,        98, 10,
                                  {0,         0, 0, 0},
                                  {49,        97, 0, 0},
                          },
                          { // cloud
                                  10,        98, 10,
                                  {0,         0, 0, 0},
                                  {0,         0,  0, 0},
                          }
                  }},
          {// MobileNetSSD
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ✅
                                  INT32_MAX, 0,  0,
                                  {INT32_MAX, 0,  0, 0},
                                  {INT32_MAX, 0,  0, 0},
                          },
                          { // pi4b ✅
                                  INT32_MAX, 0,  0,
                                  {INT32_MAX, 0,  0, 0},
                                  {INT32_MAX, 0,  0, 0},
                          },
                          { // jetson nano
                                  56,        10, 10,
                                  {0,         83, 0, 0},
                                  {36,        0,  0, 0},
                          },
                          { // xavier nx
                                  133,       10, 10,
                                  {0,         0,  0, 0},
                                  {50,        99, 0, 0},
                          },
                          { // cloud
                                  10,        10, 10,
                                  {1,         1,  1, 1},
                                  {1,         1,  1, 1},
                          }
                  }},
          {// hitch_proxy
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3
                                  303, 10, 10,
                                  {26, 0, 1, 1},
                                  {0, 0, 1, 1},
                          },
                          { // pi4b
                                  145, 10, 10,
                                  {1,  0, 1, 1},
                                  {1, 0, 1, 1},
                          },
                          { // jetson nano
                                  153, 10, 10,
                                  {1,  1, 1, 1},
                                  {1, 1, 1, 1},
                          },
                          { // xavier nx
                                  214, 10, 10,
                                  {1,  1, 1, 1},
                                  {1, 1, 1, 1},
                          },
                          { // cloud
                                  10,  10, 10,
                                  {1,  1, 1, 1},
                                  {1, 1, 1, 1},
                          }
                  }},
          {// nodejs_http_server
                  .initial_weight = 1,
                  .per_node_service_date = {
                          { // pi3 ✅
                                  55, 351,  1961,
                                  {0, 0, 0, 113},
                                  {121, 0, 0, 134},
                          },
                          { // pi4b ✅
                                  11, 351,  1718,
                                  {0, 0, 0, 74},
                                  {106, 0, 0, 114},
                          },
                          { // jetson nano ✅
                                  15, 351,  989,
                                  {0, 0, 0, 122},
                                  {116, 0, 0, 184},
                          },
                          { // xavier nx ✅
                                  14, 351,  794,
                                  {0, 0, 0, 107},
                                  {114, 0, 0, 160},
                          },
                          { // cloud ✅
                                  10, 1015, 701,
                                  {0, 0, 0, 112},
                                  {107, 0, 0, 164},
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
  exponential_distribution<> ed(20);

  random_device rd{};
  mt19937 ngen{rd()};
  normal_distribution<> nd{TASK_CATEGORY_NUMS / 2.0, TASK_CATEGORY_NUMS / 5.0};

  // 生成 TASK_NUMS 个按指数分布到达的随机任务，任务的种类服从正态分布
  for (int i = 0; i < TASK_NUMS; i++) {
    // 任务种类
    int t = round(nd(ngen));
    if (t < 0) t = 0;
    else if (t >= TASK_CATEGORY_NUMS) t = TASK_CATEGORY_NUMS - 1;
    tasks[i].task_type = t;
//    tasks[i].mapped_task_type = TO_MAPPED_TASK_TYPE(t);
    // 到达时间
    cur += ed(egen);
    tasks[i].request_time = (int) cur;

  }

  for (int i = 0; i < TASK_NUMS; i++) {
    cout << "<" << tasks[i].task_type << ", " << tasks[i].request_time << "> ";
  }
  cout << endl;
}

int main() {
  initNodes();
  initServices();
  initTasks();

  Deployer *d = new PdDeployer();
  d->deployment();
  d->run();
  d->analysis();
}