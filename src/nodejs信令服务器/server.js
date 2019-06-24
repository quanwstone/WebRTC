const static = require('./node_modules/node-static');
const http = require('http');
const file = new(static.Server)();
const app = http.createServer(function (req, res) {
  file.serve(req, res);
}).listen(2013);

const io = require('socket.io').listen(app); //侦听 2013

io.sockets.on('connection', (socket) => {

  // convenience function to log server messages to the client
  function log(){ 
    const array = ['>>> Message from server: ']; 
    for (var i = 0; i < arguments.length; i++) {
      array.push(arguments[i]);
    } 
      socket.emit('log', array);
  }

  socket.on('message', (message) => { //收到message时，进行广播
    log('Got message:', message);
    // for a real app, would be room only (not broadcast)
    socket.broadcast.emit('message', message); //在真实的应用中，应该只在房间内广播
  });

  socket.on('create or join', (room) => { //收到 “create or join” 消息

	var clientsInRoom = io.sockets.adapter.rooms[room];
    var numClients = clientsInRoom ? Object.keys(clientsInRoom.sockets).length : 0; //房间里的人数

    log('Room ' + room + ' has ' + numClients + ' client(s)');
    log('Request to create or join room ' + room);

    if (numClients === 0){ //如果房间里没人
      socket.join(room);
      socket.emit('created', room); //发送 "created" 消息
    } else if (numClients === 1) { //如果房间里有一个人
	  io.sockets.in(room).emit('join', room);
      socket.join(room);
      socket.emit('joined', room); //发送 “joined”消息
    } else { // max two clients
      socket.emit('full', room); //发送 "full" 消息
    }
    socket.emit('emit(): client ' + socket.id +
      ' joined room ' + room);
    socket.broadcast.emit('broadcast(): client ' + socket.id +
      ' joined room ' + room);

  });

});