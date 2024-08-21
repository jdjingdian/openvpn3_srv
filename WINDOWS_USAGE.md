# Windows 平台编译与使用教程
我在 Windows 平台尝试进行编译，但是在配置环境上花了很大的周折，由于 ldns 是一个基于 posix 的库，所以要使用 mingw-w64 进行编译。

建议使用 Ubuntu 22.04 进行交叉编译，基本不会有坑。。。

如果不想折腾编译，也可以到 [release](https://github.com/jdjingdian/openvpn3_srv/releases) 页面下载我编译好的输出目录，然后根据文档最后的使用教程进行配置。

## 编译

### 安装环境依赖
安装 mingw-w64 支持交叉编译的版本
```shell
sudo apt-get install gcc-mingw-w64-x86-64-posix g++-mingw-w64-x86-64-posix
```

克隆 vcpkg 项目，并运行准备脚本
```shell
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.sh
```
完成后会有如下输出
```shell
$ ./bootstrap-vcpkg.sh 
Downloading vcpkg-glibc...
vcpkg package management program version 2024-08-01-fd884a0d390d12783076341bd43d77c3a6a15658

See LICENSE.txt for license information.
Telemetry
---------
vcpkg collects usage data in order to help us improve your experience.
The data collected by Microsoft is anonymous.
You can opt-out of telemetry by re-running the bootstrap-vcpkg script with -disableMetrics,
passing --disable-metrics to vcpkg on the command line,
or by setting the VCPKG_DISABLE_METRICS environment variable.

Read more about vcpkg telemetry at docs/about/privacy.md
```

最后下载 ldns 的预编译库，下载链接： https://nlnetlabs.nl/~wouter/ldns-1.8.3_20240130.zip

其实这里我是想尝试自己编译 ldns 的，但是使用官方的 makewin.sh 脚本没有成功编译。。。

解压 ldns-1.8.3_20240130.zip

进入 include/ldns 目录，找到util.h，注释第18行的 `#include <unistd.h>`，因为这个头文件会在 mingw 编译的过程中引起如下报错
```shell
/home/magicdian/magicdata/openvpn3_srv/test/ovpncli/cli.cpp:1320:21: error: reference to ‘optind’ is ambiguous
 1320 |             argv += optind;
      |                     ^~~~~~
In file included from /home/magicdian/magicdata/openvpn3_srv/test/ovpncli/cli.cpp:69:
/home/magicdian/magicdata/openvpn3_srv/cmake/../openvpn/common/getopt.hpp:55:5: note: candidates are: ‘int openvpn::optind’
   55 | int optind = 1;               /* index into parent argv vector */
      |     ^~~~~~
In file included from /usr/share/mingw-w64/include/unistd.h:12,
                 from /home/magicdian/magicdata/ldns-bin/include/ldns/util.h:18,
                 from /home/magicdian/magicdata/ldns-bin/include/ldns/ldns.h:95,
                 from /home/magicdian/magicdata/openvpn3_srv/cmake/../openvpn/client/remotelist.hpp:54,
                 from /home/magicdian/magicdata/openvpn3_srv/cmake/../openvpn/transport/client/udpcli.hpp:37,
                 from /home/magicdian/magicdata/openvpn3_srv/cmake/../openvpn/client/cliopt.hpp:54,
                 from /home/magicdian/magicdata/openvpn3_srv/cmake/../openvpn/client/cliconnect.hpp:60,
                 from /home/magicdian/magicdata/openvpn3_srv/cmake/../client/ovpncli.cpp:95,
                 from /home/magicdian/magicdata/openvpn3_srv/test/ovpncli/cli.cpp:63:
/usr/share/mingw-w64/include/getopt.h:22:12: note:                 ‘int optind’
   22 | extern int optind;  /* index of first non-option in argv      */
      |            ^~~~~~
gmake[3]: *** [test/ovpncli/CMakeFiles/ovpncli.dir/build.make:77: test/ovpncli/CMakeFiles/ovpncli.dir/cli.cpp.obj] Error 1
gmake[2]: *** [CMakeFiles/Makefile2:1161: test/ovpncli/CMakeFiles/ovpncli.dir/all] Error 2
gmake[1]: *** [CMakeFiles/Makefile2:1168: test/ovpncli/CMakeFiles/ovpncli.dir/rule] Error 2
gmake: *** [Makefile:621: ovpncli] Error 2
```

虽然这样注释掉的操作有点不够严谨，但是，程序员和代码有一个能跑就行了

```c
#include <sys/types.h>
//#include <unistd.h>
#include <ldns/common.h>
```

你也可以直接在 [release](https://github.com/jdjingdian/openvpn3_srv/releases) 页面下载我修改好的 `ldns-1.8.3_20240130_modified.zip`

**建议新建一个目录，然后再解压压缩包。**

最后，拷贝 `libldns.dll.a` 到 `/usr/lib` 目录
```shell
sudo cp libldns.dll.a /usr/lib
```

### 编译
拉取 openvpn 3 srv 仓库
`git clone https://github.com/jdjingdian/openvpn3_srv`

进入仓库并配置 `VCPKG_ROOT` 环境变量, 指向之前你克隆的 vcpkg 的根目录即可

> 目前 Linux 与 macOS 的编译命令与官方原版一致，Windows 版本仍需手动指定 `LDNS_INCLUDE_DIR` 才能正常编译

```shell
export VCPKG_ROOT=/home/magicdian/magicdata/vcpkg # 改成你自己的
cd openvpn3_srv # 进入项目根目录
cmake --preset mingw-x64-release -DLDNS_INCLUDE_DIR=/home/magicdian/magicdata/ldns-bin/include # 改成 ldns 解压包中 include 的路径
```
执行 cmake 编译耗时比较久，如果输出如下则没有问题，表明找到了 ldns 的库并完成了编译配置
```shell
[100%] Built target googletest
-- Looking for pthread.h
-- Looking for pthread.h - found
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE  
-- Found asio: /home/magicdian/magicdata/openvpn3_srv/build/mingw/x64/vcpkg_installed/x64-mingw-dynamic/include  
-- Found lz4: /home/magicdian/magicdata/openvpn3_srv/build/mingw/x64/vcpkg_installed/x64-mingw-dynamic/lib/liblz4.dll.a  
-- Found ldns: /usr/lib/libldns.dll.a  
-- Found OpenSSL: /home/magicdian/magicdata/openvpn3_srv/build/mingw/x64/vcpkg_installed/x64-mingw-dynamic/lib/libcrypto.dll.a (found version "3.3.1")  
-- Found xxHash: /home/magicdian/magicdata/openvpn3_srv/build/mingw/x64/vcpkg_installed/x64-mingw-dynamic/include  
-- Could NOT find LZO (missing: LZO_LIBRARY LZO_INCLUDE_DIR) 
lzo not found, skipping lzo compression tests
-- Configuring done
-- Generating done
-- Build files have been written to: /home/magicdian/magicdata/openvpn3_srv/build/mingw/x64
```

现在开始正式编译 `cmake --build --preset mingw-x64-release --target ovpncli`

编译完成后会生成 build 目录，在 cli 工具路径在 `build/mingw/x64/test/ovpncli/ovpncli.exe`

## 使用

关于 OVPN 配置文件，请参考主说明页面的 [使用 OpenVPN 3 SRV](README.md#使用-openvpn-3-srv)

### 环境依赖安装
**环境依赖**
- TUN TAP 驱动
- MINGW-W64 环境动态库
- ldns 动态库，可以到 https://nlnetlabs.nl/~wouter/ldns-1.8.3_20240130.zip 或者 release 页面下载

Windows 平台使用需要安装 tap 网卡驱动，我使用的版本是这个 https://build.openvpn.net/downloads/releases/tap-windows-9.9.2_3.exe

安装 msys2，然后在 msys2 环境内执行以下命令安装工具链
```shell
pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-lz4
```
默认安装的路径是这个 `C:\msys64\mingw64\bin` ，将这个路径添加到系统环境变量

一同需要添加环境变量的是 ldns 库文件夹，解压后的根目录地址添加到系统环境变量


安装好后，打开一个**管理员权限**终端或者 PowerShell，并进入 build/mingw/x64/test/ovpncli/ 目录

>如果不使用管理员权限，连接后 ovpncli 就没办法更新 tap 网卡的ip等信息
### PowerShell 或 命令提示符
```shell
cd build/mingw/x64/test/ovpncli/
.\ovpncli.exe --no-dco -Q -u 你的用户名 -p 密码 -z 私钥密码 配置文件.ovpn
```