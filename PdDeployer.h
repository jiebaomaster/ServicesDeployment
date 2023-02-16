#ifndef __PDDEPLOYER_H__
#define __PDDEPLOYER_H__

#include "experiment.h"
#include <map>
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

  void load_update_single(service &s, int running_time) {
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
    s.weight += running_time * (double) (cur_sd.processing_time + cur_sd.transmission_time) /
                (cloud_sd.processing_time + cloud_sd.transmission_time);
    weight2serviceIndex.insert({s.weight, s.task_type});
  }

  bool adjust(int t) { // 云边调整，迁移 t 到 edge, t 为任务编号
    int service_index = tasks[t].task_type;
    //*** 1. 判断能否直接迁移
    for (int i = 0; i < EDGE_NODE_NUMS - 1; i++) {
      auto &sc = get_source_categories(service_index, i);
      if (checkDeploy(sc, nodes[i])) { // 如果可以直接迁移就不必卸载 edge 上的 service
        doDeploy(sc, nodes[i]);
        services[service_index].node_index = i;
        return true;
      }
    }
    //*** 2. 从权重最小的开始遍历直到 t，尝试释放权重更小的服务，找到的第一个可以部署 t 的 edge 节点就是迁移权重最小的节点
    std::vector<node> n = nodes;
    int targetNode = -1; // 选中的目标节点
    for (auto iter = weight2serviceIndex.begin(); iter->second != service_index; iter++) {
      // 跳过繁忙的服务和不在 edge 的服务
      if (!services[iter->second].waiting_tasks.empty() || services[iter->second].node_index == CLOUD_NODE_INDEX)
        continue;

      // 尝试取消部署
      source_categories &sc_cur = get_source_categories(iter->second, services[iter->second].node_index);
      chancelDeploy(sc_cur, n[services[iter->second].node_index]);
      // 如果可以部署成功了说明找到了目标 edge 节点
      if (checkDeploy(get_source_categories(service_index, services[iter->second].node_index),
                      n[services[iter->second].node_index])) {
        targetNode = services[iter->second].node_index;
        break;
      }
    }
    // 没找到可以部署的 edge 节点，迁移失败
    if (targetNode == -1) return false;
    std::cout << "adjust!" << std::endl;
    //*** 3. 执行迁移，将目标节点上的服务迁出一部分到 cloud，然后迁入 t
    auto &sc = get_source_categories(service_index, targetNode);
    for (auto iter = weight2serviceIndex.begin(); iter->second != service_index; iter++) {
      if (targetNode == services[iter->second].node_index && services[iter->second].waiting_tasks.empty()) {
        services[iter->second].node_index = CLOUD_NODE_INDEX; // 标记迁移回 cloud
        chancelDeploy(get_source_categories(iter->second, targetNode), nodes[targetNode]);
        nodes[targetNode].deploy_service.remove(iter->second);

        doDeploy(get_source_categories(iter->second, CLOUD_NODE_INDEX), nodes[CLOUD_NODE_INDEX]);
        nodes[CLOUD_NODE_INDEX].deploy_service.push_back(iter->second);

        // 对迁移到 cloud 的服务插入迁移任务
        task adjust2cloud_task;
        adjust2cloud_task.is_transmission_task = true; // 标记临时迁移任务
        adjust2cloud_task.arrive_time = // 消耗的时间是在 cloud 上重新部署的时间
                tasks[t].request_time + get_per_node_service_date(iter->second, CLOUD_NODE_INDEX).redeploy_time;
        adjust2cloud_task.processing_time = adjust2cloud_task.arrive_time;
        tasks.push_back(adjust2cloud_task);
        services[iter->second].waiting_tasks.push(tasks.size() - 1);

        // 如果可以部署成功了，执行迁移
        if (checkDeploy(sc, nodes[targetNode])) {
          doDeploy(sc, nodes[targetNode]);
          services[service_index].node_index = targetNode;
          chancelDeploy(get_source_categories(service_index, CLOUD_NODE_INDEX), nodes[CLOUD_NODE_INDEX]);
          nodes[CLOUD_NODE_INDEX].deploy_service.remove(service_index);

          // 对迁移到 targetNode 的服务插入了迁移任务
          task adjust2edge_task;
          adjust2edge_task.is_transmission_task = true; // 标记临时迁移任务
          adjust2edge_task.arrive_time = // 消耗的时间是在 targetNode 上重新部署的时间
                  tasks[t].request_time + get_per_node_service_date(service_index, targetNode).redeploy_time;
          if (services[service_index].waiting_tasks.empty())
            adjust2edge_task.processing_time = adjust2edge_task.arrive_time;
          tasks.push_back(adjust2edge_task);
          services[service_index].waiting_tasks.push(tasks.size() - 1);
        }

        break;
      }
    }

    return true; // 迁移失败
  }

public:
  void deployment() override {
    // 顺序遍历 tasks，尽量为每一个服务找一个 edge 节点部署
    for (int s = 0; s < TASK_CATEGORY_NUMS; s++) {
      for (int i = 0; i < EDGE_NODE_NUMS; ++i) { // 最后一个节点是 cloud 兜底
        source_categories &sc = get_source_categories(s, i);
        // 如果 n 可以部署 s，就部署 s
        if (checkDeploy(sc, nodes[i])) {
          services[s].node_index = i;
          // 初始负载设置为加速比
          services[s].weight = service_statistics[TO_MAPPED_TASK_TYPE(s)].initial_weight;
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

  void run() override {
    int t = 0; // 任务遍历标记
    for (int clock = 10;; clock += 10) { // 10ms 一次，计算整个时间线上任务处理情况
      std::cout << "clock: " << clock << std::endl;

      bool has_remain_task = false; // 标记是否还有任务需要处理

      // 1. 更新衰减负载，所有任务都衰减一次
      load_update_all();
      // 2. 处理这一个帧内的所有新到达的任务
      for (; t < TASK_NUMS && tasks[t].request_time < clock; t++) {
        service &s = services[tasks[t].task_type];

        // 如果当前调用的服务在云端，触发云边调整
        if (s.node_index == CLOUD_NODE_INDEX && s.waiting_tasks.empty()) {
          adjust(t); // 调整成功时需要插入一个迁移任务消耗时间
        }
        // 设置任务到达时间
        tasks[t].arrive_time =
                tasks[t].request_time + get_per_node_service_date(tasks[t].task_type, s.node_index).transmission_time;
        // 空闲服务可以直接开始处理
        if (s.waiting_tasks.empty())
          tasks[t].processing_time = tasks[t].arrive_time;
        // 将新到达的任务挂入对应服务的等待队列
        s.waiting_tasks.push(t);
      }

      // 计算这一帧的平均资源利用率
      cal_resource_utilization();

      // 3. 处理每一个服务上待处理的任务队列
      for (auto &s: services) {
        while (!s.waiting_tasks.empty()) {
          has_remain_task = true;

          auto &cur_task = tasks[s.waiting_tasks.front()];
          // 队首任务的开始执行时间在这一帧之内，则需要被处理
          if (cur_task.processing_time >= clock)
            break;

          cur_task.complete_time = cur_task.processing_time +
                                   get_per_node_service_date(s.task_type, s.node_index).processing_time;
          cur_task.respond_time =
                  cur_task.complete_time + get_per_node_service_date(s.task_type, s.node_index).transmission_time;
          // 计算这个任务在这一帧之内运行的时间
          int running_time =
                  std::min(clock, cur_task.complete_time) - std::max(cur_task.processing_time, clock - 10);
          // 更新运行这个任务贡献的负载
          load_update_single(s, running_time);
          // 本次任务处理超过了这一帧，不用继续遍历等待队列了
          if (cur_task.complete_time >= clock) {
            break;
          }

          // 从等待队列中删除已完成任务
          s.waiting_tasks.pop();
          // 由前一个任务的完成时间，更新下一个任务的开始处理时间
          if (!s.waiting_tasks.empty()) {
            auto &next_task = tasks[s.waiting_tasks.front()];
            next_task.processing_time = std::max(next_task.arrive_time, cur_task.complete_time);
          }
        }

      }

      // 如果没有任务要处理了就退出时间循环
      if (!has_remain_task) break;
    }
  }
};

#endif