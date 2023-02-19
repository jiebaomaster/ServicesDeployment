#ifndef __PDDEPLOYER_H__
#define __PDDEPLOYER_H__

#include "experiment.h"
#include <map>
#include <unordered_map>
#include <iostream>
#include <cmath>

class PdDeployer : public Deployer {
  double y = 0.97857206; // 衰减因子
  std::multimap<double, int> weight2serviceIndex; // 加速比权重树，所有服务

  void load_update_all() { // 更新所有负载
    std::multimap<double, int> tmp;
    for (auto iter = weight2serviceIndex.begin(); iter != weight2serviceIndex.end(); iter++) {
      tmp.insert({iter->first * y, iter->second});
      services[iter->second].weight = iter->first * y;
    }
    weight2serviceIndex = tmp;
  }

  void load_update_single(service &s, long long running_time) {
    // 从权重树中删除原有节点
    for (auto iter = weight2serviceIndex.begin(); iter != weight2serviceIndex.end(); iter++) {
      if (iter->first == s.weight && iter->second == s.task_type) {
        weight2serviceIndex.erase(iter);
        break;
      }
    }
    // todo 权重应该和运行时间和当前加速比有关
    auto &cur_sd = get_per_node_service_date(s.task_type, s.node_index);
    auto &cloud_sd = get_per_node_service_date(s.task_type, CLOUD_NODE_INDEX);
    s.weight += running_time
            *(double) (cloud_sd.processing_time + cloud_sd.transmission_time) / (cur_sd.processing_time + cur_sd.transmission_time);
    weight2serviceIndex.insert({s.weight, s.task_type});
  }

  /**
   * 任务 t 导致服务 s 从 from 节点迁移到 to 节点
   * @param t
   * @param service_index
   * @param from
   * @param to
   */
  void do_migrate(int t, int service_index, int from, int to) {
    std::cout << "for task " << t << " migrate " << service_index << " from " << from << " to " << to << std::endl;
    // 迁出
    chancelDeploy(get_source_categories(service_index, from), nodes[from]);
    nodes[from].deploy_service.remove(service_index);

    // 迁入
    doDeploy(get_source_categories(service_index, to), nodes[to]);
    nodes[to].deploy_service.push_back(service_index);
    services[service_index].node_index = to;

    // 对迁移到 targetNode 的服务插入了迁移任务消耗时间
    task adjust2edge_task;
    adjust2edge_task.is_transmission_task = true; // 标记临时迁移任务
    adjust2edge_task.arrive_time = // 消耗的时间是在 targetNode 上重新部署的时间
            tasks[t].request_time + get_per_node_service_date(service_index, to).redeploy_time;
    if (services[service_index].waiting_tasks.empty())
      adjust2edge_task.processing_time = adjust2edge_task.arrive_time;
    tasks.push_back(adjust2edge_task);
    services[service_index].waiting_tasks.push(tasks.size() - 1);
  }

  bool adjust(int t) { // 云边调整，迁移 t 到 edge, t 为任务编号
    int service_index = tasks[t].task_type;
    //*** 1. 判断能否直接迁移
    for (int i = 0; i < EDGE_NODE_NUMS - 1; i++) {
      auto &sc = get_source_categories(service_index, i);
      if (checkDeploy(sc, nodes[i])) { // 如果可以直接迁移就不必卸载 edge 上的 service
        do_migrate(t, service_index, CLOUD_NODE_INDEX, i);
        return true;
      }
    }
    //*** 2. 从权重最小的开始遍历直到 t，尝试释放权重更小的服务，找到的第一个可以部署 t 的 edge 节点就是迁移权重最小的节点
    std::vector<node> n = nodes;
    std::unordered_map<int, double> node_weight;
    int targetNode = -1; // 选中的目标节点
    for (auto iter = weight2serviceIndex.begin(); iter->second != service_index; iter++) {
      int cur_node = services[iter->second].node_index;
      // 跳过繁忙的服务和不在 edge 的服务
      if (!services[iter->second].waiting_tasks.empty() || cur_node == CLOUD_NODE_INDEX)
        continue;

      // 尝试取消部署
      auto &sc_cur = get_source_categories(iter->second, cur_node);
      auto redeploy_time = get_per_node_service_date(service_index, cur_node).redeploy_time;
      chancelDeploy(sc_cur, n[cur_node]);
      node_weight[cur_node] += services[iter->second].weight;
      // todo 如果可以部署成功了说明找到了目标 edge 节点，权重差距应该和重部署时间相关
      if (checkDeploy(get_source_categories(service_index, cur_node), n[cur_node])
          && services[service_index].weight - node_weight[cur_node] > redeploy_time/10) {
        targetNode = services[iter->second].node_index;
        break;
      }
    }
    // 没找到可以部署的 edge 节点，迁移失败
    if (targetNode == -1) return false;
    //*** 3. 执行迁移，将目标节点上的服务迁出一部分到 cloud，然后迁入 t
    auto &sc = get_source_categories(service_index, targetNode);
    for (auto iter = weight2serviceIndex.begin(); iter->second != service_index; iter++) {
      if (targetNode == services[iter->second].node_index && services[iter->second].waiting_tasks.empty()) {
        do_migrate(t, iter->second, targetNode, CLOUD_NODE_INDEX);

        // 如果可以部署成功了，执行迁移
        if (checkDeploy(sc, nodes[targetNode])) {
          do_migrate(t, service_index, CLOUD_NODE_INDEX, targetNode);
          break;
        }
      }
    }

    return true; // 迁移失败
  }

public:
  void deployment() override {
    // 顺序遍历 services，尽量为每一个服务找一个 edge 节点部署
    for (int s = 0; s < TASK_CATEGORY_NUMS; s++) {
      for (int i = 0; i <= EDGE_NODE_NUMS; ++i) { // 最后一个节点是 cloud 兜底
        source_categories &sc = get_source_categories(s, i);
        // 如果 n 可以部署 s，就部署 s
        if (checkDeploy(sc, nodes[i])) {
          services[s].node_index = i;

          auto &cur_sd = get_per_node_service_date(s, i);
          auto &cloud_sd = get_per_node_service_date(s, CLOUD_NODE_INDEX);
          services[s].weight += CLOCK_TICK * ((double)cur_sd.redeploy_time / 3500)
                      *(double) (cloud_sd.processing_time + cloud_sd.transmission_time) / (cur_sd.processing_time + cur_sd.transmission_time);
          // 初始负载设置为加速比
//          services[s].weight = service_statistics[TO_MAPPED_TASK_TYPE(s)].initial_weight;
          // 更新 n 的剩余资源
          doDeploy(sc, nodes[i]);
          nodes[i].deploy_service.push_back(s);
          // 初始负载设置为加速比
          weight2serviceIndex.insert({services[s].weight, s});
          break;
        }
      }
    }
  }

  void clock_tick_handler() override {
    load_update_all();
  }
  void new_task_handler(service& s, int t) override {
    if (s.node_index == CLOUD_NODE_INDEX && s.waiting_tasks.size() == 1) {
      adjust(t);
    }
  }
  void task_process_handler(service& s, long long running_time) override {
    load_update_single(s, running_time);
  }

  void deployment_swarm_spread() {
    // 如果节点配置相同，选择一个正在运行的容器数量最少的那个节点，即尽量平摊容器到各个节点
    for (int s = 0; s < TASK_CATEGORY_NUMS; s++) {
      int targetNode = -1;
      for (int i = 0; i <= EDGE_NODE_NUMS; ++i) { // 最后一个节点是 cloud 兜底
        source_categories &sc = get_source_categories(s, i);
        // 如果 n 可以部署 s，就部署 s
        if (checkDeploy(sc, nodes[i])) {
          if (targetNode == -1 || nodes[targetNode].deploy_service.size() > nodes[i].deploy_service.size()) {
            targetNode = i;
          }
        }
      }
      services[s].node_index = targetNode;
      // 初始负载设置为加速比
      services[s].weight = service_statistics[TO_MAPPED_TASK_TYPE(s)].initial_weight;
      // 更新 n 的剩余资源
      doDeploy(get_source_categories(s, targetNode), nodes[targetNode]);
      nodes[targetNode].deploy_service.push_back(s);
      // 初始负载设置为加速比
      weight2serviceIndex.insert({services[s].weight, s});
    }
  }


};

#endif