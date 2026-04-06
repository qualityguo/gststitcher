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

## Build & Installation

### Prerequisites

#### Core Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install -y meson ninja-build pkg-config \
  libglib2.0-dev libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev libjson-glib-dev

# Fedora/RHEL
sudo dnf install -y meson ninja-build pkg-config \
  glib2-devel gstreamer1-devel gstreamer1-plugins-base-devel \
  json-glib-devel

# Arch Linux
sudo pacman -S meson ninja pkgconf glib2 gstreamer json-glib
```

#### Optional Dependencies

```bash
# For calibration tool (OpenCV-based)
sudo apt-get install -y libopencv-dev

# For CUDA backend (GPU acceleration)
# Install CUDA Toolkit 11.0+ from NVIDIA
```

### Build Steps

```bash
# 1. Enter source directory
cd gst-stitcher/gst-stitcher

# 2. Configure build
# Basic CPU-only build:
meson setup build

# Disable calibration tool (no OpenCV):
meson setup build -Dcalibrate=false

# Enable CUDA backend (requires CUDA):
meson setup build -Dcuda=true

# 3. Build
ninja -C build

# 4. Run tests
ninja -C build test

# 5. Install (optional, installs to system)
sudo ninja -C build install
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `-Dcuda=true` | `false` | Enable CUDA backend for GPU acceleration |
| `-Dcalibrate=true` | `true` | Build calibration tool (requires OpenCV) |

### Local Testing (Without System Install)

```bash
# Set plugin path to use locally built plugin
export GST_PLUGIN_PATH=$(pwd)/build/gst

# Verify plugin is loaded
gst-inspect-1.0 gststitcher

# Run test pipeline
gst-launch-1.0 --gst-debug-level=2 gststitcher name=s ...
```

### System Installation

```bash
# Install to system directories
sudo ninja -C build install

# Update linker cache (if needed)
sudo ldconfig

# Verify installation
gst-inspect-1.0 gststitcher
```

### Uninstall

```bash
# If you installed to system
sudo ninja -C build uninstall

# Or manually remove installed files
sudo rm /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgststitcher.so
```

## Troubleshooting

### Plugin Not Found

```bash
# Check if GStreamer can find the plugin
gst-inspect-1.0 gststitcher

# If not found, set plugin path
export GST_PLUGIN_PATH=/path/to/gst-stitcher/build/gst

# Or install system-wide
sudo ninja -C build install
```

### Build Errors

```bash
# Clean build directory
rm -rf build
meson setup build --reconfigure
ninja -C build

# Check dependencies
meson setup build --wipe

# Enable verbose build output
ninja -C build -v
```

### Runtime Issues

```bash
# Enable GStreamer debug output
export GST_DEBUG=3  # 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG, 5=LOG
gst-launch-1.0 ...

# Check plugin capabilities
gst-inspect-1.0 gststitcher

# Verify caps negotiation
gst-launch-1.0 -v gststitcher name=s ... 2>&1 | grep -i caps
```

## Quick Demo

### Using Pre-built Homographies (Fastest)

```bash
cd gst-stitcher

# Set plugin path
export GST_PLUGIN_PATH=$(pwd)/build/gst

# Test with example images (if available)
gst-launch-1.0 -e gststitcher name=s \
  homography-file=tests/data/homographies_2img_example.json \
  filesrc location=tests/data/ImageA.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=tests/data/ImageB.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! pngenc ! filesink location=output.png
```

### Generate Homographies from Your Images

```bash
cd gst-stitcher

# 1. Build calibration tool (requires OpenCV)
meson setup build -Dcalibrate=true
ninja -C build

# 2. Generate homography matrix
build/calibrate/gst-stitcher-calibrate \
  -i image_left.png image_right.png \
  -o homography.json

# 3. Run stitching
export GST_PLUGIN_PATH=$(pwd)/build/gst
gst-launch-1.0 -e gststitcher name=s \
  homography-file=homography.json \
  filesrc location=image_left.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=image_right.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! pngenc ! filesink location=stitched_output.png
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

- [**Build Guide**](docs/BUILD.md) - **Complete build and installation instructions**
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
