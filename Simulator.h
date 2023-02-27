//
// Created by 洪陈杰 on 2023/2/18.
//

#ifndef TASK_DEPLOYMENT_SIMULATOR_H
#define TASK_DEPLOYMENT_SIMULATOR_H

#include "experiment.h"

class Simulator {
  long long total_time; // 系统处理完所有任务花费的时间
  std::vector<std::vector<double>> real_resource_utilization_trace; // 每一帧的各容器的实际资源利用率 {{n0},{n1},{n2},{n3},{avg}}
  std::vector<std::vector<double>> expected_resource_utilization_trace; // 每一帧的各容器的期望资源利用率 {{n0},{n1},{n2},{n3},{avg}}
  void reset() {
    total_time = 0;
    expected_resource_utilization_trace.clear();
    expected_resource_utilization_trace.resize(mapped_node_nums + 1);
    real_resource_utilization_trace.clear();
    real_resource_utilization_trace.resize(mapped_node_nums + 1);
  }

  // 计算一帧的平均资源利用率
  void cal_resource_utilization(long long current_time) {
    std::vector<std::vector<double>> per_node_real_resource_utilization(mapped_node_nums + 1,
                                                                        std::vector<double>(EDGE_NODE_NUMS));
    std::vector<std::vector<double>> per_node_expected_resource_utilization(mapped_node_nums + 1,
                                                                            std::vector<double>(EDGE_NODE_NUMS));
    for (int i = 0; i < EDGE_NODE_NUMS; i++) {
      if (nodes[i].deploy_service.empty()) {
        continue;
      }
      // expected_resource_utilization = 1 - remain/total
      auto expected_resource_utilization = 1.0 - cal_resource_utilization_helper(nodes[i].remain_source, i);
      per_node_expected_resource_utilization[TO_MAPPED_NODE_TYPE(i)].push_back(expected_resource_utilization);
      per_node_expected_resource_utilization[mapped_node_nums].push_back(expected_resource_utilization);

      source_categories real_sc{0, 0, 0};
      for (auto s: nodes[i].deploy_service) {
        // real_resource_utilization = sum(real_use)/total
        if (services[s].waiting_tasks.empty() || tasks[services[s].waiting_tasks.front()].arrive_time >= current_time) {
          real_sc.cpu += get_per_node_service_date(s, i).free_source.cpu;
          real_sc.memory += get_per_node_service_date(s, i).free_source.memory;
          real_sc.gpu += get_per_node_service_date(s, i).free_source.gpu;
        } else {
          real_sc.cpu += get_per_node_service_date(s, i).busy_source.cpu;
          real_sc.memory += get_per_node_service_date(s, i).busy_source.memory;
          real_sc.gpu += get_per_node_service_date(s, i).busy_source.gpu;
        }
      }
      auto real_resource_utilization = cal_resource_utilization_helper(real_sc, i);
      per_node_real_resource_utilization[TO_MAPPED_NODE_TYPE(i)].push_back(real_resource_utilization);
      per_node_real_resource_utilization[mapped_node_nums].push_back(real_resource_utilization);
    }
    for (int i = 0; i < mapped_node_nums; i++) {
      expected_resource_utilization_trace[i].push_back(
              std::accumulate(per_node_expected_resource_utilization[i].begin(),
                              per_node_expected_resource_utilization[i].end(), 0.0) /
              (EDGE_NODE_NUMS / mapped_node_nums));
      real_resource_utilization_trace[i].push_back(
              std::accumulate(per_node_real_resource_utilization[i].begin(),
                              per_node_real_resource_utilization[i].end(), 0.0) /
              (EDGE_NODE_NUMS / mapped_node_nums));
    }
    expected_resource_utilization_trace[mapped_node_nums].push_back(
            std::accumulate(per_node_expected_resource_utilization[mapped_node_nums].begin(),
                            per_node_expected_resource_utilization[mapped_node_nums].end(), 0.0) /
            EDGE_NODE_NUMS);
    real_resource_utilization_trace[mapped_node_nums].push_back(
            std::accumulate(per_node_real_resource_utilization[mapped_node_nums].begin(),
                            per_node_real_resource_utilization[mapped_node_nums].end(), 0.0) /
            EDGE_NODE_NUMS);
  }

public:
// 对模拟结果的分析
  void analysis(std::vector<double> &avg_wait_time_table, std::vector<std::vector<double>> &avg_real_resource_utilization_table,
                std::vector<double> &benefitRate_table, std::vector<double> &victimRate_table) {
    long long wait_time_sum = 0.0;
    int benefitCnt = 0;
    int victimCnt = 0;
    // 平均任务响应时间
    for (int i = 0; i < TASK_NUMS; i++) {
      wait_time_sum += tasks[i].respond_time - tasks[i].request_time;

      if (tasks[i].process_node == CLOUD_NODE_INDEX) continue;
      auto cloud_sd = get_per_node_service_date(tasks[i].task_type, CLOUD_NODE_INDEX);
      if (tasks[i].respond_time - tasks[i].request_time < (cloud_sd.processing_time + 2 * cloud_sd.transmission_time))
        benefitCnt++;
      else victimCnt++;
    }

    auto benefitRate = (double) benefitCnt / TASK_NUMS;
    auto victimRate = (double) victimCnt / TASK_NUMS;
    auto avg_wait_time = (double) wait_time_sum / TASK_NUMS;

    std::vector<double> avg_expected_resource_utilization(mapped_node_nums + 1);
    for (int i = 0; i <= mapped_node_nums; i++) {
      avg_expected_resource_utilization[i] = std::accumulate(expected_resource_utilization_trace[i].begin(),
                                                             expected_resource_utilization_trace[i].end(), 0.0) /
                                             expected_resource_utilization_trace[0].size();
    }
    std::vector<double> avg_real_resource_utilization(mapped_node_nums + 1);
    for (int i = 0; i <= mapped_node_nums; i++) {
      avg_real_resource_utilization[i] =
              std::accumulate(real_resource_utilization_trace[i].begin(), real_resource_utilization_trace[i].end(),
                              0.0) /
              real_resource_utilization_trace[0].size();
    }


    avg_wait_time_table.push_back(avg_wait_time);
    avg_real_resource_utilization_table.push_back(avg_real_resource_utilization);
    benefitRate_table.push_back(benefitRate);
    victimRate_table.push_back(victimRate);

    std::cout << avg_wait_time << " [";
    for(int i = 0; i <= mapped_node_nums; i++) {
      std::cout << avg_real_resource_utilization[i] << "/" << avg_expected_resource_utilization[i] << " ";
    }
    std::cout << "] " << benefitRate << "/" << victimRate << " "
              << total_time << std::endl;
  }

  void run(Deployer &deployer, bool use_adjust) {
    reset();
    int t = 0; // 任务遍历标记
    for (long long clock = CLOCK_TICK;; clock += CLOCK_TICK) { // CLOCK_TICKms 一次，计算整个时间线上任务处理情况
//      std::cout << "clock: " << clock << std::endl;

      bool has_remain_task = t < TASK_NUMS; // 标记是否还有任务需要处理

      // 1. 更新衰减负载，所有任务都衰减一次
      //      load_update_all();
      if (use_adjust)
        deployer.clock_tick_handler(clock);
      // 2. 处理这一个帧内的所有新到达的任务
      for (; t < TASK_NUMS && tasks[t].request_time < clock; t++) {
        service &s = services[tasks[t].task_type];

        // 设置任务到达时间
        tasks[t].arrive_time =
                tasks[t].request_time + get_per_node_service_date(tasks[t].task_type, s.node_index).transmission_time;
        // 空闲服务可以直接开始处理
        if (s.waiting_tasks.empty())
          tasks[t].processing_time = tasks[t].arrive_time;
        // 将新到达的任务挂入对应服务的等待队列
        s.waiting_tasks.push(t);

        // 如果当前调用的服务在云端，触发云边调整
//        if (s.node_index == CLOUD_NODE_INDEX && s.waiting_tasks.empty()) {
//          adjust(t); // 调整成功时需要插入一个迁移任务消耗时间
//        }
        if (use_adjust)
          deployer.new_task_handler(s, t);
      }

      // 计算这一帧的平均资源利用率
      cal_resource_utilization(clock);

      // 3. 处理每一个服务上待处理的任务队列
      for (auto &s: services) {
        while (!s.waiting_tasks.empty()) {
          has_remain_task = true;

          auto &cur_task = tasks[s.waiting_tasks.front()];
          // 队首任务的开始执行时间在这一帧之内，则需要被处理
          if (cur_task.processing_time >= clock)
            break;

          cur_task.process_node = s.node_index;
          cur_task.complete_time = cur_task.processing_time +
                                   get_per_node_service_date(s.task_type, s.node_index).processing_time;
          cur_task.respond_time =
                  cur_task.complete_time + get_per_node_service_date(s.task_type, s.node_index).transmission_time;
          // 计算这个任务在这一帧之内运行的时间
          int running_time =
                  std::min(clock, cur_task.complete_time) - std::max(cur_task.processing_time, clock - CLOCK_TICK);
          // 更新运行这个任务贡献的负载
//          load_update_single(s, running_time);
          if (use_adjust)
            deployer.task_process_handler(s, running_time);

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
      if (!has_remain_task) {
        total_time = clock;
        break;
      }
    }
  }
};

#endif //TASK_DEPLOYMENT_SIMULATOR_H
