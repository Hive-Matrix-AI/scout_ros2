# SCOUT MINI ROS 2 Driver

[![Build](https://github.com/Hive-Matrix-AI/scout_ros2/actions/workflows/ros-ci.yml/badge.svg?branch=humble)](https://github.com/Hive-Matrix-AI/scout_ros2/actions/workflows/ros-ci.yml)
[![Platform](https://img.shields.io/badge/platform-Ubuntu%2022.04%20%7C%2024.04-E95420?logo=ubuntu&logoColor=white)](#requirements)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE)

ROS 2 packages for operating **SCOUT MINI** and **SCOUT MINI OMNI** over
SocketCAN. The driver publishes the robot's motion, battery, actuator, light,
remote-control, odometry, and TF state, and accepts standard ROS velocity
commands.

This driver uses
[`agilex_ugv_sdk`](https://github.com/Hive-Matrix-AI/agilex_ugv_sdk), a
general-purpose SDK for new AgileX mobile robot models. The SDK currently
implements SCOUT MINI and SCOUT MINI OMNI, which this ROS 2 driver exposes.

## Supported configurations

| Configuration | Support |
| --- | --- |
| SCOUT MINI, skid-steer | Supported |
| SCOUT MINI OMNI | Supported with `omni:=true` |
| ROS 2 Humble | Ubuntu 22.04 (Jammy) |
| ROS 2 Jazzy | Ubuntu 24.04 (Noble) |
| Transport | SocketCAN at 500 kbit/s |

## Packages

| Package | Purpose |
| --- | --- |
| `scout_base` | SocketCAN driver, command watchdog, state publishers, and auxiliary validation TUI |
| `scout_msgs` | Robot status, actuator, RC and light-control interfaces |
| `scout_description` | URDF, meshes and robot-description launch file |

## Requirements

- Ubuntu 22.04 with ROS 2 Humble, or Ubuntu 24.04 with ROS 2 Jazzy
- Colcon and rosdep (`python3-colcon-common-extensions`, `python3-rosdep`)
- A CAN adapter exposed as a SocketCAN interface (normally `can0`)
- SCOUT MINI or SCOUT MINI OMNI
- The robot's emergency stop within reach during motion tests

## Install

Source the ROS distribution installed on your system:

```bash
source /opt/ros/jazzy/setup.bash
```

On Ubuntu 22.04, use `source /opt/ros/humble/setup.bash` instead. Both
distributions use the same packages and launch commands. Use separate workspaces
when building for different ROS distributions.

Initialize rosdep with `sudo rosdep init` if it has not already been initialized,
then run `rosdep update`.

The repository includes the SDK as a Git submodule:

```bash
mkdir -p ~/scout_ws/src
cd ~/scout_ws/src
git clone --recurse-submodules https://github.com/Hive-Matrix-AI/scout_ros2.git
cd ~/scout_ws
rosdep install --from-paths src --ignore-src -r -y --rosdistro "$ROS_DISTRO"
colcon build --symlink-install
source install/setup.bash
```

If the repository was cloned without submodules, initialize them before the
build:

```bash
git submodule update --init --recursive
```

The SDK may instead be checked out as a sibling package under the same `src`
directory. Use only one SDK checkout in a workspace to avoid duplicate package
names. `scout_base` consumes its installed CMake target in either layout.

## Connect the robot

The SCOUT MINI CAN bus runs at **500 kbit/s**. Replace `can0` if your adapter has
a different interface name.

```bash
sudo ip link set can0 down 2>/dev/null || true
sudo ip link set can0 type can bitrate 500000 restart-ms 100
sudo ip link set can0 up
ip -details -statistics link show can0
```

Power on the base, release its physical emergency stop, then start the driver:

```bash
ros2 launch scout_base scout_mini.launch.py
```

For SCOUT MINI OMNI:

```bash
ros2 launch scout_base scout_mini.launch.py omni:=true
```

The node switches the base to CAN commanded mode after connecting. If velocity
commands stop arriving, the built-in watchdog sends zero velocity after 0.5 s.
Shutdown sends several zero-speed frames; it does not force the base into
standby mode.

The bundled `scout_description` is a SCOUT V2 model with fixed wheel joints;
its geometry is not calibrated for SCOUT MINI or OMNI.

## Test with the terminal dashboard

Start the driver in one terminal and the test console in another:

```bash
source ~/scout_ws/install/setup.bash
ros2 run scout_base scout_test_tui
```

The dashboard shows CAN interface state, topic rates and freshness, command
subscriber count, battery voltage, error flags, motor telemetry, odometry,
lights, and remote-control inputs.

It starts in **LOCKED** monitor-only mode. Put the robot on blocks for the first
test, keep the emergency stop within reach, and make sure nobody is inside the
vehicle's motion envelope.

| Key | Action |
| --- | --- |
| `E` (uppercase) | Arm or lock motion output |
| `W` / `S` | Short forward / reverse deadman pulse |
| `A` / `D` | Short left / right turn pulse |
| `J` / `L` | Strafe left / right when started with `--omni` |
| `Space` or `Esc` | Immediate stop and lock |
| `1` / `2` | Toggle front / rear light |
| `Q` | Send zero speed and quit |

Drive keys send 0.25 s pulses and must be tapped or held to continue moving.
This is independent of the driver's own watchdog. Default test speeds are
0.15 m/s and 0.35 rad/s; lower them for a first floor test:

```bash
ros2 run scout_base scout_test_tui -- \
  --linear-speed 0.08 --angular-speed 0.20
```

Namespaced deployments can remap the console in the normal ROS 2 way:

```bash
ros2 run scout_base scout_test_tui --ros-args \
  -r scout_status:=/robot/scout_status \
  -r odom:=/robot/odom \
  -r rc_status:=/robot/rc_status \
  -r cmd_vel:=/robot/cmd_vel
```

For automated CAN feedback and low-speed motion checks, see the
[`scout_base` diagnostic node](scout_base/README.md#can-diagnostics).

## ROS interface

All names are relative and therefore support namespaces and remapping.

### Subscribed topics

| Topic | Type | Description |
| --- | --- | --- |
| `cmd_vel` | `geometry_msgs/msg/Twist` | Longitudinal and angular velocity; `linear.y` is used only by OMNI |
| `light_control` | `scout_msgs/msg/ScoutLightCmd` | Front and rear light mode and brightness |

### Published topics

| Topic | Type | Description |
| --- | --- | --- |
| `scout_status` | `scout_msgs/msg/ScoutStatus` | Motion, battery, errors, lights and four actuator states |
| `odom` | `nav_msgs/msg/Odometry` | Wheel-integrated planar odometry |
| `rc_status` | `scout_msgs/msg/ScoutRCState` | Remote switches, sticks and knob |
| `/tf` | `tf2_msgs/msg/TFMessage` | `odom` to `base_link` transform |

The actuator array order is front-right, front-left, rear-right, rear-left.
Odometry is integrated from base feedback and is not a globally corrected pose.

## Launch options

```bash
ros2 launch scout_base scout_mini.launch.py \
  port_name:=can0 \
  omni:=false \
  odom_frame:=odom \
  base_frame:=base_link \
  odom_topic_name:=odom \
  control_rate:=50 \
  cmd_vel_timeout:=0.5
```

| Argument | Default | Description |
| --- | --- | --- |
| `port_name` | `can0` | SocketCAN interface |
| `omni` | `false` | Enable lateral velocity for SCOUT MINI OMNI |
| `odom_frame` | `odom` | Parent frame for odometry and TF |
| `base_frame` | `base_link` | Robot body frame |
| `odom_topic_name` | `odom` | Odometry topic name |
| `control_rate` | `50` | Hardware command and state loop rate in Hz |
| `cmd_vel_timeout` | `0.5` | Maximum command age before zero velocity, in seconds |
| `use_sim_time` | `false` | Use the ROS simulation clock |

## Troubleshooting

**The driver reports `failed to open CAN transport`.** Check that the adapter
exists and is up with `ip link show can0`. Confirm the bitrate is 500000 and
that your user has permission to open the interface.

**The TUI shows `NO DATA`.** Confirm the driver is running and that both
terminals use the same `ROS_DOMAIN_ID`. Run `ros2 topic hz /scout_status` to
separate a ROS discovery problem from a terminal-display problem.

**The TUI is online but the robot does not move.** The console must show
`Motion: ARMED` and a non-zero `cmd_vel subscribers` count. Also check the
physical emergency stop, robot power, CAN control mode, and `scout_status`
error flags.

**The robot moves briefly and stops.** Both the TUI deadman pulse and the driver
watchdog are working as designed. Hold or repeatedly tap the drive key, and do
not increase either timeout as a substitute for fixing slow or missing command
delivery.

## Tests

Build and run the package tests from the workspace root:

```bash
colcon build --symlink-install
colcon test --packages-select scout_msgs scout_base scout_description
colcon test-result --verbose
```

## License

The driver and SDK are licensed under Apache-2.0. The wheel Xacro files retain
their BSD-3-Clause notices. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

## Contributing

Bug reports and pull requests are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md)
for the test workflow and information to include when reporting a problem.
