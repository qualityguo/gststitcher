# Demo 1: 图像拼接演示

本目录包含 gst-stitcher 的完整拼接演示，展示**融合（减少拼接缝）**和**裁剪（去除黑色边框）**功能。

## 目录结构

```
demo1/
├── 2img/              # 2张图片拼接演示
│   ├── imgs/          # 输入图片
│   ├── output/        # 输出结果
│   ├── scripts/       # 拼接脚本
│   └── README.md      # 详细说明
│
└── 3img/              # 3张图片拼接演示
    ├── imgs/          # 输入图片
    ├── output/        # 输出结果
    ├── scripts/       # 拼接脚本
    └── README.md      # 详细说明
```

## 快速开始

### 2张图片拼接

```bash
cd 2img
./scripts/run.sh
```

**演示内容：**
- 使用 ImageA.png 和 ImageB.png 进行拼接
- 展示融合和裁剪功能
- 输出干净的全景图

### 3张图片拼接

```bash
cd 3img
./scripts/run.sh
```

**演示内容：**
- 使用 desert 场景的左、中、右3张图片
- 实现宽幅全景拼接
- 展示多图拼接能力

## 功能特性

| 特性 | 说明 |
|------|------|
| **融合** | 在重叠区域使用线性渐变混合，减少拼接缝 |
| **裁剪** | 自动移除拼接后的黑色边框 |
| **自动标定** | 使用 OpenCV 自动计算 homography 矩阵 |
| **灵活配置** | 支持调整融合宽度、裁剪参数 |

## 使用场景

### 2张图片拼接
- ✅ 全景照片拼接
- ✅ 文档扫描拼接
- ✅ 双摄像头合成

### 3张图片拼接
- ✅ 宽幅风景照
- ✅ 多视角覆盖
- ✅ 高分辨率全景

## 详细文档

- **2张图片拼接**: [2img/README.md](./2img/README.md)
- **3张图片拼接**: [3img/README.md](./3img/README.md)
- **标定工具**: [examples/scripts/README.md](../examples/scripts/README.md)

## 技术栈

- **GStreamer**: 多媒体框架
- **OpenCV**: 计算机视觉库（特征检测、RANSAC）
- **gst-stitcher**: 自定义拼接插件

## 相关资源

- [项目 README](../../README.md)
- [示例文档](../examples/README.md)
- [测试指南](../../docs/测试指南.md)
