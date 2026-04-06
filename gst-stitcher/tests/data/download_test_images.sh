#!/bin/bash
# 下载 gst-stitcher 测试图片
# 来源: RidgeRun cuda-stitcher 示例图片

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "gst-stitcher 测试图片下载脚本"
echo "========================================"
echo ""

# Phase 1: 2路拼接测试图片
echo "📥 [1/5] 下载 Phase 1 MVP 测试图片 (2路拼接)..."

if [ -f "ImageA.png" ] && [ -f "ImageB.png" ]; then
    echo "✓ ImageA.png 和 ImageB.png 已存在，跳过下载"
else
    echo "  正在下载 ImageA.png (左图)..."
    curl -L -o ImageA.png "https://drive.google.com/uc?export=download&id=15abfY3IM-_3d6Mb6L2EZmYknP5BHhgp_"

    echo "  正在下载 ImageB.png (右图)..."
    curl -L -o ImageB.png "https://drive.google.com/uc?export=download&id=1j2-zvXd7Us3nPdQwEB52bcrrNbfQz6Vm"

    echo "✓ Phase 1 测试图片下载完成"
fi

# Phase 4: 3路拼接测试图片（沙漠场景）
echo ""
echo "📥 [2/5] 下载 Phase 4 测试图片 (3路拼接 - 沙漠场景)..."

if [ -f "desert_left.png" ] && [ -f "desert_center.png" ] && [ -f "desert_right.png" ]; then
    echo "✓ 沙漠场景图片已存在，跳过下载"
else
    echo "  正在下载 desert_left.png..."
    curl -L -o desert_left.png "https://drive.google.com/uc?export=download&id=1XFJu40LweGZgl1zz4ZHpTiUGjiMLSsrF"

    echo "  正在下载 desert_center.png..."
    curl -L -o desert_center.png "https://drive.google.com/uc?export=download&id=1NS8l0sCCLeJ0ph3hZIwuCbXBlG-WTke6"

    echo "  正在下载 desert_right.png..."
    curl -L -o desert_right.png "https://drive.google.com/uc?export=download&id=1jlIYOiYGzmrzdXx-o1RC5uKnoxbN8Q4D"

    echo "✓ Phase 4 测试图片下载完成"
fi

# 创建示例 homography 配置文件
echo ""
echo "📝 [3/5] 创建示例 homography 配置文件..."

cat > homographies_2img_example.json << 'EOF'
{
  "homographies": [
    {
      "images": {
        "target": 1,
        "reference": 0
      },
      "matrix": {
        "h00": 0.752823,
        "h01": -0.031433,
        "h02": 557.207642,
        "h10": -0.006882,
        "h11": 0.991056,
        "h12": 60.221836,
        "h20": -0.000021,
        "h21": -0.000018,
        "h22": 1.000000
      }
    }
  ]
}
EOF

echo "✓ 已创建 homographies_2img_example.json (示例配置，实际使用需要标定)"

# 验证下载的文件
echo ""
echo "🔍 [4/5] 验证下载的文件..."

check_file() {
    if [ -f "$1" ]; then
        size=$(du -h "$1" | cut -f1)
        echo "  ✓ $1 ($size)"
    else
        echo "  ✗ $1 未找到!"
        return 1
    fi
}

echo "Phase 1 测试图片:"
check_file "ImageA.png"
check_file "ImageB.png"

echo ""
echo "Phase 4 测试图片:"
check_file "desert_left.png"
check_file "desert_center.png"
check_file "desert_right.png"

echo ""
echo "配置文件:"
check_file "homographies_2img_example.json"

# 显示使用说明
echo ""
echo "========================================"
echo "✓ [5/5] 下载完成!"
echo "========================================"
echo ""
echo "测试图片已保存在: $SCRIPT_DIR"
echo ""
echo "📚 快速开始:"
echo ""
echo "1. 设置插件路径:"
echo "   export GST_PLUGIN_PATH=../../build/gst"
echo ""
echo "2. 验证插件加载:"
echo "   gst-inspect-1.0 gststitcher"
echo ""
echo "3. 运行2路拼接测试:"
echo "   gst-launch-1.0 -e \\"
echo "     gststitcher name=s homography-file=homographies_2img_example.json \\"
echo "     filesrc location=ImageA.png ! pngdec ! videoconvert ! \\"
echo "       \"video/x-raw,format=RGBA,width=640,height=480,framerate=1/1\" ! s.sink_0 \\"
echo "     filesrc location=ImageB.png ! pngdec ! videoconvert ! \\"
echo "       \"video/x-raw,format=RGBA,width=640,height=480,framerate=1/1\" ! s.sink_1 \\"
echo "     s. ! videoconvert ! pngenc ! filesink location=output_stitched.png"
echo ""
echo "更多测试方法请参考: ../../docs/测试指南.md"
echo ""
