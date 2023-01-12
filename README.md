# Narcissus

一个Linux下C++高性能Web服务器项目

- 使用 线程池 + 非阻塞socket + epoll(ET和LT均实现) + 事件处理(Reactor和模拟Proactor均实现) 的并发模型
- 使用状态机解析HTTP请求报文，支持解析GET和POST请求
- 实现同步/异步日志系统，记录服务器运行状态
- 经Webbench压力测试可以实现上万的并发连接数据交换
- 使用多种数据结构实现定时器，支持定时关闭非活动连接
- 功能类似于Nginx的静态资源访问


# Narcissus

## 项目结构

```
Narcissus
├─CMakeLists.txt
├─config.json
├─start.sh
├─src
|  ├─config.cpp
|  ├─config.h
|  ├─main.cpp
|  ├─webserver.cpp
|  ├─webserver.h
|  ├─timer
|  |   ├─Utils.cpp
|  |   ├─Utils.h
|  |   ├─util_timer.h
|  |   ├─set
|  |   |  ├─set_timer.cpp
|  |   |  └set_timer.h
|  |   ├─heap
|  |   |  ├─heap_timer.cpp
|  |   |  └heap_timer.h
|  ├─threadpool
|  |     └threadpool.h
|  ├─mysql
|  |   ├─sql_connection_pool.cpp
|  |   └sql_connection_pool.h
|  ├─log
|  |  ├─log.cpp
|  |  └log.h
|  ├─lock
|  |  ├─block_queue.h
|  |  └locker.h
|  ├─http
|  |  ├─http_conn.cpp
|  |  └http_conn.h
```
## 快速运行

### 环境

- 服务器测试环境
  - Ubuntu版本20.04
  - MySQL版本8.0.31
  - gcc版本9.4.0
  - cmake版本3.16.3
- 浏览器测试环境
  - Chrome
  - FireFox

### 依赖安装

- mysqlclient
- jsoncpp

```bash
sudo apt-get install libmysqlclient-dev
sudo apt-get install libjsoncpp-dev
```

### 运行前的准备

- 创建数据库

```bash
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    passwd char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, passwd) VALUES('name', 'passwd');
```

- 修改配置文件config.json，修改相关配置

```json
{
  "config": {
    "port": 9000,
    "log_write": false,
    "trig_mode": false,
    "listen_trig_mode": false,
    "conn_trig_mode": false,
    "opt_linger": false,
    "sql_num": 8,
    "thread_num": 8,
    "close_log": false,
    "mysql": {
      "url": "localhost",
      "port": 3306,
      "username": "test",
      "password": "test",
      "database": "test"
    },
    "timer_mode": 0
  }
}
```

### 编译

```bash
./start.sh
```

### 运行

```bash
cd ./build
./Narcissus
```

## 使用

在build文件夹下创建root文件夹，将静态资源放入root文件夹下，即可通过浏览器访问localhost:9000









