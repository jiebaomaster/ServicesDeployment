const axios = require('axios')
const Redis = require("ioredis")

// 标记 BE 任务是否独立部署
const deploy_BE = false
let BE_type = [1, 2, 3, 4, 1, 2] // cpu 0.1~0.5
let BE_services_statistics = {
  1: { // 1
    url: '/run/10',
    usage: [{
      cpu: 0.1,
      mem: 0.1,
    }, {
      cpu: 0.1,
      mem: 0.1,
    }, {
      cpu: 0.1,
      mem: 0.1,
    }]
  },
  2: { // 2
    url: '/run/20',
    usage: [{
      cpu: 0.2,
      mem: 0.1,
    }, {
      cpu: 0.2,
      mem: 0.1,
    }, {
      cpu: 0.2,
      mem: 0.1,
    }]
  },
  3: { // 3
    url: '/run/30',
    usage: [{
      cpu: 0.3,
      mem: 0.1,
    }, {
      cpu: 0.3,
      mem: 0.1,
    }, {
      cpu: 0.3,
      mem: 0.1,
    }]
  },
  4: { // 4
    url: '/run/40',
    usage: [{
      cpu: 0.4,
      mem: 0.1,
    }, {
      cpu: 0.4,
      mem: 0.1,
    }, {
      cpu: 0.4,
      mem: 0.1,
    }]
  },
  5: { // 5
    url: '/run/50',
    usage: [{
      cpu: 0.5,
      mem: 0.1,
    }, {
      cpu: 0.5,
      mem: 0.1,
    }, {
      cpu: 0.5,
      mem: 0.1,
    }]
  },
}
let BE_services = []

let LS_type = ['redis', 'http', 'http', 'redis', 'http', 'yolov5', 'redis', 'http', 'yolov5', 'redis', 'http', 'http', 'http', 'yolov5', 'http', 'http']
let LS_services_statistics = {
  redis: {
    url: '/redis/',
    usage: [{
      cpu: 0.25,
      mem: 0.1,
    }, {
      cpu: 0.2,
      mem: 0.1,
    }, {
      cpu: 0.2,
      mem: 0.1,
    }]
  },
  http: { // http-server
    url: '/http/',
    usage: [{
      cpu: 0.1,
      mem: 0.1,
    }, {
      cpu: 0.1,
      mem: 0.1,
    }, {
      cpu: 0.1,
      mem: 0.1,
    }]
  },
  yolov5: { // yolov5
    url: '/low/1',
    usage: [{
      cpu: 0.35,
      mem: 0.2,
    }, {
      cpu: 0.35,
      mem: 0.2,
    }, {
      cpu: 0.35,
      mem: 0.2,
    }]
  },
}
let LS_services = []
let redis_cli = {}
const BE_service_num = BE_type.length
const LS_service_num = LS_type.length
let init_services = () => {
  for (let i = 0; i < BE_service_num; i++)
    BE_services.push({type: BE_type[i % BE_type.length]})

  for (let i = 0; i < LS_service_num; i++)
    LS_services.push({type: LS_type[i % LS_type.length]})
}

let nodes = [
  { // edge
    url: "192.168.2.5",
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
      mem: 0,
    },
    history_usage: [], // 历史所有资源使用率
  }, { // edge
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
      mem: 0,
    },
    history_usage: [], // 历史所有资源使用率
  }, { // edge
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
      mem: 0,
    },
    history_usage: [], // 历史所有资源使用率
  }, { // cloud
    url: "192.168.2.6",
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
      mem: 0,
    },
    history_usage: [], // 历史所有资源使用率
  }
]

// 部署
function check_deploy(candi, remain) {
  return candi.cpu <= remain.cpu && candi.mem <= remain.mem
}

function do_deploy(candi, remain) {
  remain.cpu -= candi.cpu
  remain.mem -= candi.mem
}

function remote_deploy(service, services_statistics, target) {
  let port = nodes[target].port_cnt
  service.url = nodes[target].url + ":" + port + services_statistics[service.type].url
  axios.get(`http://${nodes[target].url}:3000/deploy/${service.type}/${port}`).then(r => {
    console.log("remote deploy succ!", service.url)
    if (service.type === 'redis') {
      setTimeout(() => {
        let cli = new Redis(port, nodes[target].url)
        cli.set('key', 'val')
        redis_cli[service.url] = cli
      }, 10000)
    }
  }).catch(e => {
    console.error('remote deploy ERR!', e.message, service.url)
  })

  nodes[target].port_cnt++
}

let prev_deployment = (deploy_BE) => {
  nodes.forEach(n => {
    n.remain_source = {cpu: 1, mem: 1}
    n.deploy_num = 0
    n.port_cnt = 6666
  })
  if (deploy_BE) {
    BE_services.forEach(s => {
      let target = -1
      let min_deploy_num = Infinity
      for (let i = 0; i < nodes.length - 1; i++) {
        if (check_deploy(BE_services_statistics[s.type].usage[i], nodes[i].remain_source)) {
          if (min_deploy_num > nodes[i].deploy_num) {
            min_deploy_num = nodes[i].deploy_num
            target = i
          }
        }
      }
      if (target === -1)
        console.error("ERR! BE cannot deploy at edge!", nodes)

      do_deploy(BE_services_statistics[s.type].usage[target], nodes[target].remain_source)
      nodes[target].deploy_num++
      s.url = nodes[target].url + ":3000"+ BE_services_statistics[s.type].url
    })

    console.log('BE_services ', BE_services)
  }

  LS_services.forEach(s => {
    let target = -1
    let min_deploy_num = Infinity
    for (let i = 0; i < nodes.length - 1; i++) {
      if (check_deploy(LS_services_statistics[s.type].usage[i], nodes[i].remain_source)) {
        if (min_deploy_num > nodes[i].deploy_num) {
          min_deploy_num = nodes[i].deploy_num
          target = i
        }
      }
    }
    if (target === -1) target = nodes.length - 1
    else
      do_deploy(LS_services_statistics[s.type].usage[target], nodes[target].remain_source)
    nodes[target].deploy_num++
    remote_deploy(s, LS_services_statistics, target)
  })
  console.log('LS_services ', LS_services)
}

// 执行准备工作
(function prepare() {
  init_services()
  prev_deployment(deploy_BE)
})()

module.exports = {
  nodes,
  deploy_BE,
  BE_type,
  BE_services,
  BE_services_statistics,
  LS_type,
  LS_services,
  LS_services_statistics,
  redis_cli,
}