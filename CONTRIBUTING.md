# Contributing

## Report a problem

Open an [issue](https://github.com/Hive-Matrix-AI/scout_ros2/issues) with the
robot model, ROS distribution, operating system, launch command, expected
behavior, and steps to reproduce. Include relevant logs after removing
credentials, personal data, and private network details.

For motion-related problems, describe the emergency-stop state and whether the
robot was raised off the ground. Never repeat an unsafe motion to collect logs.

## Submit a change

Target the `humble` branch. Keep changes focused, add regression tests when
behavior changes, and update the public documentation for interface changes.
Preserve existing license and attribution notices.

SCOUT ROS nodes, launch files, and diagnostics belong in this repository.
Shared C++ APIs and CAN protocol changes belong in
[`agilex_ugv_sdk`](https://github.com/Hive-Matrix-AI/agilex_ugv_sdk).
An SDK update must be available in that repository before changing the submodule
revision here.

From a workspace containing the repository and its initialized submodule:

```bash
source /opt/ros/humble/setup.bash
colcon build --symlink-install --cmake-args -DBUILD_TESTING=ON
source install/setup.bash
colcon test
colcon test-result --verbose
```

These tests do not require robot hardware. If you also test on a robot, state
the model and test conditions in the pull request. Keep the emergency stop
within reach and use the safety precautions in the [README](README.md).
