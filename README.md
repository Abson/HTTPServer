### HTTPServer
利用 C++ 编写的 HTTP 服务器，使用线程池 + Reactor 模式

### 运行前更改
需要到 http_conn.cpp 第 6 行，更改 doc_root 为自己服务器的文件路径，默认为 /home/parallels/Desktop

### 如何运行？
$:cd half_sync_half_reactor

$:make

$:./webserver.out x.x.x.x:x (自己的ip地址和端口号)

### 运行结果
![](https://github.com/Abson/HTTPServer/blob/master/screenshoot/WechatIMG4863.jpeg)
