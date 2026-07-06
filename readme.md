# Lcx 端口转发工具
## 工具用途
TCP端口转发工具，将本地端口的流量透明转发到指定目标服务器。常用于内网穿透、远程访问内网服务（如RDP、SSH、Web等）。

## 编译方法
### Visual Studio

```bash
cl lcx.c /Fe:lcx.exe
```
#### GCC
```bash
# windows
gcc -o lcx.exe lcx.c -lws2_32
# Linux
gcc -o lcx lcx.c -lpthread
```

使用方法
```bash
lcx <监听端口> <目标IP> <目标端口>
```

示例：
```bash
# 将本地5555端口转发到内网3389（RDP）
lcx 5555 192.168.1.10 3389

# 将本地8080端口转发到内网Web服务
lcx 8080 192.168.1.20 80
```

运行效果：
```
[+] Listening on port 5555
[+] Forwarding to 192.168.1.10:3389
[+] Client connected
[+] Connected to target
````

# 免责说明
本工具仅供合法的网络管理、渗透测试学习研究使用

使用者须遵守所在国家及地区的法律法规

禁止用于任何非法用途，一切后果由使用者自行承担

作者不承担任何因使用本工具而引发的法律责任
