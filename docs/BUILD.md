# Build Guide

Complete guide for building and installing the gst-stitcher plugin.

## Table of Contents

- [System Requirements](#system-requirements)
- [Installation](#installation)
- [Building](#building)
- [Testing](#testing)
- [Installation](#installation-1)
- [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements

| Component | Version |
|-----------|---------|
| OS | Linux (Ubuntu 18.04+, Fedora 30+, Arch) |
| Compiler | GCC 7+ or Clang 5+ |
| Meson | >= 0.56 |
| Ninja | >= 1.5 |
| GLib | >= 2.48 |
| GStreamer | >= 1.14 |
| json-glib | >= 1.2 |

### Optional Requirements

| Component | Purpose | Version |
|-----------|---------|---------|
| OpenCV | Calibration tool | >= 4.0 |
| CUDA | GPU acceleration | >= 11.0 |

## Installation

### Step 1: Install Build Tools

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y meson ninja-build pkg-config build-essential

# Fedora
sudo dnf install -y meson ninja-build pkg-config gcc gcc-c++

# Arch Linux
sudo pacman -S meson ninja pkgconf base-devel
```

### Step 2: Install Core Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install -y libglib2.0-dev \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
  libjson-glib-dev

# Fedora
sudo dnf install -y glib2-devel gstreamer1-devel \
  gstreamer1-plugins-base-devel json-glib-devel

# Arch Linux
sudo pacman -S glib2 gstreamer json-glib
```

### Step 3: Install Optional Dependencies

```bash
# For calibration tool (OpenCV)
sudo apt-get install -y libopencv-dev

# For CUDA backend
# Download and install CUDA Toolkit from NVIDIA:
# https://developer.nvidia.com/cuda-downloads
```

## Building

### Basic Build (CPU Only)

```bash
# Navigate to source directory
cd gst-stitcher/gst-stitcher

# Configure build
meson setup build

# Compile
ninja -C build
```

### Build Options

```bash
# Disable calibration tool (if you don't have OpenCV)
meson setup build -Dcalibrate=false

# Enable CUDA backend (if you have CUDA installed)
meson setup build -Dcuda=true

# Combine options
meson setup build -Dcalibrate=false -Dcuda=true

# Debug build (with debug symbols)
meson setup build --buildtype=debug

# Release build (optimized)
meson setup build --buildtype=release
```

### Build Verification

```bash
# Check if plugin was built
ls -lh build/gst/libgststitcher.so

# Check calibration tool (if enabled)
ls -lh build/calibrate/gst-stitcher-calibrate
```

## Testing

### Unit Tests

```bash
# Run all tests
ninja -C build test

# Run specific test
ninja -C build test test/test_homography_config
```

### Plugin Inspection

```bash
# Set plugin path
export GST_PLUGIN_PATH=$(pwd)/build/gst

# Verify plugin loads
gst-inspect-1.0 gststitcher
```

Expected output:
```
Factory Details:
  Rank                     none (0)
  Long-name                GstStitcher
  Klass                    Video/Effect
  Description              Stitch multiple video streams into a panoramic output
  Author                   gst-stitcher project

Plugin Details:
  Name                     gststitcher
  Description              Video stitching element
  Filename                 /path/to/build/gst/libgststitcher.so
  Version                  0.1.0
  License                  LGPL
  Source module            gststitcher
  Binary package           gst-stitcher
  Origin URL               unknown

GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstAggregator
                         +----GstStitcher
```

### Integration Test

```bash
# Test with real images
cd gst-stitcher/tests/data
bash download_test_images.sh

cd ../../..
export GST_PLUGIN_PATH=$(pwd)/build/gst

gst-launch-1.0 -e gststitcher name=s \
  homography-file=tests/data/homographies_2img_example.json \
  filesrc location=tests/data/ImageA.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=tests/data/ImageB.png ! pngdec ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! pngenc ! filesink location=test_output.png

# Check result
ls -lh test_output.png
```

## Installation

### Local Installation (Development)

```bash
# Just set GST_PLUGIN_PATH
export GST_PLUGIN_PATH=$(pwd)/build/gst

# Add to ~/.bashrc for persistence
echo 'export GST_PLUGIN_PATH=/path/to/gst-stitcher/build/gst' >> ~/.bashrc
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

Default installation paths:
- Plugin: `/usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgststitcher.so`
- Calibration tool: `/usr/local/bin/gst-stitcher-calibrate`

### Uninstallation

```bash
# Using ninja (if available)
sudo ninja -C build uninstall

# Manual removal
sudo rm /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgststitcher.so
sudo rm /usr/local/bin/gst-stitcher-calibrate
```

## Troubleshooting

### Build Issues

**Problem: meson not found**
```bash
# Install meson
sudo apt-get install -y meson
# or
pip3 install meson
```

**Problem: Dependency not found**
```bash
# Check for missing dependencies
meson setup build

# Install missing dependencies on Ubuntu
sudo apt-get install -y libglib2.0-dev libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev libjson-glib-dev
```

**Problem: OpenCV not found (when calibrate=true)**
```bash
# Either install OpenCV
sudo apt-get install -y libopencv-dev

# Or disable calibration tool
meson setup build -Dcalibrate=false
```

### Runtime Issues

**Problem: Plugin not found**
```bash
# Check if plugin is in path
gst-inspect-1.0 gststitcher

# Set plugin path explicitly
export GST_PLUGIN_PATH=/path/to/build/gst

# Or install system-wide
sudo ninja -C build install
```

**Problem: Caps negotiation error**
```bash
# Enable verbose output to debug caps issues
gst-launch-1.0 -v gststitcher name=s ... 2>&1 | grep -i caps

# Ensure format is RGBA
gst-launch-1.0 gststitcher name=s ... \
  "video/x-raw,format=RGBA,width=1920,height=1080" ! s.sink_0
```

**Problem: No output or black output**
```bash
# Enable GStreamer debug logging
export GST_DEBUG=3
gst-launch-1.0 gststitcher name=s ...

# Check homography file
cat homography.json | jq .

# Verify input images
file input_left.png input_right.png
```

### Performance Issues

**Problem: Low frame rate**
```bash
# Check CPU backend performance
export GST_DEBUG=2  # Show timing info
gst-launch-1.0 gststitcher name=s ...

# Consider enabling CUDA backend if available
meson setup build -Dcuda=true
ninja -C build
```

### Debug Tips

```bash
# Enable GStreamer debug output
export GST_DEBUG=3  # INFO level
export GST_DEBUG=4  # DEBUG level
export GST_DEBUG=5  # LOG level (verbose)

# Enable specific debug categories
export GST_DEBUG=stitcher:5  # Only stitcher debug

# Trace plugin execution
export GST_DEBUG=GST_TRACER:7
export GST_TRACERS="latency;framerate;cpuusage"

# Profile with GStreamer profiler
export GST_DEBUG="GST_TRACER:7"
export GST_TRACERS="latency;cpuusage;proctime"
gst-launch-1.0 --gst-profile=profile.log gststitcher name=s ...
```

## Build Performance Tips

```bash
# Parallel builds (use all CPU cores)
ninja -C build -j$(nproc)

# Incremental builds (only rebuild changed files)
ninja -C build

# Clean build (start fresh)
rm -rf build
meson setup build
ninja -C build
```

## Advanced Configuration

### Custom Installation Prefix

```bash
# Install to custom directory
meson setup build --prefix=/opt/gst-stitcher
ninja -C build install

# Set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/gst-stitcher/lib:$LD_LIBRARY_PATH
export GST_PLUGIN_PATH=/opt/gst-stitcher/lib/gstreamer-1.0
```

### Static Analysis

```bash
# Enable static analysis with meson
meson setup build -Db_lundef=true
ninja -C build scan-build

# Or use clang-analyzer
scan-build meson setup build
scan-build ninja -C build
```

### Code Coverage

```bash
# Enable coverage
meson setup build -Db_coverage=true
ninja -C build test
ninja -C build coverage-html

# View coverage report
firefox build/meson-logs/coveragereport/index.html
```

## Next Steps

After building and installing the plugin:

1. [Quick Start Guide](../README.md#quick-demo) - Run your first stitching pipeline
2. [Usage Examples](../README.md#usage) - Learn common usage patterns
3. [Design Document](./设计文档.md) - Understand the architecture
4. [Test Guide](./测试指南.md) - Comprehensive testing guide

## Additional Resources

- [GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)
- [Meson Build System](https://mesonbuild.com/)
- [Project GitHub](https://github.com/qualityguo/gststitcher)
