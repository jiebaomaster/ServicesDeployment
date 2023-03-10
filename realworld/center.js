const axios = require('axios')

let {BE_tasks, LS_tasks} = require('./tasks')
let {BE_services, LS_services, nodes, redis_cli, deploy_BE, BE_services_statistics} = require("./services")

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
  let total_cpu = []
  let total_mem = []
  total_cpu.length = nodes[0].history_usage.length
  total_mem.length = nodes[0].history_usage.length
  total_cpu.fill(0)
  total_mem.fill(0)
  for (let i = 0; i < nodes.length - 1; i++) {
    tickLogger.log(">>> n" + i)
    let c = []
    let c_sum = 0
    let m = []
    let m_sum = 0
    for (let j = 0; j < nodes[i].history_usage.length; j++) {
      let u = nodes[i].history_usage[j]
      c.push(u.cpu)
      c_sum += u.cpu
      m.push(u.mem)
      m_sum += u.mem
      total_cpu[j] += u.cpu
      total_mem[j] += u.mem
    }

    tickLogger.log('cpu: ', JSON.stringify(c))
    tickLogger.log('mem: ', JSON.stringify(m))
    tickLogger.log('avg_cpu_usage: ', c_sum / c.length)
    tickLogger.log('avg_mem_usage: ', m_sum / m.length)
  }

  tickLogger.log("*** total avg")
  let avg_cpu = total_cpu.map(n => n/(nodes.length - 1))
  let avg_mem = total_mem.map(n => n/(nodes.length - 1))
  tickLogger.log("cpu: ", JSON.stringify(avg_cpu))
  tickLogger.log("mem: ", JSON.stringify(avg_mem))
  tickLogger.log('avg_cpu: ', avg_cpu.reduce((a,b)=> a+b, 0) / avg_cpu.length)
  tickLogger.log('avg_mem: ', avg_mem.reduce((a,b)=> a+b, 0) / avg_mem.length)
}

function dump_task() {
  let delay = []
  let sum_delay = 0
  LS_tasks.forEach(t => {
    if (t.response_time === undefined)
      t.response_time = Date.now()
    delay.push(t.response_time - t.arrive_time)
    sum_delay += t.response_time - t.arrive_time
  })
  let avg_delay = sum_delay / LS_tasks.length
  taskLogger.log(JSON.stringify(delay))
  taskLogger.log('avg_delay: ' + avg_delay)
  delay.sort((a, b) => a - b)
  taskLogger.log(`p50 ${delay[5000]}, p95 ${delay[9500]}, p99 ${delay[9900]}, p999 ${delay[9990]}`)
}

// 任务结束后进行数据分析
let analysis = () => {
  dump_usage()
  dump_task()
}

// 负载衰减系数
const y = 0.8
let dumpCnt = 0
// 定时获取集群资源利用率
let source_tick_interval = () => {
  nodes.forEach((n) => {
    axios.get('http://'+n.url + ':3000/tick/').then(r => {
      // 更新 实际资源使用情况
      n.cur_usage = r.data
      n.max_usage.cpu = Math.max(n.max_usage.cpu, n.cur_usage.cpu)
      n.max_usage.mem = Math.max(n.max_usage.mem, n.cur_usage.mem)

      n.avg_usage.cpu = n.avg_usage.cpu * y + n.cur_usage.cpu
      n.avg_usage.mem = n.avg_usage.mem * y + n.cur_usage.mem

      n.history_usage.push({cpu: n.cur_usage.cpu, mem: n.cur_usage.mem})
    }).catch(e => {
      console.error("ERR! source tick ",e.message)
      console.error('http://'+n.url + ':3000/tick/')
    })
  })
  // 每隔 5 秒打印一次资源使用记录
  dumpCnt++
  if (dumpCnt === 5) {
    dumpCnt = 0
    // dump_usage()
  }
}

let checkDeploy = (curr_usage, task_type, node_index) => {
  return (1 - curr_usage.cpu) > BE_services_statistics[BE_services[task_type].type].usage[node_index].cpu
    && (1 - curr_usage.mem) > BE_services_statistics[BE_services[task_type].type].usage[node_index].mem
}

// 时间衰减权重
let SWD = (BE_cnt) => {
  // 寻找一个历史最小碎片满足部署要求的
  let target = -1
  for (let i = 0; i < nodes.length-1; i++) {
    if (checkDeploy(nodes[i].max_usage, BE_tasks[BE_cnt].type, i)) {
      target = i
      break
    }
  }
  // 寻找一个历史衰减资源满足部署要求的
  if (target === -1) {
    for (let i = 0; i < nodes.length-1; i++) {
      if (checkDeploy(nodes[i].avg_usage, BE_tasks[BE_cnt].type, i)) {
        target = i
        break
      }
    }
  }
  // 寻找一个当前实际剩余资源满足部署要求的
  if (target === -1) {
    for (let i = 0; i < nodes.length-1; i++) {
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
let seq = (BE_cnt) => {
  if (seqCnt + 1 === nodes.length)
    seqCnt = 0
  return seqCnt++
}

// 处理所有到达的 BE 任务
let BE_task_handler = (BE_cnt) => {
  let s = BE_services[BE_tasks[BE_cnt].type]
  let url = undefined
  if (deploy_BE) {
    url = 'http://' + s.url
  } else {
    // 选择执行节点
    let target = seq(BE_cnt)
    if (target !== -1)
      url = 'http://' + nodes[target].url + ":3000"+ BE_services_statistics[s.type].url
  }

  if (url) {
    axios.get(url)
    .then()
    .catch(e => {
      console.error("ERR! BE task ", e.message)
      console.error(url)
    })
  }

  return url !== undefined
}

let time_line = 0
const BE_tick = 20
let BE_queue = []
let BE_cnt = 0
let BE_tick_interval = () => {
  time_line += BE_tick
  while (BE_cnt < BE_tasks.length && BE_tasks[BE_cnt].arrive_time < time_line) {
    BE_queue.push(BE_cnt++)
  }
  while (BE_queue.length) {
    if (BE_task_handler(BE_queue[0]))
      BE_queue.shift()
    else break
  }
}

// 处理所有到达的 LS 任务
let LS_cnt = 0
let LS_task_handler = () => {
  let cur_LS_cnt = LS_cnt
  let service = LS_services[LS_tasks[cur_LS_cnt].type]
  let arrive_time = LS_tasks[cur_LS_cnt].arrive_time
  LS_tasks[cur_LS_cnt].arrive_time = Date.now()
  if (service.type === 'redis') {
    redis_cli[service.url].get('key').then(r => {
      LS_tasks[cur_LS_cnt].response_time = Date.now()
    }).catch(e => {
      console.error("ERR! redis_cli get", e.message)
    })

  } else {
    axios.get('http://' + service.url).then(r => {
      LS_tasks[cur_LS_cnt].response_time = Date.now()
    }).catch(e => {
      console.error("ERR! LS task ", e.message)
      console.error(service.url)
    })
  }

  LS_cnt++
  if (LS_cnt < LS_tasks.length) {
    setTimeout(LS_task_handler, LS_tasks[LS_cnt].arrive_time - arrive_time)
  } else {
    // 整个运行过程结束，等待 10 秒，保证所有请求返回
    setTimeout(() => {
      clearInterval(sti)
      clearInterval(bti)
      analysis()
      console.log("test complete")
    }, 15000)
  }
}
let sti = undefined
let bti = undefined;

// 开始测试
(function () {
  setTimeout(() => {
    sti = setInterval(source_tick_interval, 1000)
    bti = setInterval(BE_tick_interval, BE_tick)
    setTimeout(LS_task_handler, LS_tasks[0].arrive_time)

  }, 15000)
})()