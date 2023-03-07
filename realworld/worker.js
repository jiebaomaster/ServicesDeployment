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
  console.log(source_usage)
}, step)

let app = express();
app.get('/tick/',  (req, res) => {
  // console.log(source_usage)
  res.send(JSON.stringify(source_usage));
})

// 执行 ES 任务
app.get('/run', (req, res) => {
  shell.exec("ls");
  res.send()
})

let server = app.listen(3000, function () {
  let host = server.address().address
  let port = server.address().port

  console.log("应用实例，访问地址为 http://%s:%s", host, port)
})