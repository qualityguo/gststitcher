#!/bin/bash
# 生成 Homography 矩阵 (H矩阵)

# ========== 配置参数 ==========
IMGS_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/3img/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
CALIBRATE_TOOL="${BUILD_DIR}/calibrate/gst-stitcher-calibrate"
# =============================

"${CALIBRATE_TOOL}" \
    -i "${IMGS_DIR}/desert_left.jpg" "${IMGS_DIR}/desert_center.jpg" "${IMGS_DIR}/desert_right.jpg" \
    -o "${IMGS_DIR}/homography.json"
