#!/bin/bash
# 使用 Map 文件进行图片拼接 (预计算方式)

# ========== 配置参数 ==========
IMGS_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/2img/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
MAP_FILE="${IMGS_DIR}/stitching.map"
OUTPUT_FILE="${IMGS_DIR}/output_map.png"
BORDER_WIDTH=100
# =============================

export GST_PLUGIN_PATH="${BUILD_DIR}/gst"

gst-launch-1.0 -e \
    gststitcher name=s map-file="${MAP_FILE}" border-width="${BORDER_WIDTH}" \
    filesrc location="${IMGS_DIR}/ImageA.png" ! pngdec ! videoconvert ! \
        "video/x-raw,format=RGBA" ! s.sink_0 \
    filesrc location="${IMGS_DIR}/ImageB.png" ! pngdec ! videoconvert ! \
        "video/x-raw,format=RGBA" ! s.sink_1 \
    s. ! videoconvert ! pngenc ! \
    filesink location="${OUTPUT_FILE}"
