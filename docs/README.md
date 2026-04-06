# gst-stitcher 开发文档

## 项目概述

gst-stitcher 是一个基于 GStreamer 的视频/图像拼接插件，支持 CPU/GPU 双后端，适用于全景视频拼接、监控拼接等场景。

**当前状态**: Phase 1 MVP (CPU后端，2路输入)

## 文档导航

### 开发指南
- [设计文档](../设计文档.md) - 完整的架构设计和实现计划
- [测试指南](./测试指南.md) - 编译、测试、真实图片测试完整流程

### 快速开始

```bash
# 1. 安装依赖
sudo apt-get install -y meson ninja-build libglib2.0-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libjson-glib-dev

# 2. 编译
cd gst-stitcher
meson setup build -Dcuda=false
ninja -C build

# 3. 运行单元测试
ninja -C build test

# 4. 本地测试（无需安装到系统）
export GST_PLUGIN_PATH=$(pwd)/build/gst
gst-inspect-1.0 gststitcher

# 5. 下载测试图片并运行真实图片测试
cd tests/data
bash download_test_images.sh
cd ../..
gst-launch-1.0 -e gststitcher name=s homography-file=tests/data/homographies_2img_example.json \
  filesrc location=tests/data/ImageA.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=tests/data/ImageB.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! videoconvert ! pngenc ! filesink location=output.png
```

## 核心功能

### Phase 1 (当前版本)
- ✅ 2路图像/视频拼接
- ✅ CPU后端（双线性插值）
- ✅ 单应性矩阵变换
- ✅ 线性融合
- ✅ 可配置边界融合宽度
- ✅ 支持crop裁剪

### Phase 2 (计划中)
- ⏳ 自动标定工具（基于OpenCV）
- ⏳ 命令行工具

### Phase 3 (计划中)
- ⏳ CUDA后端（GPU加速）
- ⏳ 高性能融合算法

### Phase 4 (计划中)
- ⏳ 多路输入（>2路）
- ⏳ 360度全景拼接

## 目录结构

```
gst-stitcher/
├── docs/                    # 文档
│   └── 测试指南.md
├── gst-stitcher/           # 主项目
│   ├── stitcher/           # 核心算法
│   │   ├── canvas.c        # 画布计算
│   │   ├── warp_cpu.c      # CPU warp变换
│   │   ├── blend_cpu.c     # CPU融合
│   │   ├── homography_config.c  # 单应性配置
│   │   └── blender.c       # 融合协调器
│   ├── gst/                # GStreamer插件
│   │   ├── plugin.c        # 插件入口
│   │   ├── gststitcher.c   # 元素实现
│   │   └── gststitcherpad.c # Pad实现
│   ├── tests/              # 测试
│   │   ├── unit/           # 单元测试
│   │   └── data/           # 测试数据
│   │       ├── download_test_images.sh
│   │       └── homographies_2img_example.json
│   └── build/              # 构建输出
├── 设计文档.md
└── 新的思考.md
```

## 技术栈

| 组件 | 技术 |
|------|------|
| 构建系统 | Meson + Ninja |
| 核心语言 | C (C11) |
| 插件框架 | GStreamer 1.14+ |
| 单应性变换 | 自实现 CPU 优化 |
| JSON 解析 | json-glib |
| 未来标定 | OpenCV (Phase 2) |
| 未来加速 | CUDA (Phase 3) |

## 测试覆盖

| 模块 | 测试内容 | 状态 |
|------|---------|------|
| homography_config | JSON解析、矩阵求逆 | ✅ PASS |
| canvas | 画布计算、crop处理 | ✅ PASS |
| warp_cpu | 双线性插值、边界处理 | ✅ PASS |
| 真实图片 | ImageA + ImageB 拼接 | ✅ 支持 |

## 性能参考

### CPU后端 (Phase 1)
- **720p @ 30fps**: ~50ms/frame (单线程)
- **1080p @ 30fps**: ~120ms/frame (单线程)
- **优化空间**: 多线程、SIMD

### 系统要求
- **最低**: x86_64, SSE2支持
- **推荐**: x86_64, AVX2支持
- **内存**: 2GB+

## 开发者指南

### 添加新功能
1. 更新[设计文档](../设计文档.md)
2. 实现核心算法 (`stitcher/`)
3. 添加单元测试 (`tests/unit/`)
4. 更新 GStreamer 元素 (`gst/`)
5. 更新[测试指南](./测试指南.md)

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
- 遵循 C11 标准
- 使用 GLib 类型系统
- 错误处理: GError
- 内存管理: G_DEFINE_TYPE

## 已知问题

### 当前限制
1. **符号导出**: 使用 version-script 强制导出 gst_plugin_desc
2. **性能**: 单线程，未优化
3. **功能**: 仅支持2路输入

### TODO
- [ ] 修复符号导出（更好的方案）
- [ ] 多线程优化
- [ ] SIMD 优化
- [ ] 自动标定工具

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
- `fix`: 修复bug
- `docs`: 文档更新
- `test`: 测试相关
- `refactor`: 代码重构
- `perf`: 性能优化

## 许可证

LGPL - 详见 LICENSE 文件

## 联系方式

- **Issues**: [GitHub Issues](https://github.com/qualityguo/gststitcher/issues)
- **文档**: [设计文档](../设计文档.md) | [测试指南](./测试指南.md)

---

**最后更新**: 2026-04-04
**当前版本**: 0.1.0 (Phase 1 MVP)
