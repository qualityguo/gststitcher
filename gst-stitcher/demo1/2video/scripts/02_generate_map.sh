#!/bin/bash
# 从 Homography JSON 文件生成 Map 文件

# ========== 配置参数 ==========
VIDEO_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/2video"
IMGS_DIR="${VIDEO_DIR}/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
MAPGEN_TOOL="${BUILD_DIR}/calibrate/gst-stitcher-mapgen"
HOMOGRAPHY_FILE="${IMGS_DIR}/homography.json"
MAP_FILE="${IMGS_DIR}/stitching.map"
INPUT_WIDTH=960
INPUT_HEIGHT=540
NUM_INPUTS=2
# =============================

"${MAPGEN_TOOL}" \
    -i "${HOMOGRAPHY_FILE}" \
    -o "${MAP_FILE}" \
    -w ${INPUT_WIDTH} -h ${INPUT_HEIGHT} -n ${NUM_INPUTS}
