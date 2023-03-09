import axios from 'axios'
import {BE_tasks, LS_tasks} from './tasks'
import {BE_services, LS_services, nodes, redis_cli, LS_type} from "./services"

const fs = require('fs')

let options = {
  flags: 'w', //
  encoding: 'utf8', // utf8编码
}
let data = Date.now()
let tickLoggerFile = fs.createWriteStream('./log/tick_' + data + '.log', options)
let tickLogger = new console.Console(tickLoggerFile)
let taskLoggerFile = fs.createWriteStream('./log/task_' + data + '.log', options)
let taskLogger = new console.Console(taskLoggerFile)

function dump_usage() {
  tickLogger.log('-------', Date.now(), '--------')
  nodes.forEach((n, i) => {
    let c = []
    let c_sum = 0
    let m = []
    let m_sum = 0
    n.history_usage.forEach(u => {
      c.push(u.cpu)
      c_sum += u.cpu
      m.push(u.mem)
      m_sum += u.mem
    })
    tickLogger.log('n' + i + ' cpu: ', c)
    tickLogger.log('n' + i + ' mem: ', m)
    tickLogger.log('avg_cpu_usage: ', c_sum / c.length)
    tickLogger.log('avg_mem_usage: ', m_sum / m.length)
  })
}

function dump_task() {
  let delay = []
  let sum_delay = 0
  LS_tasks.forEach(t => {
    delay.push(t.response_time - t.arrive_time)
    sum_delay += t.response_time - t.arrive_time
  })
  let avg_delay = sum_delay / LS_tasks.length
  taskLogger.log(delay)
  taskLogger.log('avg_delay: ' + avg_delay)
}

// 任务结束后进行数据分析
let analysis = () => {
  dump_usage()
  dump_task()
}

// 负载衰减系数
const y = 0.1
let dumpCnt = 0
// 定时获取集群资源利用率
let source_tick_interval = setInterval(() => {
  nodes.forEach((n) => {
    axios.get(n.url + ':3000/tick/').then(r => {
      // 更新 实际资源使用情况
      n.cur_usage = JSON.parse(r.data)

      n.max_usage.cpu = Math.max(n.max_usage.cpu, n.cur_usage.cpu)
      n.max_usage.mem = Math.max(n.max_usage.mem, n.cur_usage.mem)

      n.avg_usage.cpu = n.avg_usage.cpu * y + n.cur_usage.cpu
      n.avg_usage.mem = n.avg_usage.mem * y + n.cur_usage.mem
    })
  })
  // 每个 5 秒打印一次资源使用记录
  dumpCnt++
  if (dumpCnt === 5) {
    dumpCnt = 0
    dump_usage()
  }
}, 1000)

let checkDeploy = (curr_usage, task_type, node_index) => {
  return (1 - curr_usage.cpu) > BE_services[task_type][node_index].cpu
    && (1 - curr_usage.mem) > BE_services[task_type][node_index].mem
}

// 时间衰减权重
let SWD = (BE_cnt) => {
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
  return target
}
// 随机部署
let rnd = (BE_cnt) => {
  return Math.floor((Math.random() * nodes.length))
}
// 顺序部署
let seqCnt = 0
let seq = () => {
  seqCnt++
  if (seqCnt === nodes.length)
    seqCnt = 0
  return seqCnt
}

// 处理所有到达的 BE 任务
let BE_task_handler = (BE_cnt) => {
  // 选择执行节点
  let target = SWD(BE_cnt)

  if (target !== -1) {
    axios.get(BE_services[BE_tasks[BE_cnt].type].url).then()
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

// 整个运行过程结束
function finish() {
  clearInterval(source_tick_interval)
  clearInterval(BE_tick_interval)
  analysis()
}

// 处理所有到达的 LS 任务
let LS_cnt = 0
let LS_task_handler = () => {
  let service = LS_services[LS_tasks[LS_cnt].type]
  let arrive_time = LS_tasks[LS_cnt].arrive_time
  LS_tasks[LS_cnt].arrive_time = Date.now()
  if (service.type === 'redis') {
    redis_cli[LS_tasks[LS_cnt].type / LS_type.length].get('key').then(r => {
      LS_tasks[LS_cnt].response_time = Date.now()
    })
  } else {
    axios.get(service.url).then(r => {
      LS_tasks[LS_cnt].response_time = Date.now()
    })
  }

  LS_cnt++
  if (LS_cnt < LS_tasks.length) {
    setTimeout(LS_task_handler, LS_tasks[LS_cnt].arrive_time - arrive_time)
  } else {
    finish()
  }
}
setTimeout(LS_task_handler, LS_tasks[0].arrive_time)