const axios = require('axios')
const Redis = require("ioredis")

let BE_type = [1, 3, 5] // cpu 0.1~0.5
let BE_services_statistics = {
  1: { // 1
    url: '/be/10',
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
  // 2: { // 2
  //   url: '/run2/',
  //   usage: [{
  //     cpu: 0.2,
  //     mem: 0.1,
  //   }, {
  //     cpu: 0.2,
  //     mem: 0.1,
  //   }, {
  //     cpu: 0.2,
  //     mem: 0.1,
  //   }]
  // },
  3: { // 3
    url: '/be/30',
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
  // 4: { // 4
  //   url: '/run4/',
  //   usage: [{
  //     cpu: 0.4,
  //     mem: 0.1,
  //   }, {
  //     cpu: 0.4,
  //     mem: 0.1,
  //   }, {
  //     cpu: 0.4,
  //     mem: 0.1,
  //   }]
  // },
  5: { // 5
    url: '/be/50',
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

let LS_type = ['redis', 'http', 'yolov5']
let LS_services_statistics = {
  redis: {
    url: 'redis',
    usage: [{ // i5-6500-4core 16g
      cpu: 0.3,
      mem: 0.1,
    }, { // i7-7700-8core 16g
      cpu: 0.2,
      mem: 0.2,
    }, { // i5-10400-12core 16g
      cpu: 0.2,
      mem: 0.2,
    }]
  },
  http: { // http-server
    url: '/http/',
    usage: [{
      cpu: 0.3,
      mem: 0.2,
    }, {
      cpu: 0.2,
      mem: 0.2,
    }, {
      cpu: 0.2,
      mem: 0.2,
    }]
  },
  yolov5: { // yolov5
    url: '/low/',
    usage: [{
      cpu: 0.3,
      mem: 0.1,
    }, {
      cpu: 0.2,
      mem: 0.1,
    }, {
      cpu: 0.2,
      mem: 0.1,
    }]
  },
}
let LS_services = []
let redis_cli = []
const BE_service_num = BE_type.length * 2
const LS_service_num = LS_type.length * 5
let init_services = () => {
  for (let i = 0; i < BE_service_num; i++)
    BE_services.push({type: BE_type[i % BE_type.length]})

  for (let i = 0; i < LS_service_num; i++)
    LS_services.push({type: LS_type[i % LS_type.length]})
}

let nodes = [
  { // edge
    url: "http://192.168.2.5",
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
    url: "http://192.168.2.50",
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
    url: "http://192.168.2.98",
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
    url: "http://192.168.2.6"
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
  service.url = nodes[target].url + ":" + nodes[target].port_cnt + services_statistics[service.type].url
  console.log(`${nodes[target].url}:3000/deploy/${service.type}/${nodes[target].port_cnt}`)
  axios.get(`${nodes[target].url}:3000/deploy/${service.type}/${nodes[target].port_cnt}`).then(r => {
    if (service.type === 'redis') {
      let cli = new Redis(service.port, nodes[target].url)
      cli.set('key', 'val')
      redis_cli.push(cli)
    }
  }).catch(e => {
    console.error('remove deploy ERR!', e.message)
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
  prev_deployment(true)
})()

module.exports = {
  nodes,
  redis_cli,
  BE_type,
  BE_services,
  LS_type,
  LS_services,
}