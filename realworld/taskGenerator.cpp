//
// Created by 洪陈杰 on 2023/3/8.
//


#include <random>
#include <iostream>
#include <map>
#include <vector>
#include <fstream>

using namespace std;

void generate_tasks(vector<pair<int, int>> &tasks, int task_nums, int task_category,
//                    double normal_distribution_mean,
//                    double normal_distribution_stddev,
                    double exponential_distribution_lambda) {
  mt19937 egen(1996);
  exponential_distribution<> ed(exponential_distribution_lambda);

  mt19937 ngen{6991};
//  normal_distribution<> nd{normal_distribution_mean, normal_distribution_stddev};
  uniform_int_distribution<> ud(0, task_category-1);
  int cur = 0;
  // 生成 TASK_NUMS 个按指数分布到达的随机任务，任务的种类服从随机分布
  for (int i = 0; i < task_nums; i++) {
    // 任务种类
//    int t = -1;
//    while (t < 0 || t >= task_category) {
//      t = round(nd(ngen));
//    }
    int t = ud(ngen);
    // 到达时间
    cur += ed(egen) * 100;

    tasks.push_back({t, cur});
  }
}

int main() {
  vector<pair<int, int>> LS_tasks;
  generate_tasks(LS_tasks, 10000, 16, 3);
  // [{type, arrive_time}]
  // 以写模式打开文件
  ofstream outfile;
  outfile.open("/Users/jbmaster/Desktop/ServicesDeployment/realworld/tasks.js");
  outfile << "let LS_tasks = [" << endl;
  for (auto t: LS_tasks) {
    outfile << "{ type:" << t.first << ", arrive_time: " << t.second << "}," << endl;
  }
  outfile << "]" << endl;

  vector<pair<int, int>> BE_tasks;
  generate_tasks(BE_tasks, 150, 6, 0.05);
  // [{type, arrive_time}]
  outfile << "let BE_tasks = [" << endl;
  for (auto t: BE_tasks) {
    outfile << "{ type:" << t.first << ", arrive_time: " << t.second << "}," << endl;
  }
  outfile << "]" << endl;

  outfile << "module.exports = {\n"
       << "  LS_tasks,\n"
       << "  BE_tasks,\n"
       << "}";
  // 关闭打开的文件
  outfile.close();
}