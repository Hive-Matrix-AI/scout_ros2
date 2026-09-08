# scout_base

`scout_base` is the ROS 2 Humble hardware driver for SCOUT MINI and SCOUT MINI
OMNI. It connects to the base through SocketCAN, publishes robot state and
wheel-integrated odometry, broadcasts `odom` to `base_link`, and accepts
`geometry_msgs/msg/Twist` velocity commands.

For installation, CAN configuration, launch parameters, interface contracts,
and troubleshooting, see the [repository documentation](../README.md).

## Start the driver

```bash
ros2 launch scout_base scout_mini.launch.py
```

Use `omni:=true` for SCOUT MINI OMNI. The driver enables CAN commanded mode at
startup and applies a zero-speed watchdog when `cmd_vel` becomes stale.

## Auxiliary hardware validation

The package installs a terminal dashboard alongside the driver:

```bash
ros2 run scout_base scout_test_tui
```

It monitors CAN and ROS data without commanding motion until the operator
explicitly presses uppercase `E`. `Space`, `Esc`, and normal program exit send a
zero command. This tool supports commissioning; it does not replace the
physical emergency stop or an appropriate test enclosure.

## CAN diagnostics

The package also provides a SCOUT MINI diagnostic node:

```bash
ros2 run scout_base scout_mini_smoke --ros-args -p can_interface:=can0
```

By default, it reads state and requests controller and driver versions.
It sends zero-speed commands on exit, so run it without another active motion
controller.

To run the low-speed motion sequence, put the robot on blocks, clear its motion
envelope, and keep the physical emergency stop within reach:

```bash
ros2 run scout_base scout_mini_smoke --ros-args \
  -p can_interface:=can0 -p run_motion_test:=true
```

The sequence enables CAN commanded mode, moves forward, stops, rotates, and
stops again. On exit it sends zero-speed commands without selecting standby.
The executable is provided by `scout_base`; use this package name in place of
`agilex_ugv_sdk` when updating existing diagnostic commands.

## Robot description

`scout_description` contains the SCOUT V2 model with fixed wheel joints. Its
geometry is not a calibrated SCOUT MINI or OMNI model. Applications that use
collision geometry or wheel animation should supply a model for their vehicle.
