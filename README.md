# 支持DNS SRV记录的 OpenVPN 3 核心库
You can find English version here [OpenVPN 3 with support for SRV record](README_EN.md), English doc still work in progress.

原版的 README 请访问 [原版README](ORIGIN_README.rst)

本项目为 [OpenVPN 3](https://github.com/OpenVPN/openvpn3) 添加查询DNS SRV记录的能力，旨在让 OpenVPN 获得动态端口访问的能力，这对于没有公网IP的用户但能通过TCP打洞的用户可能有帮助。

本人的 OpenVPN 服务端使用的是 Mikrotik 路由器提供的能力，对于其他类型的 OpenVPN 服务端，配置过程可能会有区别。

## 修改思路
我选择在 OpenVPN 的配置文件中（即.ovpn文件），通过添加额外的配置项来支持SRV功能，理论来说，任何基于 OpenVPN 3 库开发的UI程序，都可以引入这个功能。

> 但需要注意的是，对于使用原版核心的 OpenVPN 客户端，应该没有办法正确解析本仓库所支持的ovpn文件。

OpenVPN 3 原版通过 ASIO 查询域名对应的A记录或AAAA记录，但是 ASIO 库本质是通过 getaddrinfo 方法来查询域名对应的IP地址，似乎没有特别好的方法能通过 ASIO 库来进行 SRV 记录的查询。

为了偷懒，我选择引入 ldns 库来实现 SRV 记录的查询。

OpenVPN 3 建立连接前，会解析链路层协议，TCP、UDP或HTTP代理，各个链路以 `transport_start()` 为入口并调用 `start_connect_()` 启动连接，连接前会通过`get_endpoint()` 方法从 `remotelist.hpp` 中获取到 **endpoint** 信息，其包含服务器地址和端口信息。

我选择在 `get_endpoint()` 的过程中判断是否有配置 srv dns域名，如果配置了，则通过 ldns 查询域名的 srv 记录，并从中提取出端口信息，重写到 **endpoint** 信息里。

```bash
Fri Aug 16 00:34:13.836 2024 EVENT: RESOLVE
class 1, ttl 21, priority 40, weight 40, port 16520, host xxxxxx
Fri Aug 16 00:34:13.841 2024 SRV domain exist, override server port to [16520]
Fri Aug 16 00:34:13.841 2024 Contacting 113.xxx.xxx.100:16520 via TCP
```




## 编译教程
由于项目引入了 ldns 模块辅助 srv 记录的查询，因此环境配置与编译与原本有一些区别

### 在 macOS 上编译 OpenVPN 3
首先确保安装好工程依赖
```shell
brew install asio cmake jsoncpp lz4 openssl pkg-config xxhash ldns
```

克隆本仓库
```shell
git clone https://github.com/jdjingdian/openvpn3_srv.git
```

进入仓库目录并创建build文件夹
```shell
cd openvpn3_srv && mkdir build && cd build
```

#### 对于 ARM64 Apple Silicon 架构的 mac 设备
> 我安装的ldns版本号是1.8.4， 请自行根据版本号替换文件夹目录
```shell
cmake -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl -DCMAKE_PREFIX_PATH=/opt/homebrew ../ -DLDNS_INCLUDE_DIR=/opt/homebrew/Cellar/ldns/1.8.4/include
cmake --build .
```

#### 对于 Intel 架构的 mac设备
> 由于我手上暂时没有 Intel 芯片的 mac，所以DLDNS_INCLUDE_DIR指向的目录可能不准确，如果这个编译命令不正确，麻烦提 issue 告诉我正确的路径
```shell
cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl  -DCMAKE_PREFIX_PATH=/usr/local/opt ../ -DLDNS_INCLUDE_DIR=/opt/homebrew/Cellar/ldns/1.8.4/include
cmake --build .
```

### 在 Ubuntu 22.04 上编译 OpenVPN 3
首先安装依赖
```shell
sudo apt install --no-install-recommends ca-certificates cmake g++ git iproute2 ninja-build pkg-config
sudo apt install --no-install-recommends libasio-dev libcap-dev liblz4-dev libjsoncpp-dev libssl-dev libxxhash-dev
sudo apt-get install libldns-dev
```

克隆本仓库
```shell
git clone https://github.com/jdjingdian/openvpn3_srv.git
```

进入仓库目录并创建build文件夹
```shell
cd openvpn3_srv && mkdir build && cd build
```

编译依赖 OpenSSL 的 OpenVPN 3 库 ( 需要mbedTLS的话可以参考原版编译教程 )
```shell
cd openvpn3_srv && mkdir build && cd build
cmake -GNinja .. -DLDNS_INCLUDE_DIR=/usr/include/
cmake --build .
ctest # Run Unit Tests 这一步似乎并不一定要执行
```

### 在 Windows X64 上编译与使用
由于 Windows 平台的编译与使用与 macOS / Linux 环境有较大不同，请查看 [Windows 平台编译与使用教程](WINDOWS_USAGE.md)


## 使用 OpenVPN 3 SRV
请参考上面的文档，尝试自己编译该项目，或者使用项目 release 页面提供的编译好的 cli 程序。

相比原版 OpenVPN, 本仓库需要在 ovpn 配置文件中增加一行 remote-srv 的配置，remote-srv 指向你域名的 SRV 记录，改记录应该记录你当前公网的端口信息。
remote 部分需要填写一个域名和一个随机的端口号，因为当 remote-srv 配置存在且能正确查询时，remote 配置的端口号信息会被覆盖。yourserver.com 需要指向当前你设备的公网IP。

```shell
client
dev tun
proto tcp
remote youserver.com 65535
remote-srv you_srv_domain.com
nobind
persist-key
persist-tun
tls-client
remote-cert-tls server
verb 4
mute 10
cipher AES-256-CBC
auth SHA1
auth-user-pass
auth-nocache
route-nopull
<ca>
-----BEGIN CERTIFICATE-----
xxx
-----END CERTIFICATE-----
</ca>

<cert>
-----BEGIN CERTIFICATE-----
xxx
</cert>

<key>
-----BEGIN ENCRYPTED PRIVATE KEY-----
xxx
-----END ENCRYPTED PRIVATE KEY-----
</key>
```

由于我的 OpenVPN 服务端部署在 Mikrotik 路由器上，似乎只能用 AES-256-CBC，因此连接时需要带上`-Q` 配置

> --non-preferred-algorithms, -Q: Enables non preferred data channel algorithms

运行命令
```shell
sudo ./test/ovpncli/ovpncli -u username -p "password" -z "key_password" profile.ovpn -Q
```

理论上这样就可以成功连接，并能 ping 通服务端了。

## 感谢
感谢 [Natter](https://github.com/MikeWang000000/Natter)、[natmap](https://github.com/heiher/natmap)、[Lucky](https://github.com/gdy666/lucky) 等项目，让非公网 IP 的用户更轻松地实现通过 TCP 打洞的方式获得连回家里服务的能力。