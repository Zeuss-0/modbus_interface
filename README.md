# modbus_interface

ROS 1（catkin）节点：用 **Modbus TCP** 读写远端数字量 IO，并通过 `yikun_msgs/ModbusCmd` 提供给其他节点。

ROS 1 (catkin) node that reads and writes remote digital I/O over **Modbus TCP** and exposes those operations as a `yikun_msgs/ModbusCmd` service.

本仓库第一次提交的说明是「光源驱动」。源码本身只做通用的线圈 / 离散输入访问，**没有**记录每一路 IO 对应哪一盏灯或哪一个设备通道。下文凡是从代码和 launch 推不出来的地方，都标成未知。

The first commit message describes this as a light-source driver. The source itself only implements generic coil and discrete-input access, and it does not record which I/O point maps to which lamp or device channel. Anything that cannot be inferred from the code or the launch file is marked unknown.

## 依赖 / Dependencies

| 包 / Package | 作用 / Role |
| --- | --- |
| [`yikun_msgs`](https://github.com/Zeuss-0/yikun_msgs) | 必需。服务类型 `yikun_msgs/ModbusCmd` 定义在该仓库的 `yikun_msgs/srv/ModbusCmd.srv`。本仓库不包含这个消息包。Required. This repo does not vendor the service definition. |
| `roscpp` | ROS 1 C++ 客户端库。ROS 1 C++ client library. |
| `std_msgs` | `/obsignal` 使用 `std_msgs/String`。源码直接包含了它；原先的 `package.xml` 没有单独声明，格式 2 清单里已补上。Used by `/obsignal`. It was not declared in the old manifest. |
| `catkin` | 构建工具。Build tool. |
| Boost | 头文件使用 `boost::mutex` 和 `boost::shared_ptr`。`CMakeLists.txt` 链接 `${Boost_LIBRARIES}`，但没有 `find_package(Boost)`。实际编译靠 `roscpp` 把 Boost 带进来。Linked via `${Boost_LIBRARIES}` without its own `find_package`. Builds rely on Boost arriving with `roscpp`. |

节点**不**依赖 libmodbus。Modbus TCP（MBAP）帧是在 `src/modbus_interface.cpp` 里手写的。

The node does not use libmodbus. Modbus TCP (MBAP) frames are hardcoded in `src/modbus_interface.cpp`.

**没有串口，也没有 Modbus RTU。** 连接方式只有 `AF_INET` / `SOCK_STREAM`。波特率、校验位、串口站号都不存在。

**There is no serial port and no Modbus RTU.** The only transport is `AF_INET` / `SOCK_STREAM`. Baud rate, parity, and a serial station id are not configurable because that path does not exist.

## 编译与启动 / Build and launch

把本包和 `yikun_msgs` 放进同一个 catkin 工作空间：

Put this package and `yikun_msgs` in the same catkin workspace:

```bash
cd ~/catkin_ws/src
git clone https://github.com/Zeuss-0/yikun_msgs.git
git clone https://github.com/Zeuss-0/modbus_interface.git
cd ~/catkin_ws
catkin_make
source devel/setup.bash
roslaunch modbus_interface modbus_node.launch
```

`CMakeLists.txt` 要求 CMake 2.8.3 和 C++11，对应 ROS 1（Kinetic 到 Noetic 这一代 catkin）。这里**没有**对着某一版 ROS 实际编译过，以你的工作空间为准。也可以用 `catkin build`，效果取决于你装的是 catkin_make 还是 catkin_tools。

`CMakeLists.txt` asks for CMake 2.8.3 and C++11, which matches ROS 1 catkin from Kinetic through Noetic. This tree was not built against a specific distro here. `catkin build` works the same way if you use catkin_tools instead of catkin_make.

可执行文件是 `modbus_interface_node`。`ros::init` 里的默认节点名是 `modbus`。`launch/modbus_node.launch` 把节点名设成 `modbus_node`，并且**不传任何参数**，所以全部使用代码里的默认值。

The executable is `modbus_interface_node`. `ros::init` names the node `modbus` unless a launch file or `__name` overrides it. `launch/modbus_node.launch` renames it to `modbus_node` and sets no parameters, so every default below applies.

直接运行：

```bash
rosrun modbus_interface modbus_interface_node
```

这样节点名是 `modbus`，私有参数挂在 `/modbus/` 下面。服务名仍然由 `group_name` 决定，默认还是 `/Modbus/Command`。

With `rosrun` and no rename, the node name is `modbus` and private parameters live under `/modbus/`. The service name still comes from `group_name`, so it is still `/Modbus/Command` by default.

## 参数 / Parameters

都是节点私有参数（`~`）。

All of these are private parameters (`~`).

| 参数 / Parameter | 类型 / Type | 默认值 / Default | 含义 / Meaning |
| --- | --- | --- | --- |
| `~ip_addr` | `string` | `192.168.1.71` | Modbus TCP 从站 IP。Slave IP. |
| `~port` | `string` | `502` | TCP 端口。代码按字符串保存，连接时 `atoi`。Stored as a string and parsed with `atoi`. |
| `~timesec` | `int` | `60` | 套接字发送 / 接收超时（`SO_SNDTIMEO`、`SO_RCVTIMEO`），单位秒。Socket send and receive timeout, in seconds. |
| `~group_name` | `string` | `Modbus` | 拼进服务名，见下一节。Inserted into the service name. |

`roslaunch modbus_interface modbus_node.launch` 时，上述名字展开为 `/modbus_node/ip_addr` 等。`rosrun` 且不改节点名时，展开为 `/modbus/ip_addr` 等。

Under the shipped launch file those resolve as `/modbus_node/ip_addr` and so on. Under plain `rosrun` they resolve as `/modbus/ip_addr` and so on.

覆盖示例（现有 launch 没有这段）：

Example override. The shipped launch file does not include this:

```xml
<launch>
  <node pkg="modbus_interface" name="modbus_node" type="modbus_interface_node" output="screen">
    <param name="ip_addr" value="192.168.1.71" />
    <param name="port" value="502" />
    <param name="timesec" value="60" />
    <param name="group_name" value="Modbus" />
  </node>
</launch>
```

单元标识符固定为 `0xFF`。寄存器地址也写死在源码里。这两项**都不是参数**。

The unit identifier is fixed at `0xFF`. Register addresses are hardcoded. Neither is a parameter.

## ROS 接口 / ROS API

### 服务 / Service

名称 `/<group_name>/Command`，类型 `yikun_msgs/ModbusCmd`。默认是 `/Modbus/Command`。

Name: `/<group_name>/Command`, type `yikun_msgs/ModbusCmd`. Default: `/Modbus/Command`.

`yikun_msgs/srv/ModbusCmd.srv` 的内容是：

```
int8 cmd            # 1:读取DI  2:设置DO  3:等待DI
int8 number         # IO口编号从0开始
int8 value          # IO值 0/1 (cmd为2和3时有效)
float64 duration    #预留
---
int8 result
string description
```

本节点 `command_callback` 的实际分支：

What this node's `command_callback` actually does:

| `cmd` | 行为 / Behavior | 用到的字段 / Fields | `result` |
| --- | --- | --- | --- |
| 1 | 读离散输入，功能码 `0x02`。Read discrete inputs, function `0x02`. | `number` | 该位的 0 或 1。未连接或超时为 `-1`。Bit value, or `-1` if disconnected or timed out. |
| 2 | 写单个线圈，功能码 `0x05`。Write one coil, function `0x05`. | `number`，`value`（只接受 0 或 1） | 帧已写出为 `1`，失败为 `-1`。**不检查应答内容是否匹配。** `1` after the frame is sent, `-1` on failure. The response bytes are not checked. |
| 3 | 在 `duration` 秒内每 0.1 秒读一次离散输入。Poll discrete inputs every 0.1 s for `duration` seconds. | `number`，`value`，`duration` | 时限内读到与 `value` 相等时返回 **`-1`**；超时返回 **`1`**。Returns **`-1`** when the input matches `value`, and **`1`** on timeout. |
| 4 | 读线圈，功能码 `0x01`。Read coils, function `0x01`. | `number` | 该位的 0 或 1，失败为 `-1`。`.srv` 注释没有写 cmd 4，但节点实现了这一支。Bit value, or `-1` on failure. The `.srv` comment does not mention cmd 4; the node implements it anyway. |
| 其他 / other | 不访问总线。No bus access. | | `-1` |

`description` 节点从不赋值，调用方会看到空字符串。

`description` is never filled in. Callers receive an empty string.

`.srv` 把 `duration` 标成「预留」，但 cmd 3 把它当作等待秒数。服务调用里不填该字段时，ROS 默认是 `0.0`：循环条件一次都不成立，直接返回 `1`。

The `.srv` comment marks `duration` as reserved, but cmd 3 uses it as a timeout in seconds. If the caller omits it, ROS sends `0.0`, the wait loop never runs, and the result is `1`.

```bash
rosservice call /Modbus/Command "cmd: 1
number: 0
value: 0
duration: 0.0"
```

### 话题 / Topic

| 话题 / Topic | 类型 / Type | 说明 / Notes |
| --- | --- | --- |
| `/obsignal` | `std_msgs/String` | 队列长度 10。节点会广告这个发布者，但主循环**从不调用** `publishSignal()`，所以正常运行时没有消息。Advertised with queue 10. The main loop never calls `publishSignal()`, so normal operation publishes nothing. |

内部字符串初始为 `"0"`。只有写入**线圈编号 1**（协议地址 101）时才会修改它：`value == 0` 记成 `"0"`，`value == 1` 记成 `"1"`。不调用 `publishSignal()` 的话，这个变化不会出现在话题上。

The internal string starts as `"0"`. It changes only when coil index 1 (protocol address 101) is written: `"0"` for value 0, `"1"` for value 1. That update is not published unless something calls `publishSignal()`.

## 连接 / Connection

主循环 10 Hz。尚未连接时调用 `modbusConnect()`：成功打一条 `ROS_INFO`，失败打 `ROS_ERROR`，下一拍再试。

The main loop runs at 10 Hz. While disconnected it calls `modbusConnect()`, logs `ROS_INFO` on success or `ROS_ERROR` on failure, and retries on the next tick.

- `socket()` 或 `setsockopt()` 失败时，进程直接 `exit(1)`。`socket()` or `setsockopt()` failure calls `exit(1)`.
- `connect()` 失败只返回 false，进程继续重试。A failed `connect()` returns false and the process keeps retrying.
- `isconnected_` 一旦为真，后面的收发失败**不会**把连接标成断开，也没有自动重连。After `isconnected_` is true, later I/O errors do not clear it and do not reconnect.
- 析构时如果仍标记为已连接，会 `close` 套接字。The destructor closes the socket if it is still marked connected.

## Modbus 寄存器 / Register map

报文是 Modbus TCP。MBAP 事务号固定 `0x0001`，协议号固定 `0x0000`，单元标识符固定 `0xFF`。表中的地址是线上的协议地址（从 0 起算），不是 00001 / 10001 那种从 1 起算的 PLC 编号。

Frames are Modbus TCP. The MBAP transaction id is fixed at `0x0001`, the protocol id at `0x0000`, and the unit id at `0xFF`. Addresses below are the zero-based protocol addresses on the wire, not 1-based PLC numbers such as 00001 or 10001.

| 操作 / Operation | 功能码 / Function | 起始地址 / Start | 数量 / Count | 入口 / Entry point |
| --- | --- | --- | --- | --- |
| 读离散输入 DI / Read discrete inputs | `0x02` | `0x00C8` (200) | 4 | 服务 cmd 1、cmd 3 / service cmd 1 and 3 |
| 读线圈 DO / Read coils | `0x01` | `0x0064` (100) | 4 | 服务 cmd 4 / service cmd 4 |
| 写单个线圈 / Write single coil | `0x05` | 见下表 / see below | 1 | 服务 cmd 2 / service cmd 2 |
| 写多个线圈 / Write multiple coils | `0x0F` | `0x0064` (100) | 4 | 仅 C++ `setDigitalOutputs`。**服务调用不到。** C++ only. Not reachable from the service. |

写单个线圈时，`number` 只改地址低字节：

For a single-coil write, `number` only changes the low address byte:

| `number` | 线圈地址 / Coil address | 附加行为 / Side effect |
| --- | --- | --- |
| 0 | 100 (`0x64`) | 无 / none |
| 1 | 101 (`0x65`) | 同时改内部 `/obsignal` 字符串 / also updates the internal `/obsignal` string |
| 2 | 102 (`0x66`) | 无 / none |
| 3 | 103 (`0x67`) | 无 / none |
| 其他 / other | **仍是 100 / stays 100** | `switch` 没有 default，地址保持模板里的 `0x64`。There is no `default`, so the template address `0x64` is left unchanged. |

线圈取值：`value == 0` 发送 `0x0000`，`value == 1` 发送 `0xFF00`（功能码 05 的关 / 开）。其他 `value` 不发送，返回 `-1`。

Coil data: value 0 sends `0x0000`, value 1 sends `0xFF00` (function 05 off / on). Any other value is not sent and returns `-1`.

读操作取应答第 10 个字节（下标 9，即功能码 01 / 02 的数据字节），放进 `std::bitset<4>`，返回第 `number` 位。`number` 没有范围检查。头文件把 `DIGITAL_INPUTS` 和 `DIGITAL_OUTPUTS` 定义成 7，**这两宏没有被使用**，也和报文里的 4 个点不一致。

Reads take byte index 9 of the response (the data byte of function 01 / 02), interpret it as a 4-bit `std::bitset`, and return bit `number`. `number` is not bounds-checked. The header defines `DIGITAL_INPUTS` and `DIGITAL_OUTPUTS` as 7; those macros are unused and do not match the 4 points in the frames.

写多个线圈时，数据字节是参数 `outputs` 的低 8 位，起始线圈 100，数量 4，字节数为 1。

The multiple-coil write places the low 8 bits of `outputs` in the data byte, starting at coil 100, count 4, byte count 1.

这些地址和数量都是从源码里的帧常量读出来的，**没有**设备手册可以对照。是否与现场从站一致，需要用实机确认。

These addresses and counts are read off the frame constants in the source. No device manual is in the repo. Whether they match the slave on the bench has to be checked on hardware.

## 使用时需要知道的行为 / Behavior that affects callers

这些是当前源码的行为，本次没有改逻辑。

This is how the current source behaves. This change does not alter it.

- cmd 3 匹配成功返回 `-1`，超时返回 `1`，和「`1` 表示成功」的习惯相反。Cmd 3 returns `-1` on a match and `1` on timeout, which is the reverse of a "1 means success" convention.
- `number` 不在 0–3 时，写单个线圈仍然写地址 100。A single-coil write with `number` outside 0–3 still writes address 100.
- 连接成功之后，总线错误不会触发重连。Once connected, bus errors do not trigger a reconnect.
- `/obsignal` 在本节点的主循环里不会发出消息。`/obsignal` is not published by this node's main loop.
- 读请求之前有一次 `recv(..., flags = 1)`。在 Linux 上 flags 值 `1` 是 `MSG_OOB`，不是非阻塞标志。Reads issue a `recv` with `flags = 1` first. On Linux that value is `MSG_OOB`, not a non-blocking flag.
- 应答只打印到标准输出，不核对事务号、功能码或异常码。Responses are printed to stdout and are not checked for transaction id, function code, or an exception code.

## 未知项 / Unknowns

- 各 DI / DO 的电气含义，以及它们和光源通道的对应关系。提交说明写了光源，源码没有通道表。Electrical meaning of each DI/DO, and any mapping onto light-source channels. The commit message says light source; the source has no channel table.
- 默认 IP `192.168.1.71` 上的具体设备型号。The device model at the default IP `192.168.1.71`.
- 从站是否只接受单元号 `0xFF`，地址 100 和 200 是否与手册一致。Whether the slave only accepts unit id `0xFF`, and whether addresses 100 and 200 match its manual.
- 串口参数：没有 RTU 实现，所以无从记录。Serial settings: there is no RTU implementation to document.
- `yikun_msgs` 自己的 `package.xml` 许可证仍是 `TODO`。That package's own `package.xml` license is still `TODO`.
- 本仓库没有测试，也没有在硬件上跑过这里写的寄存器表。This repo has no tests, and the register table above was not checked on hardware.

## 许可与维护者 / License and maintainer

许可证是 Apache License 2.0，全文在仓库根目录的 [`LICENSE`](LICENSE)。`package.xml` 的 `<license>` 与此一致（SPDX 标识 `Apache-2.0`）。

Licensed under the Apache License 2.0. The full text is [`LICENSE`](LICENSE). `package.xml` uses the SPDX id `Apache-2.0`.

源文件头写的是 Copyright 2019 Shanghai Yikun Electrical Engineering Co., Ltd.，作者栏有 Jiayuan Sun，文件头注释里还有 wzy。这些版权声明留在源文件里，本次没有改它们。

Source headers say Copyright 2019 Shanghai Yikun Electrical Engineering Co., Ltd., name Jiayuan Sun as author, and the header comment also names wzy. Those notices stay in the source files.

维护者：Jiuxu Yang（GitHub [Zeuss-0](https://github.com/Zeuss-0)），邮箱 `jixuyang0715@gmail.com`。这个邮箱来自本仓库现有提交记录，用来替换原来的占位符 `sjy@todo.todo`。

Maintainer: Jiuxu Yang (GitHub [Zeuss-0](https://github.com/Zeuss-0)), `jixuyang0715@gmail.com`. That address is the author email already on this repo's commits. It replaces the placeholder `sjy@todo.todo`.
