#!/bin/bash
# 从 Homography JSON 文件生成 Map 文件

# ========== 配置参数 ==========
IMGS_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/3img/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
MAPGEN_TOOL="${BUILD_DIR}/calibrate/gst-stitcher-mapgen"
HOMOGRAPHY_FILE="${IMGS_DIR}/homography.json"
MAP_FILE="${IMGS_DIR}/stitching.map"
INPUT_WIDTH=3840
INPUT_HEIGHT=2880
NUM_INPUTS=3
# =============================

"${MAPGEN_TOOL}" \
    -i "${HOMOGRAPHY_FILE}" \
    -o "${MAP_FILE}" \
    -w ${INPUT_WIDTH} -h ${INPUT_HEIGHT} -n ${NUM_INPUTS}
