const shell = require('shelljs');

var max = 10; // 开启 4 * max 个 BE 进程
var process = [];
for(var k = 0; k < max; k++) {
  for(var i = 0; i < 4; i++) {
    var p = shell.exec("./worker1 10", {async:true});
    process.push(p.pid)
  }
}

console.log(process)