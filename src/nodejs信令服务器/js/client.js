var isInitiator;

room = prompt('Enter room name:'); //弹出一个输入窗口

const socket = io.connect(); //与服务端建立socket连接

if (room !== '') { //如果房间不空，则发送 "create or join" 消息
  console.log('Joining room ' + room);
  socket.emit('create or join', room);
}

socket.on('full', (room) => { //如果从服务端收到 "full" 消息
  console.log('Room ' + room + ' is full');
});

socket.on('empty', (room) => { //如果从服务端收到 "empty" 消息
  isInitiator = true;
  console.log('Room ' + room + ' is empty');
});

socket.on('join', (room) => { //如果从服务端收到 “join" 消息
  console.log('Making request to join room ' + room);
  console.log('You are the initiator!');
});

socket.on('log', (array) => {
  console.log.apply(console, array);
});