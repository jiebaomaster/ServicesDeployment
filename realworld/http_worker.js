const express = require('express')

let app = express();

app.get('/http/',  (req, res) => {
  console.log(Date.now(), 'got http access!')
  for (let i = 0; i < 100000; i++);
  res.send();
})

let port_string = process.argv.slice(2)[0];
console.log(port_string)
let port = 6789
if(port_string) port = parseInt(port_string)
let server = app.listen(port, function () {
  let host = server.address().address
  let port = server.address().port

  console.log("http-worker >>> http://%s:%s", host, port)
})