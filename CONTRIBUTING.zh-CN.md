# 参与贡献

[English](CONTRIBUTING.md) | [简体中文](CONTRIBUTING.zh-CN.md)

## 反馈问题

请在 [GitHub Issues](https://github.com/Hive-Matrix-AI/scout_ros2/issues) 中提供车型、
ROS 2 发行版、操作系统、启动命令、预期行为和复现步骤。
上传日志前移除凭据、个人信息及私有网络信息。

运动相关问题还应说明急停状态和车轮是否离地。不要为收集日志重复危险操作。

## 提交修改

Pull Request 请提交到同时支持 Humble 和 Jazzy 的 `humble` 分支。
保持修改范围明确，为行为变化增加回归测试，为接口变化更新公开文档。
同步维护中英文指南中的命令、默认参数、支持环境和安全说明，并保留现有许可证与署名。

SCOUT ROS 节点、启动文件和诊断工具在本仓库维护；共享 C++ API 和 CAN 协议修改应提交到
[`agilex_ugv_sdk`](https://github.com/Hive-Matrix-AI/agilex_ugv_sdk)。
更新子模块引用前，确保对应 SDK 提交已在其远端仓库可获取。

CI 分别在 Ubuntu 22.04 / Humble 和 Ubuntu 24.04 / Jazzy 上构建、测试。
按[快速开始](README.zh-CN.md#快速开始)安装依赖，并为不同发行版使用独立工作区。
初始化子模块后，在工作区根目录运行：

```bash
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --cmake-args -DBUILD_TESTING=ON
source install/setup.bash
colcon test
colcon test-result --verbose
ros2 launch scout_base scout_mini.launch.py --show-args
ros2 launch scout_description scout_base_description.launch.py --show-args
ros2 run scout_base scout_test_tui --help
```

Humble 用户改为加载 `/opt/ros/humble/setup.bash`。
上述测试和入口检查不需要机器人硬件。
如果进行了实车验证，请在 Pull Request 中说明车型及测试条件，并遵循
[README](README.zh-CN.md) 中的安全要求，确保急停触手可及。
