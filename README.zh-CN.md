# SCOUT ROS 2

[English](README.md) | [简体中文](README.zh-CN.md)

[![构建](https://github.com/Hive-Matrix-AI/scout_ros2/actions/workflows/ros-ci.yml/badge.svg?branch=humble)](https://github.com/Hive-Matrix-AI/scout_ros2/actions/workflows/ros-ci.yml)
[![ROS 2](https://img.shields.io/badge/ROS%202-Humble%20%7C%20Jazzy-22314E?logo=ros&logoColor=white)](#支持环境)
[![Ubuntu](https://img.shields.io/badge/Ubuntu-22.04%20%7C%2024.04-E95420?logo=ubuntu&logoColor=white)](#支持环境)
[![SocketCAN](https://img.shields.io/badge/transport-SocketCAN-3C8D6E)](#连接底盘)
[![许可证](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE)

**将 SCOUT MINI 和 SCOUT MINI OMNI 接入你的 ROS 2 应用。**

通过标准速度消息控制底盘，获取机器人状态，并使用终端面板检查连接与运行情况。
支持通过 SocketCAN 接入滑移转向和全向底盘。

[快速开始](#快速开始) · [支持环境](#支持环境) · [终端面板](#终端面板) ·
[驱动参考](scout_base/README.zh-CN.md) · [更新记录（英文）](CHANGELOG.md)

## 功能亮点

- 通过 SocketCAN 控制 SCOUT MINI 和 SCOUT MINI OMNI。
- 接收 `Twist` 或 `TwistStamped` 速度指令。
- 发布里程计、TF、电池、电机、灯光和遥控器状态。
- 指令超时自动发送零速，支持前后灯控制。
- 提供终端状态面板、CAN 诊断和低速运动测试工具。

## 支持环境

| 车型 | 启动选项 |
| --- | --- |
| SCOUT MINI | 默认，滑移转向 |
| SCOUT MINI OMNI | `omni:=true` |

| ROS 2 | Ubuntu |
| --- | --- |
| Humble | 22.04 LTS（Jammy） |
| Jazzy | 24.04 LTS（Noble） |

两个 ROS 2 版本均使用 `humble` 分支。硬件连接使用 **500 kbit/s SocketCAN**。

## 快速开始

### 编译工作区

先安装对应发行版的 ROS 2 **ROS Base** 或 **Desktop**，再通过 APT 安装构建工具和 Xacro。
以下示例使用 Jazzy；Ubuntu 22.04 用户将第一行改为 `source /opt/ros/humble/setup.bash`。
不同 ROS 2 发行版应使用独立工作区。

```bash
source /opt/ros/jazzy/setup.bash
sudo apt update
sudo apt install -y build-essential cmake python3-colcon-common-extensions \
  "ros-${ROS_DISTRO}-xacro"
mkdir -p ~/scout_ws/src
cd ~/scout_ws/src
git clone --branch humble --recurse-submodules https://github.com/Hive-Matrix-AI/scout_ros2.git
cd ~/scout_ws
colcon build --symlink-install
source install/setup.bash
```

SDK 以 Git 子模块随仓库提供。已有检出目录的用户应先在仓库根目录运行
`git submodule update --init --recursive`，再进行编译。
其他安装方式见 [SDK 依赖](third_party/README.zh-CN.md)。

### 连接底盘

连接 SocketCAN 适配器；如接口名不是 `can0`，请替换为实际名称：

```bash
sudo ip link set can0 down
sudo ip link set can0 type can bitrate 500000 restart-ms 100
sudo ip link set can0 up
ip -details -statistics link show can0
```

首次运动测试前，将底盘可靠架起、使车轮离地，清空可能触及的区域，并确保物理急停触手可及。

### 启动驱动

底盘上电，确认可以安全运行后释放急停：

```bash
ros2 launch scout_base scout_mini.launch.py
```

SCOUT MINI OMNI 使用：

```bash
ros2 launch scout_base scout_mini.launch.py omni:=true
```

如需在 `cmd_vel` 上接收 `TwistStamped`：

```bash
ros2 launch scout_base scout_mini.launch.py use_stamped_cmd_vel:=true
```

默认输入类型为 `Twist`。时间戳和坐标系要求见[速度指令](scout_base/README.zh-CN.md#速度指令)。

驱动连接后会启用 CAN 指令控制模式。超过 **0.5 秒**未收到有效速度指令时，
看门狗将发送零速；正常退出时也会发送零速帧。这些软件保护不能替代物理急停。

## 终端面板

保持驱动运行，在另一个终端执行：

```bash
source ~/scout_ws/install/setup.bash
ros2 run scout_base scout_test_tui
```

面板展示 CAN 接口状态、话题接收情况、电池电压、电机数据、里程计、灯光和遥控器输入。
运动输出初始为**锁定**状态；按大写 `E` 解锁，按 `Space` 或 `Esc` 发送零速并重新锁定。

降低测试速度：

```bash
ros2 run scout_base scout_test_tui --linear-speed 0.08 --angular-speed 0.20
```

驱动使用 `TwistStamped` 时，为终端工具添加 `--stamped`。

按键、OMNI 横移和话题重映射见[终端面板说明](scout_base/README.zh-CN.md#终端面板)；
自动检查见 [CAN 诊断](scout_base/README.zh-CN.md#can-诊断)。

## 软件包与接口

| 软件包 | 用途 |
| --- | --- |
| [`scout_base`](scout_base/README.zh-CN.md) | 驱动、指令看门狗、终端面板和 CAN 诊断 |
| [`scout_msgs`](scout_msgs/msg) | 底盘、电机、遥控器状态及灯光控制消息 |
| [`scout_description`](scout_base/README.zh-CN.md#机器人模型) | SCOUT V2 URDF 和网格资源 |

驱动订阅 `cmd_vel`（启动时选择 `geometry_msgs/msg/Twist` 或 `TwistStamped`）和
`light_control`，发布 `scout_status`、`rc_status`、`odom` 以及 `odom` → `base_link` TF。
OMNI 还使用速度指令中的 `linear.y` 进行横向移动。

随附模型是使用固定轮关节的 SCOUT V2。SCOUT MINI 或 OMNI 的碰撞检测应使用对应车型的几何模型。

CAN 通信由面向 AgileX 新车型的通用 C++ 库
[`agilex_ugv_sdk`](https://github.com/Hive-Matrix-AI/agilex_ugv_sdk) 提供；
SCOUT 车型的 ROS 集成在本仓库维护。

## 文档

- [启动参数](scout_base/README.zh-CN.md#启动参数)
- [ROS 话题与消息约定](scout_base/README.zh-CN.md#ros-接口)
- [常见问题](scout_base/README.zh-CN.md#常见问题)
- [SCOUT MINI CAN 协议](third_party/agilex_ugv_sdk/docs/reference/models/scout/scout_mini_can.md)
- [参与贡献与运行测试](CONTRIBUTING.zh-CN.md)

欢迎通过 [GitHub Issues](https://github.com/Hive-Matrix-AI/scout_ros2/issues)
反馈问题，或提交 [Pull Request](https://github.com/Hive-Matrix-AI/scout_ros2/pulls)。

## 许可证

驱动和 SDK 使用 **Apache-2.0** 许可证。车轮 Xacro 文件保留其 **BSD-3-Clause** 声明。
详见 [LICENSE](LICENSE) 和 [NOTICE](NOTICE)。
