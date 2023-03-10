// content of index.js
const os = require('os');
const express = require('express')
const shell = require('shelljs');

let prev_cpu_usage = {}
let source_usage = {
  cpu: 0,
  mem: 0,
}
/**
 * 获取cpu花费的总时间与空闲模式的时间
 */
function getCPUInfo() {
  const cpus = os.cpus();
  let user = 0,
    nice = 0,
    sys = 0,
    idle = 0,
    irq = 0,
    total = 0;
  cpus.forEach(cpu => {
    const { times } = cpu;
    user += times.user;
    nice += times.nice;
    sys += times.sys;
    idle += times.idle;
    irq += times.irq;
  });
  total = user + nice + sys + idle + irq;
  return {
    total, // 总时间
    idle // 空闲时间
  };
}
prev_cpu_usage = getCPUInfo()
function getCPUUsage() {
  let curr_cpu_usage = getCPUInfo()
  const idleDiff = curr_cpu_usage.idle - prev_cpu_usage.idle;
  const totalDiff = curr_cpu_usage.total - prev_cpu_usage.total;
  source_usage.cpu = 1 - (idleDiff / totalDiff);
  prev_cpu_usage = curr_cpu_usage
}
function getMemUsage() {
  source_usage.mem = 1 - os.freemem() / os.totalmem();
}

const step = 3000
setInterval(() => {
  getMemUsage()
  getCPUUsage()
  // console.log(source_usage)
}, step)

let app = express();
app.get('/tick/',  (req, res) => {
  console.log(Date.now(), source_usage)
  res.send(JSON.stringify(source_usage));
})

// 执行 BE 任务
// todo BE 任务需要预先编译，需要调整运行类别
app.get('/run/:percent', (req, res) => {
  let num = parseInt(req.params.percent) / 10
  for (let j = 0; j < num; j++) {
    let total = Math.floor(Math.random()*5 + 1)
    for (let i = 0; i < os.cpus().length; i++) {
      // shell.exec(`/home/jbmaster/Desktop/ServicesDeployment/realworld/be 10 ${total}`, {async:true});
      shell.exec(`nice -n 19 /home/jbmaster/Desktop/ServicesDeployment/realworld/be 10 ${total}`, {async:true});
    }
  }
  res.send()
})

// 部署 LS 服务
app.get('/deploy/:type/:port', (req, res) => {
  let s = undefined
  switch (req.params.type) {
    case "redis":
      s = shell.exec(`sudo docker run -itd --rm -p ${req.params.port}:6379 -v /home/jbmaster/Desktop/redis.conf:/etc/redis/redis.conf redis`, (code, stdout, stderr) => {
        console.log('redis, Exit code:', code);
      })
      break
    case "http":
      s = shell.exec(`nohup node http_worker.js ${req.params.port} &`, (code, stdout, stderr) => {
        console.log('http, Exit code:', code);
      })
      break
    case "yolov5":
      s = shell.exec(`sudo docker run -itd --rm -p ${req.params.port}:8080 es_server:3`, (code, stdout, stderr) => {
        console.log('yolov5, Exit code:', code);
      })
      break
    default:
      console.error("deploy wrong service!")
  }
  res.end()
})

const port = 3000
let server = app.listen(port, function () {
  let host = server.address().address
  let port = server.address().port

  console.log("应用实例，访问地址为 http://%s:%s", host, port)
})