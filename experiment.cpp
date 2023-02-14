#include "experiment.h"
#include "PdDeployer.h"
#include <random>
#include <cstdlib>
#include <iostream>
#include <cmath>

using namespace std;

int CLOUD_NODE_INDEX = -1;

void initNodes() {
  // 初始化各节点的指标，实际只有5种类型
  node_statistics = {
          {}, // pi3
          {}, // pi4b
          {}, // jetson nano
          {}, // xavier nx
          {} // cloud
  };

  // 8 edge
  for(int i = 0; i < mapped_node_nums-1; i++) {
    nodes.emplace_back(i);
    nodes.back().remain_source = node_statistics[i].total_source;
    nodes.emplace_back(i);
    nodes.back().remain_source = node_statistics[i].total_source;
  }
  // 1 cloud
  CLOUD_NODE_INDEX = EDGE_NODE_NUMS - 1;
  nodes.emplace_back(CLOUD_NODE_INDEX );
  nodes.back().remain_source = node_statistics[mapped_node_nums - 1].total_source;
}

void initServices() {
  // 初始化各服务的性能指标，实际有 7 种类型
  service_statistics = {
          {},
          {},
          {},
          {},
          {},
          {},
          {}
  };

  // TASK_CATEGORY_NUMS 种服务，每种服务都对应一种任务类型 TO_MAPPED_TASK_TYPE(i)
  for(int i = 0; i < TASK_CATEGORY_NUMS; i++) {
    services.emplace_back(i);
  }
}

void initTasks() {
  tasks.resize(TASK_NUMS);
  double cur = 0.0;

  // 任务之间的时间间隔为指数分布
  mt19937 egen(1701);
  exponential_distribution<> ed(0.5);

  random_device rd{};
  mt19937 ngen{rd()};
  normal_distribution<> nd{TASK_CATEGORY_NUMS/2.0, TASK_CATEGORY_NUMS/5.0};

  // 生成 TASK_NUMS 个按指数分布到达的随机任务，任务的种类服从正态分布
  for(int i = 0; i < TASK_NUMS; i++) {
    // 任务种类
    int t = round(nd(ngen));
    if(t < 0) t = 0;
    else if(t >= TASK_CATEGORY_NUMS) t = TASK_CATEGORY_NUMS-1;
    tasks[i].task_type = t;
//    tasks[i].mapped_task_type = TO_MAPPED_TASK_TYPE(t);
    // 到达时间
    cur += ed(egen);
    tasks[i].request_time = (int)cur;

  }

  for(int i = 0; i < TASK_NUMS; i++) {
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