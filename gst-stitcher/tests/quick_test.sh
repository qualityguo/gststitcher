#!/bin/bash
# gst-stitcher 快速测试脚本
# 用于验证插件基本功能

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "========================================"
echo "gst-stitcher 快速测试"
echo "========================================"
echo ""

# 检查构建目录
if [ ! -d "$PROJECT_DIR/build" ]; then
    echo "❌ 错误: 构建目录不存在"
    echo "   请先运行: meson setup build"
    exit 1
fi

# 检查插件文件
if [ ! -f "$PROJECT_DIR/build/gst/libgststitcher.so" ]; then
    echo "❌ 错误: 插件未编译"
    echo "   请先运行: ninja -C build"
    exit 1
fi

echo "✓ 构建目录检查通过"
echo ""

# 设置环境变量
export GST_PLUGIN_PATH="$PROJECT_DIR/build/gst"
export GST_DEBUG=2

echo "📋 测试列表:"
echo ""

# 测试1: 检查插件是否可以加载
echo "[1/4] 插件加载测试..."
if gst-inspect-1.0 gststitcher >/dev/null 2>&1; then
    echo "✓ gststitcher 插件加载成功"
    VERSION=$(gst-inspect-1.0 gststitcher 2>/dev/null | grep "Version" | head -1 | awk '{print $2}')
    echo "  版本: $VERSION"
else
    echo "❌ 插件加载失败"
    echo "  请检查编译输出和符号表"
    exit 1
fi
echo ""

# 测试2: 运行单元测试
echo "[2/4] 单元测试..."
cd "$PROJECT_DIR"
if ninja -C build test >/dev/null 2>&1; then
    echo "✓ 所有单元测试通过"
    TEST_COUNT=$(find build/tests -name "test_*" -type f | wc -l)
    echo "  测试数量: $TEST_COUNT"
else
    echo "❌ 单元测试失败"
    echo "  请运行: ninja -C build test"
    exit 1
fi
echo ""

# 测试3: 合成源快速测试
echo "[3/4] 合成源功能测试..."
echo "  运行简单拼接管道（100帧）..."

if timeout 10 gst-launch-1.0 -e --gst-debug-level=0 \
  gststitcher name=s \
    homography-list='{"homographies":[{"images":{"target":1,"reference":0},"matrix":{"h00":1,"h01":0,"h02":320,"h10":0,"h11":1,"h12":0,"h20":0,"h21":0,"h22":1}}]}' \
  videotestsrc pattern=snow num-buffers=50 ! "video/x-raw,format=RGBA,width=640,height=480,framerate=30/1" ! s.sink_0 \
  videotestsrc pattern=ball num-buffers=50 ! "video/x-raw,format=RGBA,width=640,height=480,framerate=30/1" ! s.sink_1 \
  s. ! videoconvert ! fakesink >/dev/null 2>&1; then
    echo "✓ 合成源测试通过"
else
    echo "❌ 合成源测试失败"
    echo "  可能需要检查 GStreamer 管道配置"
fi
echo ""

# 测试4: 检查测试数据
echo "[4/4] 测试数据检查..."
if [ -d "$PROJECT_DIR/tests/data" ]; then
    IMAGE_COUNT=$(find "$PROJECT_DIR/tests/data" -name "*.png" -type f 2>/dev/null | wc -l)
    if [ $IMAGE_COUNT -gt 0 ]; then
        echo "✓ 测试图片已下载 ($IMAGE_COUNT 个)"
        echo "  位置: tests/data/"
    else
        echo "⚠ 测试图片未下载"
        echo "  运行: cd tests/data && bash download_test_images.sh"
    fi
else
    echo "⚠ 测试数据目录不存在"
fi
echo ""

# 总结
echo "========================================"
echo "✓ 快速测试完成!"
echo "========================================"
echo ""
echo "📚 下一步:"
echo ""
echo "1. 查看完整测试指南:"
echo "   cat docs/测试指南.md"
echo ""
echo "2. 下载真实图片并测试:"
echo "   cd tests/data && bash download_test_images.sh"
echo ""
echo "3. 运行真实图片拼接:"
echo "   export GST_PLUGIN_PATH=$(pwd)/build/gst"
echo "   gst-launch-1.0 -e gststitcher name=s \\"
echo "     homography-file=tests/data/homographies_2img_example.json \\"
echo "     filesrc location=tests/data/ImageA.png ! pngdec ! videoconvert ! \\"
echo "       \"video/x-raw,format=RGBA\" ! s.sink_0 \\"
echo "     filesrc location=tests/data/ImageB.png ! pngdec ! videoconvert ! \\"
echo "       \"video/x-raw,format=RGBA\" ! s.sink_1 \\"
echo "     s. ! videoconvert ! pngenc ! filesink location=output.png"
echo ""
