#!/bin/bash
# 从视频帧生成 Homography 矩阵 (H矩阵)

# ========== 配置参数 ==========
VIDEO_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/2video"
IMGS_DIR="${VIDEO_DIR}/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
CALIBRATE_TOOL="${BUILD_DIR}/calibrate/gst-stitcher-calibrate"
# =============================

"${CALIBRATE_TOOL}" \
    -i "${IMGS_DIR}/frame1.png" "${IMGS_DIR}/frame2.png" \
    -o "${IMGS_DIR}/homography.json"
