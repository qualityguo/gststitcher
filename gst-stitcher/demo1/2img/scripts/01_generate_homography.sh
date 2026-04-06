#!/bin/bash
# 生成 Homography 矩阵 (H矩阵)

# ========== 配置参数 ==========
IMGS_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/2img/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
CALIBRATE_TOOL="${BUILD_DIR}/calibrate/gst-stitcher-calibrate"
# =============================

"${CALIBRATE_TOOL}" \
    -i "${IMGS_DIR}/ImageA.png" "${IMGS_DIR}/ImageB.png" \
    -o "${IMGS_DIR}/homography.json"
