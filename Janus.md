Janus是一个基础的webRTC服务器,它除了实现浏览器间webrtc媒体通信，和json消息交换，及rtp/rtcp传输外，不提供别的功能.其余的功能需要在服务器插件实现,比如回音消除，媒体录制等.

GitHub：<https://github.com/meetecho/janus-gateway>

### Ubuntu安装

![](./image/1563934732718.png)



### 新环境安装

sudo apt-get install aptitude

```
	aptitude install libmicrohttpd-dev libjansson-dev \
		libssl-dev libsrtp-dev libsofia-sip-ua-dev libglib2.0-dev \
		libopus-dev libogg-dev libcurl4-openssl-dev liblua5.3-dev \
		libconfig-dev pkg-config gengetopt libtool automake
		
	export PKG_CONFIG_PATH=/usr/lib/pkgconfig	
```

```
git clone https://gitlab.freedesktop.org/libnice/libnice
	cd libnice
	./autogen.sh
	./configure --prefix=/usr
	make && sudo make install
	
	pkg-config --cflags --libs nice
```

```
	wget https://github.com/cisco/libsrtp/archive/v2.2.0.tar.gz
	tar xfv v2.2.0.tar.gz
	cd libsrtp-2.2.0
	./configure --prefix=/usr --enable-openssl
	make shared_library && sudo make install
```

```
git clone https://github.com/warmcat/libwebsockets.git
cd libwebsockets
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_C_FLAGS="-fpic" ..
make && sudo make install
```

```
git clone https://github.com/meetecho/janus-gateway.git
sh autogen.sh
./configure --prefix=/opt/janus --enable-websockets
make
sudo make install
make configs
```

证书生成

```

# 生成rsa密钥
$ openssl genrsa -des3 -out server.key 1024
# 去除掉密钥文件保护密码
$ openssl rsa -in server.key -out server.key
# 生成ca对应的csr文件
$ openssl req -new -key server.key -out server.csr
# 自签名
$ openssl x509 -req -days 1024 -in server.csr -signkey server.key -out server.crt
$ cat server.crt server.key > server.pem
```

安装Nginx

```
sudo apt-get install nginx -y
sudo netstat -ntlp | grep nginx
http://localhost：80

vi /etc/nginx/conf.d/default.conf

server {
listen       80;
listen  *:443  ssl;
server_name  localhost;

location / {
     root /opt/janus/share/janus/demos;
     index index.html index.htm index.php;
}

 location /janus{
	proxy_set_header Host $host:$server_port;
	proxy_set_header X-Real-IP $remote_addr;
	proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
	proxy_pass http://192.168.44.131:8088/janus;
 }
 location /admin{
	proxy_set_header Host $host:$server_port;
	proxy_set_header X-Real-IP $remote_addr;
	proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
	proxy_pass http://192.168.44.131:7088/admin;
 }
 
 location ~/.*\.(bmp|gif|jpg|png|css|js|cur|flv|ico|swf|doc|pdf|html)$ {
	root /opt/janus/share/janus/demos;
	expires 1d;
 }
 
# redirect server error pages to the static page /50x.html
#
 error_page   500 502 503 504  /50x.html;
 location = /50x.html {
 	root   /usr/share/nginx/html;
 
 sl_certificate /home/q/janus_server/openssl_key/server.pem;
 sl_certificate_key /home/q/janus_server/openssl_key/server.key;
}
```



修改janus配置

```bash
vi /opt/janus/etc/janus/janus.jcfg

certificates: {
	cert_pem = "/home/q/janus_server/openssl_key/server.pem"
	cert_key = "/home/q/janus_server/openssl_key/server.key"
	#cert_pwd = "secretpassphrase"
}

 vi /opt/janus/etc/janus/janus.transport.http.jcfg
 
 general: {
	https = true	
}
admin: {
	admin_https = true
}
certificates: {
	cert_pem = "/home/q/janus_server/openssl_key/server.pem"
	cert_key = "/home/q/janus_server/openssl_key/server.key"
	#cert_pwd = "secretpassphrase"
	#ciphers = "PFS:-VERS-TLS1.0:-VERS-TLS1.1:-3DES-CBC:-ARCFOUR-128"
}
```



启动

sudo /opt/janus/bin/janus

sudo service nginx restart

sudo service nginx stop

service nginx status

使用google chrome进行访问https://192.168.44.131,使用firefox使用提示ICE错误.



*netstat -ntlp | grep nginx*

*netstat -ntlp | grep janus*

lsof - i:8080

