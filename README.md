# KylinSystemMonitor

一个基于 Qt（C++17）的 Linux 桌面系统监控工具，采用 MVC 分层设计，提供系统概览、进程管理、资源监控、文件系统监控与数据导出能力。

## 功能概览

- **System（系统概览）**
  - 展示 CPU / 内存等基础系统信息
  - 展示硬件拓扑（CPU、内存、PCIe、磁盘、网络等）
  - 支持拓扑图导出
- **Process（进程管理）**
  - 进程列表与进程树视图
  - 支持结束/强制结束/挂起/恢复进程
  - 支持调整 nice 优先级
  - 支持进程资源报告与单进程 CSV/JSON 导出
  - 支持 cgroup CPU/内存限制（依赖系统权限与环境）
- **Resource（资源监控）**
  - 实时 CPU、内存、网络、磁盘速率展示
  - 历史曲线（Qt Charts）
- **File System（文件系统监控）**
  - 磁盘读写速度
  - 基于 inode 使用率的“碎片率”近似指标
- **Export（数据导出）**
  - 系统数据导出为 CSV / JSON
  - 加密导出（项目内置简化实现）

## 技术栈

- C++17
- Qt5：`core gui widgets charts sql network concurrent`
- OpenSSL：`ssl crypto`
- 运行环境：Linux（大量依赖 `/proc`、`/sys` 与 Linux 命令）

## 项目结构

```text
.
├── controller/   # 控制层：业务编排、刷新调度、导出控制
├── model/        # 模型层：系统与进程数据采集、缓存与历史记录
├── service/      # 服务层：异步监控、加密辅助
├── view/         # 视图层：各个 Tab 页面与交互组件
├── main.cpp
└── mainwindow.cpp
```

## 依赖安装（Ubuntu 示例）

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  qtbase5-dev qt5-qmake \
  libqt5charts5-dev \
  libssl-dev \
  pciutils usbutils lm-sensors procps
```

> 如果你使用的是其它发行版，请安装等效的 Qt5、Qt Charts、OpenSSL 和基础系统工具包。

## 构建与运行

在项目根目录执行：

```bash
qmake KylinSystemMonitor.pro
make -j"$(nproc)"
./KylinSystemMonitor
```

## 测试与覆盖率

新增了基于 QtTest 的服务层测试（`service/cryptohelper` 与 `service/asyncmonitor`），包含：

- 单元测试：哈希、加解密、JSON 加解密、导出成功/失败、空密钥等边界条件
- 集成测试：并发加解密、多线程 `AsyncMonitor` 启停及间隔边界（`intervalMs <= 0`）

执行方式：

```bash
chmod +x tests/run_tests_with_coverage.sh
./tests/run_tests_with_coverage.sh
```

说明：

- 脚本会构建并运行 `tests/tests.pro` 测试工程
- 若系统安装了 `gcovr`，会校验行覆盖率阈值 `>=70%`
- 若未安装 `gcovr`，仅执行测试，不做覆盖率阈值校验

## 性能/内存分析脚本

仓库提供了以下脚本：

- `memcheck.sh`：Valgrind 内存检测
- `cpu_profile.sh`：perf + top 的 CPU 采样
- `performance_test.sh`：综合性能分析（含 valgrind/perf）

注意：

- 部分脚本包含 `sudo` 命令，需要管理员权限
- `performance_test.sh` 中 `PROJECT_DIR` 为固定路径，使用前请改为你的本地实际路径

## 已知限制

- 目前面向 Linux 环境，跨平台支持有限
- 部分进程管理与 cgroup 操作需要 root 权限
- 构建依赖本机存在可用的 Qt5 工具链（如 `qmake`）

## 致谢

欢迎提交 Issue / PR 改进功能、文档与兼容性。
