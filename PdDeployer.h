#ifndef __PDDEPLOYER_H__
#define __PDDEPLOYER_H__

#include "experiment.h"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cmath>

class PdDeployer : public Deployer {
private:
  double y = 0.97857206; // 衰减因子
  std::multimap<double, int> weight2serviceIndex; // 加速比权重树，所有服务
public:
  std::vector<std::vector<int>> migrate_trace; // 容器迁移轨迹 {task，service, from to}
private:
  void clear() {
    weight2serviceIndex.clear();
    migrate_trace.clear();
  }

  // 初始化将服务 s 部署到节点 n 的负载
  void load_init(int s, int n) {
    services[s].weight += CLOCK_TICK * cal_speedup_helper(s, n);
    weight2serviceIndex.insert({services[s].weight, s});
  }

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
    auto speedup = cal_speedup_helper(s.task_type, s.node_index);
    s.weight += (int)running_time * speedup;
    weight2serviceIndex.insert({s.weight, s.task_type});
  }


  /**
   * 在时间 time 时导致服务 s 从 from 节点迁移到 to 节点
   * @param t
   * @param service_index
   * @param from
   * @param to
   */
  void _do_migrate(long long time, int service_index, int from, int to) {
    migrate_trace.push_back({(int)time, service_index, from, to});

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
            time + get_per_node_service_date(service_index, to).redeploy_time;
    if (services[service_index].waiting_tasks.empty())
      adjust2edge_task.processing_time = adjust2edge_task.arrive_time;
    tasks.push_back(adjust2edge_task);
    services[service_index].waiting_tasks.push(tasks.size() - 1);
  }

  // 任务 t 导致服务 s 从 from 节点迁移到 to 节点
  void do_migrate_for_task(int t, int service_index, int from, int to) {
    // std::cout << "for task " << t << " migrate " << service_index << " from " << from << " to " << to << std::endl;
    _do_migrate(tasks[t].request_time, service_index, from, to);
  }

  bool adjust(int t) { // 云边调整，迁移 t 到 edge, t 为任务编号
    int service_index = tasks[t].task_type;
    std::vector<node> n = nodes;
    std::unordered_map<int, double> node_weight; // 需要迁出节点的权重
    std::unordered_set<int> candidate; // 备选节点
    std::unordered_set<int> loser; // 落选节点
    int targetNode = -1; // 选中的目标节点
    double max_score = -1;
    //*** 1. 判断能否直接迁移，如果可以直接迁移就不必卸载 edge 上的 service
    for (int i = 0; i < EDGE_NODE_NUMS; i++) {
      auto &sc = get_source_categories(service_index, i);
      auto redeploy_time = get_per_node_service_date(service_index, i).redeploy_time;
      auto speedup = cal_speedup_helper(service_index, i);
      if (speedup <= 1.0) { // 不能部署到获取不到加速的节点上
        loser.insert(i);
        continue;
      }
      if (checkDeploy(sc, nodes[i]) && services[service_index].weight > redeploy_time / 100.0) {
        candidate.insert(i);
        double score = services[service_index].weight / redeploy_time;
        if (score > max_score) {
          targetNode = i;
          max_score = score;
        }
      }
    }
    //*** 2. 从权重最小的开始遍历直到 t，尝试释放权重更小的服务，找到的第一个可以部署 t 的 edge 节点就是迁移权重最小的节点
    for (auto iter = weight2serviceIndex.begin(); iter->second != service_index; iter++) {
      int cur_node = services[iter->second].node_index;
      // 跳过 繁忙的服务 部署在云端的服务 备选节点上的服务 落选节点上的服务
      if (cur_node == CLOUD_NODE_INDEX
          || candidate.count(cur_node)
          || loser.count(cur_node))
        continue;

      // 尝试取消部署
      auto &sc_cur = get_source_categories(iter->second, cur_node);
      auto redeploy_time = get_per_node_service_date(service_index, cur_node).redeploy_time;
      chancelDeploy(sc_cur, n[cur_node]);
      node_weight[cur_node] += services[iter->second].weight;
      // todo 如果可以部署成功了说明找到了目标 edge 节点，权重差距应该和重部署时间相关
      if (checkDeploy(get_source_categories(service_index, cur_node), n[cur_node])
          && services[service_index].weight - node_weight[cur_node] > redeploy_time / 100.0) {
        candidate.insert(cur_node);
        // 综合权重差和重部署时间打分，选择分数最高的
        double score = (services[service_index].weight - node_weight[cur_node]) / redeploy_time;
        if (score > max_score) {
          targetNode = services[iter->second].node_index;
          max_score = score;
        }
      }
    }
    // 没找到可以部署的 edge 节点，迁移失败
    if (targetNode == -1) return false;
    //*** 3. 执行迁移
    if (node_weight[targetNode] == 0) { // 直接迁入
      do_migrate_for_task(t, service_index, CLOUD_NODE_INDEX, targetNode);
      return true;
    } else { // 将目标节点上的服务迁出一部分到 cloud，然后迁入 t
      auto &sc = get_source_categories(service_index, targetNode);
      for (auto iter = weight2serviceIndex.begin(); iter->second != service_index; iter++) {
        if (targetNode == services[iter->second].node_index) {
          do_migrate_for_task(t, iter->second, targetNode, CLOUD_NODE_INDEX);

          // 如果可以部署成功了，执行迁移
          if (checkDeploy(sc, nodes[targetNode])) {
            do_migrate_for_task(t, service_index, CLOUD_NODE_INDEX, targetNode);
            break;
          }
        }
      }
    }
    return true;
  }

public:
  void deployment() override {
    clear();
    // 将 service 按平均加速比从大到小排序
    std::vector<int> service_index(TASK_CATEGORY_NUMS);
    for(int i = 0; i < TASK_CATEGORY_NUMS; i++)
      service_index[i] = i;
    std::sort(service_index.begin(), service_index.end(), [](int ls, int rs) {
      return service_statistics[TO_MAPPED_TASK_TYPE(services[ls].task_type)].initial_weight >
             service_statistics[TO_MAPPED_TASK_TYPE(services[rs].task_type)].initial_weight;
    });

    // 顺序遍历 services，尽量为每一个服务找一个 edge 节点部署
    for (auto s : service_index) {
      int targetNode = -1;
      double max_score = -1;
      for (int i = 0; i < EDGE_NODE_NUMS; ++i) {
        source_categories &sc = get_source_categories(s, i);
        auto speedup = cal_speedup_helper(s, i);
        // 如果 n 可以部署 且 可以获得加速效果
        if (checkDeploy(sc, nodes[i]) && speedup > 1.0) {
          auto score = speedup + cal_resource_utilization_helper(nodes[i].remain_source, i);
          if (score > max_score) {
            targetNode = i;
            max_score = score;
          }
        }
      }

      if (targetNode == -1) targetNode = CLOUD_NODE_INDEX;
      services[s].node_index = targetNode;
      // 更新 n 的剩余资源
      doDeploy(get_source_categories(s, targetNode), nodes[targetNode]);
      nodes[targetNode].deploy_service.push_back(s);
      // 初始化负载
      load_init(s, targetNode);
    }
  }

  /**
  * swarm spread 部署策略
  * 如果节点配置相同，选择一个正在运行的容器数量最少的那个节点，即尽量平摊容器到各个节点
  */
  void deployment_swarm_spread() {
    clear();
    for (int s = 0; s < TASK_CATEGORY_NUMS; s++) {
      int targetNode = -1;
      for (int i = 0; i <= EDGE_NODE_NUMS; ++i) { // 最后一个节点是 cloud 兜底
        source_categories &sc = get_source_categories(s, i);
        // 如果 n 可以部署
        if (checkDeploy(sc, nodes[i])) {
          if (targetNode == -1 || nodes[targetNode].deploy_service.size() > nodes[i].deploy_service.size()) {
            targetNode = i;
          }
        }
      }
      services[s].node_index = targetNode;
      // 更新 n 的剩余资源
      doDeploy(get_source_categories(s, targetNode), nodes[targetNode]);
      nodes[targetNode].deploy_service.push_back(s);
      // 初始化负载
      load_init(s, targetNode);
    }
  }

  void deployment_k8s_NodeResourcesBalancedAllocation() {
    clear();
    for (int s = 0; s < TASK_CATEGORY_NUMS; s++) {
      int targetNode = -1;
      double min_score = 1.0;
      for (int i = 0; i < EDGE_NODE_NUMS; ++i) { // 最后一个节点是 cloud 兜底
        source_categories &sc = get_source_categories(s, i);
        // 如果 n 可以部署
        if (checkDeploy(sc, nodes[i])) {
          auto &cur_sd = get_per_node_service_date(s, i);
          // Abs[CPU(Request / Allocatable) - Mem(Request / Allocatable)]
          // 差值越大表示差距越大，应该分配到差距小的节点上
          auto score = abs((double) cur_sd.busy_source.cpu / nodes[i].remain_source.cpu -
                           (double) cur_sd.busy_source.memory / nodes[i].remain_source.memory);
          if (min_score > score) {
            min_score = score;
            targetNode = i;
          }
        }
      }
      if (targetNode == -1) targetNode = CLOUD_NODE_INDEX;
      services[s].node_index = targetNode;
      // 更新 n 的剩余资源
      doDeploy(get_source_categories(s, targetNode), nodes[targetNode]);
      nodes[targetNode].deploy_service.push_back(s);
      // 初始化负载
      load_init(s, targetNode);
    }
  }

  void clock_tick_handler(long long clock) override {
    load_update_all();

    // 每 CLOCK_TICK * 100 时间做一次边缘到云的迁移
    if (clock % (CLOCK_TICK * 100))
      return;

    for (int i = 0; i < EDGE_NODE_NUMS; i++) {
      // do_migrate 会改变 deploy_service，所以需要复制一份用于遍历
      auto deploy_service = nodes[i].deploy_service;
      for (int s : deploy_service) { // 迁移不能受益的节点到服务器上
        auto speedup_cur = cal_speedup_helper(s, i);
        if (speedup_cur <= 1.0) {
          _do_migrate(clock, s, i, CLOUD_NODE_INDEX);
        }
      }
    }
  }

  void new_task_handler(service &s, int t) override {
    if (s.node_index == CLOUD_NODE_INDEX) {
      adjust(t);
    }
  }

  void task_process_handler(service &s, long long running_time) override {
    load_update_single(s, running_time);
  }

};

#endif