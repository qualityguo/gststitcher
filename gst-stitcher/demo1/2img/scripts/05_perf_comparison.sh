#!/bin/bash
# 性能对比测试：JSON vs Map 文件方式
# 使用 fpsdisplaysink 显示实时 FPS 和吞吐率

# ========== 配置参数 ==========
IMGS_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/2img/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
HOMOGRAPHY_FILE="${IMGS_DIR}/homography.json"
MAP_FILE="${IMGS_DIR}/stitching.map"
BORDER_WIDTH=100
DURATION=10    # 测试持续时间（秒）
# =============================

export GST_PLUGIN_PATH="${BUILD_DIR}/gst"

echo "========================================"
echo "性能对比测试：JSON vs Map 方式"
echo "使用 fpsdisplaysink 显示实时 FPS"
echo "========================================"
echo "测试时长: ${DURATION} 秒"
echo ""

# JSON 方式测试
echo ">>> 测试 JSON 方式 (homography-file) ..."
timeout ${DURATION} gst-launch-1.0 -e \
    gststitcher name=s homography-file="${HOMOGRAPHY_FILE}" border-width="${BORDER_WIDTH}" \
    filesrc location="${IMGS_DIR}/ImageA.png" ! pngdec ! imagefreeze ! \
        videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
    filesrc location="${IMGS_DIR}/ImageB.png" ! pngdec ! imagefreeze ! \
        videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
    s. ! videoconvert ! fpsdisplaysink video-sink=fakesink sync=false -v \
    2>&1 | grep -E "(FPS|fpsdisplaysink)"

echo ""
echo ">>> 测试 Map 方式 (map-file) ..."
timeout ${DURATION} gst-launch-1.0 -e \
    gststitcher name=s map-file="${MAP_FILE}" border-width="${BORDER_WIDTH}" \
    filesrc location="${IMGS_DIR}/ImageA.png" ! pngdec ! imagefreeze ! \
        videoconvert ! "video/x-raw,format=RGBA" ! s.sink_0 \
    filesrc location="${IMGS_DIR}/ImageB.png" ! pngdec ! imagefreeze ! \
        videoconvert ! "video/x-raw,format=RGBA" ! s.sink_1 \
    s. ! videoconvert ! fpsdisplaysink video-sink=fakesink sync=false -v \
    2>&1 | grep -E "(FPS|fpsdisplaysink)"

echo ""
echo "========================================"
echo "测试完成"
echo "========================================"
