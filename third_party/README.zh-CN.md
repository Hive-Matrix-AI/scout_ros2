# SDK 依赖

[English](README.md) | [简体中文](README.zh-CN.md)

[`agilex_ugv_sdk`](agilex_ugv_sdk/README.md) 是面向 AgileX 新车型的通用 SDK。
本仓库锁定的版本支持 SCOUT MINI 和 SCOUT MINI OMNI；上游 SDK 可能支持更多车型。
SDK 以 Git 子模块形式提供，供 SCOUT 驱动使用。

在本仓库根目录初始化锁定的版本：

```bash
git submodule update --init --recursive
```

驱动链接 SDK 导出的 `agilex_ugv_sdk::agilex_ugv_sdk` CMake 目标。
Colcon 将此目录下的 SDK 识别为标准 CMake 包。SCOUT ROS 节点和诊断工具由 `scout_base` 提供。
如果使用独立安装的 SDK 或工作区中另一个 SDK 包，不要同时检出此嵌套副本，以免出现重复包。

SDK 使用 [Apache-2.0](agilex_ugv_sdk/LICENSE) 许可证。
