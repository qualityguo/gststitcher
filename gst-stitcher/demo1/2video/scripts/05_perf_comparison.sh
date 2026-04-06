#!/bin/bash
# 性能对比测试：JSON vs Map 文件方式 (视频流)
# 使用 fpsdisplaysink 显示实时 FPS 和吞吐率

# ========== 配置参数 ==========
VIDEO_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/2video/videos"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
HOMOGRAPHY_FILE="${VIDEO_DIR}/homography.json"
MAP_FILE="${VIDEO_DIR}/stitching.map"
BORDER_WIDTH=100
DURATION=15    # 测试持续时间（秒）
# =============================

export GST_PLUGIN_PATH="${BUILD_DIR}/gst"

echo "========================================"
echo "性能对比测试：JSON vs Map 方式 (视频)"
echo "使用 fpsdisplaysink 显示实时 FPS"
echo "========================================"
echo "测试时长: ${DURATION} 秒"
echo "视频文件: video1.mp4, video2.mp4"
echo ""

# 检查视频文件
if [ ! -f "${VIDEO_DIR}/video1.mp4" ] || [ ! -f "${VIDEO_DIR}/video2.mp4" ]; then
    echo "错误: 找不到视频文件 video1.mp4 或 video2.mp4"
    exit 1
fi

# JSON 方式测试
echo ">>> 测试 JSON 方式 (homography-file) ..."
timeout ${DURATION} gst-launch-1.0 -e \
    gststitcher name=s homography-file="${HOMOGRAPHY_FILE}" border-width="${BORDER_WIDTH}" \
    filesrc location="${VIDEO_DIR}/video1.mp4" ! decodebin ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
    filesrc location="${VIDEO_DIR}/video2.mp4" ! decodebin ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
    s. ! videoconvert ! fpsdisplaysink video-sink=fakesink sync=false -v \
    2>&1 | grep -E "(current:|average:)"

echo ""
echo ">>> 测试 Map 方式 (map-file) ..."
timeout ${DURATION} gst-launch-1.0 -e \
    gststitcher name=s map-file="${MAP_FILE}" border-width="${BORDER_WIDTH}" \
    filesrc location="${VIDEO_DIR}/video1.mp4" ! decodebin ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
    filesrc location="${VIDEO_DIR}/video2.mp4" ! decodebin ! videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
    s. ! videoconvert ! fpsdisplaysink video-sink=fakesink sync=false -v \
    2>&1 | grep -E "(current:|average:)"

echo ""
echo "========================================"
echo "测试完成"
echo "========================================"
echo "对于视频流，关注点："
echo "  1. 稳定性：FPS 波动范围"
echo "  2. 平均吞吐率：处理速度"
echo "  3. 两种方式的性能差异"
echo "========================================"
