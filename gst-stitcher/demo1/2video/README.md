# 2Video 拼接测试

## 目录结构

```
2video/
├── videos/
│   ├── video1.mp4          # 第一个输入视频
│   ├── video2.mp4          # 第二个输入视频
│   ├── output_json.mp4     # 使用 H 矩阵拼接的输出
│   └── output_map.mp4      # 使用 Map 文件拼接的输出
├── imgs/
│   ├── frame1.png          # 从 video1.mp4 提取的参考帧
│   ├── frame2.png          # 从 video2.mp4 提取的参考帧
│   ├── homography.json     # 生成的 Homography 矩阵文件
│   └── stitching.map       # 生成的 Map 文件
└── scripts/
    ├── 01_generate_homography.sh  # 从参考帧生成 H 矩阵
    ├── 02_generate_map.sh          # 从 H 矩阵生成 Map 文件
    ├── 03_stitch_with_json.sh      # 使用 H 矩阵拼接视频
    └── 04_stitch_with_map.sh       # 使用 Map 文件拼接视频
```

## 使用流程

### 1. 生成 Homography 矩阵

从视频的参考帧计算 Homography 矩阵：

```bash
./scripts/01_generate_homography.sh
```

### 2. 生成 Map 文件（可选）

从 Homography 矩阵生成 Map 文件：

```bash
./scripts/02_generate_map.sh
```

### 3. 拼接视频

使用 GStreamer 直接处理 MP4 视频：

**方式 A：使用 Homography JSON 文件**
```bash
./scripts/03_stitch_with_json.sh
```

**方式 B：使用 Map 文件（推荐，性能更好）**
```bash
./scripts/04_stitch_with_map.sh
```

## 输出

输出视频保存在 `videos/` 目录下：
- `output_json.mp4` - 使用 H 矩阵拼接的输出
- `output_map.mp4` - 使用 Map 文件拼接的输出

## 参数说明

- `INPUT_WIDTH=960` - 输入视频宽度
- `INPUT_HEIGHT=540` - 输入视频高度
- `FPS=25` - 视频帧率
- `BORDER_WIDTH=100` - 拼接边框宽度
- `x264enc bitrate=2048` - H.264 编码比特率

## 所需 GStreamer 插件

确保已安装以下插件：
- `gstreamer1.0-plugins-base` - qtdemux, qtmux
- `gstreamer1.0-plugins-bad` - h264parse
- `gstreamer1.0-plugins-ugly` - avdec_h264
- `gstreamer1.0-libav` - avdec_h264
- `gstreamer1.0-tools` - gst-launch-1.0

验证安装：
```bash
gst-inspect-1.0 qtdemux
gst-inspect-1.0 avdec_h264
gst-inspect-1.0 x264enc
gst-inspect-1.0 qtmux
```
