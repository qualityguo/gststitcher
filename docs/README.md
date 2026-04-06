# gst-stitcher 开发文档

## 项目概述

gst-stitcher 是一个基于 GStreamer 的高性能视频/图像拼接插件，支持多种运行模式和后端加速。

**当前版本**: 0.1.0
**核心特性**:
- ✅ 支持 2+ 路图像/视频拼接
- ✅ 双模式运行：Homography 变换模式 + Map 文件模式
- ✅ CPU 后端（双线性插值）
- ✅ 线性融合 + 边界裁剪
- ✅ 可配置融合区域宽度
- ✅ 预计算 Map 文件支持（高性能实时拼接）

## 文档导航

### 核心文档
- [测试指南](./测试指南.md) - 编译、测试、真实图片测试完整流程
- [设计文档](./设计文档.md) - 架构设计和实现细节

### 快速开始

```bash
# 1. 安装依赖
sudo apt-get install -y meson ninja-build libglib2.0-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libjson-glib-dev

# 2. 编译
cd gst-stitcher
meson setup build -Dcuda=false
ninja -C build

# 3. 运行测试
ninja -C build test

# 4. 本地测试
export GST_PLUGIN_PATH=$(pwd)/build/gst
gst-inspect-1.0 gststitcher

# 5. 运行演示
cd demo1/2img
./scripts/run.sh
```

## 核心功能

### 运行模式

**模式 1: Homography 变换模式**
- 适用场景：动态拼接、实时处理
- 输入格式：JSON 格式的单应性矩阵
- 优势：灵活、支持任意变换

**模式 2: Map 文件模式** ⭐ 推荐
- 适用场景：固定相机位置、高性能实时拼接
- 输入格式：预计算的坐标映射文件（v2 格式）
- 优势：性能极高、延迟低、支持多相机扩展

### 已实现功能

| 功能 | 状态 | 说明 |
|------|------|------|
| 2路图像拼接 | ✅ | 基础拼接功能 |
| 3+路图像拼接 | ✅ | 支持多相机全景拼接 |
| 视频拼接 | ✅ | 实时视频流处理 |
| CPU 后端 | ✅ | 双线性插值 |
| 线性融合 | ✅ | 可配置融合宽度 |
| 边界裁剪 | ✅ | 去除黑色边框 |
| Map 文件支持 | ✅ | v2 格式，高性能 |
| 标定工具 | ✅ | OpenCV 自动标定 |

### 计划中功能

| 功能 | 状态 | 说明 |
|------|------|------|
| CUDA 后端 | ⏳ | GPU 加速（Phase 3） |
| 多频段融合 | ⏳ | 更好的拼接效果 |
| 性能优化 | ⏳ | 多线程、SIMD |

## 目录结构

```
gst-stitcher/
├── gst-stitcher/
│   ├── stitcher/              # 核心算法
│   │   ├── canvas.c           # 画布计算
│   │   ├── warp_cpu.c         # CPU warp 变换
│   │   ├── warp_map.c         # Map 文件 warp
│   │   ├── blend_cpu.c        # CPU 融合
│   │   ├── homography_config.c  # 单应性配置
│   │   ├── map_loader.c       # Map 文件加载器
│   │   └── blender.c          # 融合协调器
│   ├── gst/                   # GStreamer 插件
│   │   ├── plugin.c           # 插件入口
│   │   ├── gststitcher.c      # 元素实现
│   │   └── gststitcherpad.c   # Pad 实现
│   ├── calibrate/             # 标定工具
│   │   ├── calibrate_main.cpp # 主程序
│   │   ├── feature_matcher.cpp # 特征匹配
│   │   ├── homography_solver.cpp # 单应性求解
│   │   ├── json_writer.cpp    # JSON 输出
│   │   └── map_generator.cpp  # Map 文件生成
│   ├── tests/                 # 测试
│   │   ├── unit/              # 单元测试
│   │   └── data/              # 测试数据
│   ├── tools/                 # 工具
│   │   └── map_info           # Map 文件查看工具
│   └── demo1/                 # 演示
│       ├── 2img/              # 2图拼接演示
│       ├── 2video/            # 2视频拼接演示
│       └── 3img/              # 3图拼接演示
├── docs/                      # 文档
└── README.md                  # 项目说明
```

## 技术栈

| 组件 | 技术 |
|------|------|
| 构建系统 | Meson + Ninja |
| 核心语言 | C (C11) + C++ (标定工具) |
| 插件框架 | GStreamer 1.14+ |
| 单应性变换 | 自实现 CPU 优化 |
| Map 文件 | 自定义二进制格式 |
| JSON 解析 | json-glib |
| 标定工具 | OpenCV 4.0+ |
| 未来加速 | CUDA (Phase 3) |

## 测试覆盖

| 模块 | 测试内容 | 状态 |
|------|---------|------|
| homography_config | JSON 解析、矩阵求逆 | ✅ PASS |
| canvas | 画布计算、crop 处理 | ✅ PASS |
| warp_cpu | 双线性插值、边界处理 | ✅ PASS |
| 真实图片 | ImageA + ImageB 拼接 | ✅ 支持 |
| Map 文件 | v2 格式加载和验证 | ✅ 支持 |

## 性能参考

### CPU 后端
- **720p @ 30fps**: ~50ms/frame (单线程)
- **1080p @ 30fps**: ~120ms/frame (单线程)
- **Map 模式**: ~20ms/frame (预计算，无矩阵运算)

### Map 文件模式性能

| 分辨率 | Homography 模式 | Map 模式 | 加速比 |
|--------|-----------------|----------|--------|
| 720p   | 50ms            | 20ms     | 2.5x   |
| 1080p  | 120ms           | 40ms     | 3x     |

**系统要求**:
- **最低**: x86_64, SSE2 支持
- **推荐**: x86_64, AVX2 支持
- **内存**: 2GB+

## 使用示例

### 模式 1: Homography JSON

```bash
gst-launch-1.0 -e gststitcher name=s \
  homography-file=homographies.json \
  filesrc location=img1.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=img2.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! pngenc ! filesink location=output.png
```

### 模式 2: Map 文件（推荐）

```bash
gst-launch-1.0 -e gststitcher name=s \
  map-file=stitching.map \
  filesrc location=video1.mp4 ! decodebin ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=video2.mp4 ! decodebin ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! videoconvert ! x264enc ! mp4mux ! filesink location=output.mp4
```

## 开发者指南

### 添加新功能
1. 更新[设计文档](../设计文档.md)
2. 实现核心算法 (`stitcher/`)
3. 添加单元测试 (`tests/unit/`)
4. 更新 GStreamer 元素 (`gst/`)
5. 更新演示脚本 (`demo1/`)

### 调试技巧
```bash
# 启用详细日志
export GST_DEBUG=3,GST_STITCHER:4

# 检查数据流
gst-launch-1.0 ... ! fakesink dump=1

# 性能分析
export GST_TRACERS="latency;fps;cpuusage"
```

### 代码规范
- C 代码遵循 C11 标准
- C++ 代码遵循 C++17 标准
- 使用 GLib 类型系统
- 错误处理: GError
- 内存管理: G_DEFINE_TYPE

## 已知问题

### 当前限制
1. **性能**: 单线程，未优化
2. **融合**: 仅支持线性 alpha 融合
3. **CUDA**: 尚未实现

### TODO
- [ ] 多线程优化
- [ ] SIMD 优化
- [ ] CUDA 后端实现
- [ ] 多频段融合
- [ ] 自动曝光调整

## 贡献指南

欢迎贡献！请遵循以下流程：

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'feat: add AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 提交规范
遵循 Conventional Commits:
- `feat`: 新功能
- `fix`: 修复 bug
- `docs`: 文档更新
- `test`: 测试相关
- `refactor`: 代码重构
- `perf`: 性能优化

## 许可证

LGPL - 详见 LICENSE 文件

## 联系方式

- **Issues**: [GitHub Issues](https://github.com/qualityguo/gststitcher/issues)
- **文档**: [设计文档](./设计文档.md) | [测试指南](./测试指南.md)

---

**最后更新**: 2026-04-06
**当前版本**: 0.1.0
