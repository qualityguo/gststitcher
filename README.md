# gststitcher

A high-performance video/image stitching plugin for GStreamer, supporting CPU/GPU dual backends.

## Features

### Current (Phase 1 MVP)
- ✅ 2-way image/video stitching
- ✅ CPU backend with bilinear interpolation
- ✅ Homography transformation
- ✅ Linear alpha blending
- ✅ Configurable border blending width
- ✅ Per-image crop support

### Planned
- ⏳ Automatic calibration tool (OpenCV-based)
- ⏳ CUDA backend (GPU acceleration)
- ⏳ Multi-camera support (3+ inputs)

## Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install -y meson ninja-build libglib2.0-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libjson-glib-dev

# Fedora
sudo dnf install -y meson ninja-build glib2-devel \
  gstreamer1-devel gstreamer1-plugins-base-devel json-glib-devel
```

### Build

```bash
cd gst-stitcher
meson setup build -Dcuda=false
ninja -C build

# Run tests
ninja -C build test

# Local test (without system install)
export GST_PLUGIN_PATH=$(pwd)/build/gst
gst-inspect-1.0 gststitcher
```

## Usage

### Test with Real Images

1. **Download test images:**

```bash
cd gst-stitcher/tests/data
bash download_test_images.sh
```

2. **Run stitching pipeline:**

```bash
cd ../..
gst-launch-1.0 -e gststitcher name=s \
  homography-file=gst-stitcher/tests/data/homographies_2img_example.json \
  filesrc location=gst-stitcher/tests/data/ImageA.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=gst-stitcher/tests/data/ImageB.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! pngenc ! filesink location=output.png
```

### Video Stitching

```bash
gst-launch-1.0 -e gststitcher name=s \
  homography-file=homographies.json \
  filesrc location=video1.mp4 ! decodebin ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=video2.mp4 ! decodebin ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! videoconvert ! x264enc ! mp4mux ! filesink location=output.mp4
```

## Test Images

### 2-Image Stitching (MVP)

| File | Description | Download |
|------|-------------|----------|
| `ImageA.png` | Left image | [Download](https://drive.google.com/uc?export=download&id=15abfY3IM-_3d6Mb6L2EZmYknP5BHhgp_) |
| `ImageB.png` | Right image | [Download](https://drive.google.com/uc?export=download&id=1j2-zvXd7Us3nPdQwEB52bcrrNbfQz6Vm) |

### 3-Image Stitching (Desert Scene)

| File | Description | Download |
|------|-------------|----------|
| `desert_left.png` | Left image | [Download](https://drive.google.com/uc?export=download&id=1XFJu40LweGZgl1zz4ZHpTiUGjiMLSsrF) |
| `desert_center.png` | Center image | [Download](https://drive.google.com/uc?export=download&id=1NS8l0sCCLeJ0ph3hZIwuCbXBlG-WTke6) |
| `desert_right.png` | Right image | [Download](https://drive.google.com/uc?export=download&id=1jlIYOiYGzmrzdXx-o1RC5uKnoxbN8Q4D) |

**Or use the download script:**
```bash
cd gst-stitcher/tests/data
bash download_test_images.sh
```

## Project Structure

```
gst-stitcher/
├── gst-stitcher/
│   ├── stitcher/          # Core algorithms
│   ├── gst/               # GStreamer plugin
│   ├── tests/             # Unit & integration tests
│   └── calibrate/         # Calibration tool (planned)
├── docs/                  # Documentation
└── README.md
```

## Documentation

- [Design Document](docs/设计文档.md) - Architecture and implementation details
- [Test Guide](docs/测试指南.md) - Build, test, and usage guide

## Technical Details

| Component | Technology |
|-----------|------------|
| Build System | Meson + Ninja |
| Language | C (C11) |
| Plugin Framework | GStreamer 1.14+ |
| Homography | Custom CPU implementation |
| JSON Parsing | json-glib |
| Future Calibration | OpenCV (Phase 2) |
| Future Acceleration | CUDA (Phase 3) |

## Performance

### CPU Backend (Phase 1)
- **720p @ 30fps**: ~50ms/frame (single-threaded)
- **1080p @ 30fps**: ~120ms/frame (single-threaded)
- **Optimization potential**: Multi-threading, SIMD

## License

LGPL - See LICENSE file for details

## Contributing

Contributions welcome! Please follow Conventional Commits:
- `feat`: New features
- `fix`: Bug fixes
- `docs`: Documentation
- `test`: Tests
- `refactor`: Code refactoring
- `perf`: Performance optimization

## Contact

- **Issues**: [GitHub Issues](https://github.com/qualityguo/gststitcher/issues)
- **Documentation**: [Design Document](docs/设计文档.md) | [Test Guide](docs/测试指南.md)

---

**Last Updated**: 2026-04-06
**Version**: 0.1.0 (Phase 1 MVP)
