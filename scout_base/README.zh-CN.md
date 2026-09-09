# scout_base

[English](README.md) | [简体中文](README.zh-CN.md)

`scout_base` 是 SCOUT MINI 和 SCOUT MINI OMNI 的 ROS 2 硬件驱动。
通过 SocketCAN 连接底盘，发布机器人状态和根据底盘速度反馈积分的里程计，
广播 `odom` → `base_link` TF，并接收 `geometry_msgs/msg/Twist` 或
`geometry_msgs/msg/TwistStamped` 速度指令。

支持 Ubuntu 22.04 / Humble 和 Ubuntu 24.04 / Jazzy。
安装与 CAN 配置见[项目快速开始](../README.zh-CN.md#快速开始)。

## 启动驱动

```bash
ros2 launch scout_base scout_mini.launch.py
```

SCOUT MINI OMNI 添加 `omni:=true`。驱动启动后启用 CAN 指令控制模式，
`cmd_vel` 超时时发送零速。正常退出会发送多帧零速指令，但不会切换到待机模式。

## 启动参数

```bash
ros2 launch scout_base scout_mini.launch.py \
  port_name:=can0 \
  omni:=false \
  odom_frame:=odom \
  base_frame:=base_link \
  odom_topic_name:=odom \
  control_rate:=50 \
  cmd_vel_timeout:=0.5 \
  use_stamped_cmd_vel:=false
```

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `port_name` | `can0` | SocketCAN 接口 |
| `omni` | `false` | 启用 SCOUT MINI OMNI 横向速度 |
| `odom_frame` | `odom` | 里程计和 TF 的父坐标系 |
| `base_frame` | `base_link` | 底盘坐标系 |
| `odom_topic_name` | `odom` | 里程计话题名称 |
| `control_rate` | `50` | 硬件指令发送及状态发布频率，Hz |
| `cmd_vel_timeout` | `0.5` | 速度指令有效期，秒；超时发送零速 |
| `use_stamped_cmd_vel` | `false` | 启动时将 `cmd_vel` 类型从 `Twist` 切换为 `TwistStamped` |
| `use_sim_time` | `false` | 使用 ROS 仿真时钟 |

## ROS 接口

驱动话题使用相对名称，支持命名空间和重映射。TF 发布到 `/tf`；
多机器人部署时应设置独立的坐标系名称，并按需重映射 TF。

### 订阅话题

| 话题 | 类型 | 说明 |
| --- | --- | --- |
| `cmd_vel` | `geometry_msgs/msg/Twist` 或 `geometry_msgs/msg/TwistStamped` | 由 `use_stamped_cmd_vel` 选择；仅 OMNI 使用 `linear.y` |
| `light_control` | `scout_msgs/msg/ScoutLightCmd` | 前后灯模式和亮度 |

### 发布话题

| 话题 | 类型 | 说明 |
| --- | --- | --- |
| `scout_status` | `scout_msgs/msg/ScoutStatus` | 运动、电池、故障、灯光及四个电机的状态 |
| `odom` | `nav_msgs/msg/Odometry` | 根据底盘反馈积分的平面里程计 |
| `rc_status` | `scout_msgs/msg/ScoutRCState` | 遥控器开关、摇杆和旋钮 |
| `/tf` | `tf2_msgs/msg/TFMessage` | `odom` → `base_link` 坐标变换 |

电机数组顺序为：右前、左前、右后、左后。
里程计来自底盘反馈积分，不是经过全局定位校正的位姿。

### 速度指令

`cmd_vel` 默认接收 `Twist`。切换为同一话题上的 `TwistStamped`：

```bash
ros2 launch scout_base scout_mini.launch.py use_stamped_cmd_vel:=true
```

驱动只订阅选定的消息类型。速度发布端必须使用相同类型，同一时刻仅启用一个指令源。

- 两种消息都使用 `linear.x` 表示前后速度、`angular.z` 表示偏航角速度。
  OMNI 还使用 `linear.y`，滑移转向款忽略该字段。
- `TwistStamped.header.frame_id` 必须为空或与 `base_frame` 一致。
  所有指令均按底盘坐标系解释，不执行 TF 转换。
- 非零 `header.stamp` 必须与驱动使用同一 ROS 时钟，且不超过 `cmd_vel_timeout`。
  拒绝未来时间戳和格式无效的时间戳；零时间戳使用接收时刻。
- 带时间戳指令从源时间戳计算有效期，普通指令从接收时刻计算。
  看门狗还使用单调时钟：即使 ROS 时间暂停，超过 `cmd_vel_timeout` 未收到有效指令也会发送零速。
  无效指令不会刷新看门狗。

## 终端面板

```bash
ros2 run scout_base scout_test_tui
```

面板默认监视 CAN 和 ROS 数据，只有按大写 `E` 才解锁运动输出。
`Space`、`Esc` 和正常退出均发送零速。该工具用于调试与验收，不能替代物理急停或安全测试场地。

显示内容包括 CAN 接口状态、话题频率和接收时间、速度话题订阅者数量、电池电压、
故障位、电机数据、里程计、灯光和遥控器输入。

| 按键 | 操作 |
| --- | --- |
| `E`（大写） | 解锁或锁定运动输出 |
| `w` / `s` | 短时前进 / 后退 |
| `a` / `d` | 短时左转 / 右转 |
| `j` / `l` | 左移 / 右移，需使用 `--omni` |
| `Space` 或 `Esc` | 发送零速并锁定 |
| `1` / `2` | 切换前灯 / 后灯 |
| `q` | 发送零速并退出 |

每次运动按键产生 0.25 秒指令脉冲，持续运动需要重复按键或保持按键。
该保护独立于驱动看门狗。默认前后和横移速度为 0.15 m/s，转动速度为 0.35 rad/s。
使用 OMNI 并降低测试速度：

```bash
ros2 run scout_base scout_test_tui --omni \
  --linear-speed 0.08 --lateral-speed 0.08 --angular-speed 0.20
```

驱动使用 `use_stamped_cmd_vel:=true` 时：

```bash
ros2 run scout_base scout_test_tui --stamped
```

面板使用自身 ROS 时钟为指令添加时间戳。如果驱动自定义了 `base_frame`，
请通过 `--frame-id` 传入相同名称。

命名空间部署可使用 ROS 2 话题重映射：

```bash
ros2 run scout_base scout_test_tui --ros-args \
  -r scout_status:=/robot/scout_status \
  -r odom:=/robot/odom \
  -r rc_status:=/robot/rc_status \
  -r cmd_vel:=/robot/cmd_vel \
  -r light_control:=/robot/light_control
```

## CAN 诊断

```bash
ros2 run scout_base scout_mini_smoke --ros-args -p can_interface:=can0
```

默认读取状态并请求主控和驱动器版本。退出时会发送零速，因此请勿与其他正在控制运动的程序同时运行。

如需运行低速运动序列，请先可靠架起底盘、使车轮离地，清空运动范围，并准备好物理急停：

```bash
ros2 run scout_base scout_mini_smoke --ros-args \
  -p can_interface:=can0 -p run_motion_test:=true
```

该序列启用 CAN 指令控制模式，依次前进、停止、旋转、停止。
退出时发送零速，但不切换到待机模式。

## 机器人模型

`scout_description` 提供使用固定轮关节的 SCOUT V2 模型，
不是经过标定的 SCOUT MINI 或 OMNI 模型。需要碰撞几何或车轮动画的应用应提供对应车型模型。

## 常见问题

**驱动报错 `failed to open CAN transport`。** 使用 `ip link show can0` 检查适配器是否存在且已启用。
确认波特率为 500000，并检查当前用户的接口访问权限。

**面板显示 `NO DATA`。** 确认驱动正在运行，两个终端使用相同 ROS 2 发行版和 `ROS_DOMAIN_ID`。
使用 `ros2 topic hz /scout_status` 判断是 ROS 发现问题还是终端显示问题。

**面板在线但底盘不动。** 面板必须显示 `Motion: ARMED`，且 `cmd_vel subscribers` 大于零。
同时检查急停、电源、CAN 控制模式和 `scout_status` 故障码。
`--stamped` 应与驱动的 `use_stamped_cmd_vel` 设置一致；带时间戳指令还需使用相同的底盘坐标系和 ROS 时钟。

话题的新鲜度表示 ROS 消息接收情况，不代表 CAN 反馈仍在更新。
驱动会重复发布最近保存的状态，因此面板在线不能单独证明底盘仍在发送 CAN 帧。
检查连接时，可通过 `ip -details -statistics link show can0` 观察接收计数是否增长。

**底盘短暂运动后停止。** 面板按键脉冲和驱动看门狗均会限制指令持续时间。
保持或重复按下运动键；不要通过增大超时来掩盖指令发送过慢或中断的问题。
