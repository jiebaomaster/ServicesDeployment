#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>

#define SCHED_BT 7

int main(int argc, char **argv) {
  pid_t pid = getpid();
  struct sched_param sp = {.sched_priority = 0};
  // 设置使用 Batch 调度器
  int res = sched_setscheduler(pid, SCHED_BT, &sp);

  int percent = 50; // 运行时间占比
  int total = 5*1000; // 默认运行5秒
  if (argc > 1)
    percent = atoi(argv[1]);
  if (argc > 2)
    total = atoi(argv[2]) * 1000;

  printf("set cpu usage: %d%\n", percent);

  int worktime = percent;//ms
  int sleeptime = 100 - percent;

  struct timeval tv;
  long long start_time, end_time;
  while (total > 0) {
    gettimeofday(&tv, NULL);
    start_time = tv.tv_sec * 1000000 + tv.tv_usec;
    end_time = start_time;

    while ((end_time - start_time) < worktime * 1000) //60000
    {
      for (int i = 0; i < 1000000; i++);
      gettimeofday(&tv, NULL);
      end_time = tv.tv_sec * 1000000 + tv.tv_usec;
    }
    usleep(sleeptime * 1000); //60ms
    total -= 100;
  }
  return 0;
}