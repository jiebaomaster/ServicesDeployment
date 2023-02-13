#ifndef __PDDEPLOYER_H__
#define __PDDEPLOYER_H__

#include "experment.h"
#include <map>
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

  void load_update_single(service &s, double running_time) {
    // 从权重树中删除原有节点
    for (auto it = weight2serviceIndex.begin(); it != weight2serviceIndex.end();) {
      if (it->first == s.weight && it->second == s.task_type) {
        weight2serviceIndex.erase(it);
        break;
      }
    }
    s.weight += running_time;
    weight2serviceIndex.insert({s.weight, s.task_type});
  }

  bool adjust(int s) { // 云边调整，迁移 s 到 edge, s 为任务编号
    // 任务 s 消耗的资源
    source_categories &sc = service_statistics[TO_MAPPED_TASK_TYPE(tasks[s].task_type)].busy_source;
    //*** 1. 判断能否直接迁移
    for (int i = 0; i < EDGE_NODE_NUMS; i++) {
      if (checkDeploy(sc, nodes[i])) { // 如果可以直接迁移就不必卸载 edge 上的 service
        doDeploy(sc, nodes[i]);
        services[tasks[s].task_type].node_index = i;
        return true;
      }
    }
    //*** 2. 从权重最小的开始遍历直到 s，尝试释放权重更小的服务，直到某个 edge 可以部署 s
    std::vector<node> n = nodes;
    int targetNode = -1; // 选中的目标节点
    for (auto iter = weight2serviceIndex.begin(); iter->second != tasks[s].task_type; iter++) {
      // 跳过繁忙的服务和不在 edge 的服务
      if (!services[iter->second].waiting_tasks.empty() || services[iter->second].node_index == -1)
        continue;

      // 尝试取消部署
      source_categories &sc_cur = service_statistics[TO_MAPPED_TASK_TYPE(iter->second)].busy_source;
      chancelDeploy(sc_cur, n[services[iter->second].node_index]);
      // 如果可以部署成功了说明找到了目标 edge 节点
      if (checkDeploy(sc, n[services[iter->second].node_index])) {
        targetNode = services[iter->second].node_index;
        break;
      }
    }
    // 没找到可以部署的 edge 节点，迁移失败
    if (targetNode == -1) return false;
    //*** 3. 执行迁移
    for (auto iter = weight2serviceIndex.begin(); iter->second != tasks[s].task_type; iter++) {
      if (targetNode == services[iter->second].node_index) {
        services[iter->second].node_index = -1; // 标记迁移回 cloud
        source_categories &sc_cur = service_statistics[TO_MAPPED_TASK_TYPE(iter->second)].busy_source;
        chancelDeploy(sc_cur, nodes[targetNode]);
        // 如果可以部署成功了，执行迁移
        if (checkDeploy(sc, nodes[targetNode])) {
          doDeploy(sc, nodes[targetNode]);
          services[tasks[s].task_type].node_index = targetNode;
          break;
        }
      }
    }

    return true; // 迁移失败
  }

public:
  void deployment() override {
    // 顺序遍历 tasks，尽量为每一个服务找一个 edge 节点部署
    for (int s = 0; s < TASK_CATEGORY_NUMS; s++) {
      for (int i = 0; i < EDGE_NODE_NUMS; ++i) {
        source_categories &sc = service_statistics[TO_MAPPED_TASK_TYPE(services[s].task_type)].busy_source;
        // 如果 n 可以部署 s，就部署 s
        if (checkDeploy(sc, nodes[i])) {
          services[s].node_index = i;
          // 初始负载设置为加速比
          services[s].weight = service_statistics[TO_MAPPED_TASK_TYPE(services[s].task_type)].acceleration_ratio;
          // 更新 n 的剩余资源
          doDeploy(sc, nodes[i]);
          // 初始负载设置为加速比
          weight2serviceIndex.insert({services[s].weight, s});
        }
      }
    }
  }

  void run() override {
    int t = 0;
    for (double clock = 1;; clock++) { // 1ms 一次，计算整个时间线上任务处理情况
      bool has_remain_task = false; // 标记是否还有任务需要处理

      // 1. 更新衰减负载，所有任务都衰减一次
      load_update_all();
      // 2. 处理这一个帧内的所有新到达的任务
      for (; t < TASK_CATEGORY_NUMS && tasks[t].request_time < clock; t++) {
        service &s = services[tasks[t].task_type];
        service_data &sd = service_statistics[TO_MAPPED_TASK_TYPE(tasks[t].task_type)];

        // 如果当前调用的服务在云端，触发云边调整
        if (s.node_index == -1) {
          if (adjust(t)) { // 调整成功时需要插入一个迁移任务消耗时间
            // todo 这里只对迁移到 edge 的服务插入了迁移任务，实际上迁移到 cloud 的服务也需要
            task adjustTask;
            adjustTask.is_transmission_task = true; // 标记临时迁移任务
            adjustTask.remain_time = sd.edge_reDeploy_time;
            adjustTask.arrive_time = tasks[t].request_time;
            if (s.waiting_tasks.empty())
              adjustTask.processing_time = adjustTask.arrive_time;
            tasks.push_back(adjustTask);
            s.waiting_tasks.push(tasks.size() - 1);
          }
        }
        // 将新到达的任务挂入对应服务的等待队列
        if (s.node_index == -1)
          tasks[t].arrive_time = tasks[t].request_time + sd.cloud_transmission_time;
        else
          tasks[t].arrive_time = tasks[t].request_time + sd.edge_transmission_time;
        // 空闲服务可以直接开始处理
        if (s.waiting_tasks.empty())
          tasks[t].processing_time = tasks[t].arrive_time;

        s.waiting_tasks.push(t);
      }

      // 3. 处理每一个服务上待处理的任务队列
      for (auto &s: services) {
        while (!s.waiting_tasks.empty()) {
          has_remain_task = true;

          auto &cur_task = tasks[s.waiting_tasks.front()];
          // 队首任务的开始执行时间在这一帧之内，则需要被处理
          if (cur_task.processing_time < clock) {
            if (s.node_index == -1) {
              cur_task.complete_time = cur_task.processing_time +
                                       service_statistics[TO_MAPPED_TASK_TYPE(s.task_type)].cloud_transmission_time;
              cur_task.respond_time = cur_task.complete_time +
                                      service_statistics[TO_MAPPED_TASK_TYPE(s.task_type)].cloud_transmission_time;
            } else {
              cur_task.complete_time = cur_task.processing_time +
                                       service_statistics[TO_MAPPED_TASK_TYPE(s.task_type)].edge_processing_time;
              cur_task.respond_time = cur_task.complete_time +
                                      service_statistics[TO_MAPPED_TASK_TYPE(s.task_type)].edge_transmission_time;
            }
            // 计算这个任务在这一帧之内运行的时间
            double running_time =
                    std::min(clock, cur_task.complete_time) - std::max(cur_task.processing_time, clock - 1);
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

      }

      // 如果没有任务要处理了就退出时间循环
      if (!has_remain_task) break;
    }
  }
};

#endif