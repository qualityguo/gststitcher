#!/bin/bash
# 使用 JSON 文件进行视频拼接 (H矩阵方式) - 直接处理MP4

# ========== 配置参数 ==========
VIDEO_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/2video"
IMGS_DIR="${VIDEO_DIR}/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
HOMOGRAPHY_FILE="${IMGS_DIR}/homography.json"
OUTPUT_FILE="${VIDEO_DIR}/videos/output_json.mp4"
BORDER_WIDTH=100
INPUT_WIDTH=960
INPUT_HEIGHT=540
FPS=25
# =============================

export GST_PLUGIN_PATH="${BUILD_DIR}/gst"

echo "=== Video Stitching with Homography JSON ==="
echo "Input 1: ${VIDEO_DIR}/videos/video1.mp4"
echo "Input 2: ${VIDEO_DIR}/videos/video2.mp4"
echo "Output: ${OUTPUT_FILE}"
echo "Homography: ${HOMOGRAPHY_FILE}"
echo ""

# 直接使用 GStreamer 处理 MP4 视频
echo "Processing videos with GStreamer..."
gst-launch-1.0 -e \
    gststitcher name=s homography-file="${HOMOGRAPHY_FILE}" border-width="${BORDER_WIDTH}" \
    filesrc location="${VIDEO_DIR}/videos/video1.mp4" ! qtdemux ! h264parse ! avdec_h264 ! \
        videoconvert ! "video/x-raw,format=RGBA,width=${INPUT_WIDTH},height=${INPUT_HEIGHT},framerate=${FPS}/1" ! \
        queue ! s.sink_0 \
    filesrc location="${VIDEO_DIR}/videos/video2.mp4" ! qtdemux ! h264parse ! avdec_h264 ! \
        videoconvert ! "video/x-raw,format=RGBA,width=${INPUT_WIDTH},height=${INPUT_HEIGHT},framerate=${FPS}/1" ! \
        queue ! s.sink_1 \
    s. ! videoconvert ! x264enc bitrate=2048 speed-preset=fast ! h264parse ! qtmux ! \
        filesink location="${OUTPUT_FILE}"

echo ""
echo "Done! Output: ${OUTPUT_FILE}"
