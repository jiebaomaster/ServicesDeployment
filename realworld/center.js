import axios from 'axios'

let BE_services = [
  {
    url: "",
    usage: [{
      cpu: 0.2,
      mem: 0.1,
    }, {
      cpu: 0.3,
      mem: 0.1,
    }, {
      cpu: 0.4,
      mem: 0.1,
    }
    ]
  }, {}
]

let LS_services = [
  {
    url: ""
  }
]

let nodes = [
  {
    url: "192.168.3.3",
    cur_usage: { // 当前资源使用率
      cpu: 0,
      mem: 0,
    },
    avg_usage: { // 时间衰减资源使用率
      cpu: 0,
      mem: 0,
    },
    max_usage: { // 历史资源使用率峰值
      cpu: 0,
      men: 0,
    },
  }, {
    url: "192.168.2.50",
    cur_usage: { // 当前资源使用率
      cpu: 0,
      mem: 0,
    },
    avg_usage: { // 时间衰减资源使用率
      cpu: 0,
      mem: 0,
    },
    max_usage: { // 历史资源使用率峰值
      cpu: 0,
      men: 0,
    },
  }, {
    url: "192.168.2.98",
    cur_usage: { // 当前资源使用率
      cpu: 0,
      mem: 0,
    },
    avg_usage: { // 历史衰减资源使用率
      cpu: 0,
      mem: 0,
    },
    max_usage: { // 历史资源使用率峰值
      cpu: 0,
      men: 0,
    },
  }
]

let LS_tasks = [
  {
    type: 0, // 任务类型
    arrive_time: 0, // 到达时间
    response_time: 0, // 请求返回时间
  }
]

let BE_tasks = [
  {
    type: 0, // 任务类型
    arrive_time: 0, // 到达时间
  }
]

// 任务结束后进行数据分析
let analysis = () => {
  let sum_delay = 0
  LS_tasks.forEach(t => {
    sum_delay += t.response_time - t.arrive_time
  })
  let avg_delay = sum_delay / LS_tasks.length
}

// 负载衰减系数
const y = 0.1
// 定时获取集群资源利用率
let source_tick_interval = setInterval(() => {
  nodes.forEach((n) => {
    axios.get(n.url).then(r => {
      // 更新 实际资源使用情况
      n.cur_usage = JSON.parse(r.data)

      n.max_usage.cpu = Math.max(n.max_usage.cpu, n.cur_usage.cpu)
      n.max_usage.mem = Math.max(n.max_usage.mem, n.cur_usage.mem)

      n.avg_usage.cpu = n.avg_usage.cpu * y + n.cur_usage.cpu
      n.avg_usage.mem = n.avg_usage.mem * y + n.cur_usage.mem
    })
  })
}, 3000)

let checkDeploy = (curr_usage, task_type, node_index) => {
  return (1 - curr_usage.cpu) > BE_services[task_type][node_index].cpu
    && (1 - curr_usage.mem) > BE_services[task_type][node_index].mem
}

// 处理所有到达的 BE 任务
let BE_task_handler = (BE_cnt) => {
  // 寻找一个历史最小碎片满足部署要求的
  let target = -1
  for (let i = 0; i < nodes.length; i++) {
    if (checkDeploy(nodes[i].max_usage, BE_tasks[BE_cnt].type, i)) {
      target = i
      break
    }
  }
  // 寻找一个历史衰减资源满足部署要求的
  if (target === -1) {
    for (let i = 0; i < nodes.length; i++) {
      if (checkDeploy(nodes[i].avg_usage, BE_tasks[BE_cnt].type, i)) {
        target = i
        break
      }
    }
  }
  // 寻找一个当前实际剩余资源满足部署要求的
  if (target === -1) {
    for (let i = 0; i < nodes.length; i++) {
      if (checkDeploy(nodes[i].cur_usage, BE_tasks[BE_cnt].type, i)) {
        target = i
        break
      }
    }
  }

  if (target !== -1) {
    axios.get(nodes[target].url).then(r => {

    })
  }
  return target !== -1
}

let time_line = 0
const BE_tick = 20
let BE_queue = []
let BE_cnt = 0
let BE_tick_interval = setInterval(() => {
  time_line += BE_tick
  while (BE_cnt < BE_tasks.length && BE_tasks[BE_cnt].arrive_time < time_line) {
    BE_queue.push(BE_cnt++)
  }
  while (BE_queue.length) {
    if (BE_task_handler(BE_queue[0]))
      BE_queue.shift()
  }
}, BE_tick)

// 处理所有到达的 LS 任务
let LS_cnt = 0
let LS_task_handler = () => {
  LS_tasks[LS_cnt].arrive_time = Date.now()
  axios.get(nodes[LS_tasks[LS_cnt].type].url).then(r => {
    LS_tasks[LS_cnt].response_time = Date.now()
  })
  LS_cnt++
  if (LS_cnt < LS_tasks.length) {
    setTimeout(LS_task_handler, LS_tasks[LS_cnt].arrive_time)
  } else {
    clearInterval(source_tick_interval)
    clearInterval(BE_tick_interval)
    analysis()
  }
}
setTimeout(LS_task_handler, LS_tasks[0].arrive_time)