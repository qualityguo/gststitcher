#!/bin/bash
# 使用 JSON 文件进行图片拼接 (H矩阵方式)

# ========== 配置参数 ==========
IMGS_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/3img/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
HOMOGRAPHY_FILE="${IMGS_DIR}/homography.json"
OUTPUT_FILE="${IMGS_DIR}/output_json.png"
BORDER_WIDTH=100
# =============================

export GST_PLUGIN_PATH="${BUILD_DIR}/gst"

gst-launch-1.0 -e \
    gststitcher name=s homography-file="${HOMOGRAPHY_FILE}" border-width="${BORDER_WIDTH}" \
    filesrc location="${IMGS_DIR}/desert_left.jpg" ! jpegdec ! videoconvert ! \
        "video/x-raw,format=RGBA" ! s.sink_0 \
    filesrc location="${IMGS_DIR}/desert_center.jpg" ! jpegdec ! videoconvert ! \
        "video/x-raw,format=RGBA" ! s.sink_1 \
    filesrc location="${IMGS_DIR}/desert_right.jpg" ! jpegdec ! videoconvert ! \
        "video/x-raw,format=RGBA" ! s.sink_2 \
    s. ! videoconvert ! pngenc ! \
    filesink location="${OUTPUT_FILE}"
