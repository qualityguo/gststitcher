#!/bin/bash
# Integration test: run gststitcher with 2 image inputs and verify output

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
TEST_DATA="${PROJECT_DIR}/tests/data"
OUTPUT="/tmp/stitcher_test_output.png"

HOMOGRAPHY=$(cat "${TEST_DATA}/homographies_2img.json" | tr -d '\n' | tr -d ' ')

echo "=== Testing gststitcher image stitching ==="

gst-launch-1.0 -e gststitcher name=stitcher \
  homography-list="${HOMOGRAPHY}" \
  videotestsrc pattern=red num-buffers=1 ! \
    video/x-raw,format=RGBA,width=640,height=480 ! \
    queue ! stitcher.sink_0 \
  videotestsrc pattern=blue num-buffers=1 ! \
    video/x-raw,format=RGBA,width=640,height=480 ! \
    queue ! stitcher.sink_1 \
  stitcher. ! pngenc ! filesink location="${OUTPUT}"

if [ -f "${OUTPUT}" ]; then
  echo "Output file created: ${OUTPUT}"
  echo "File size: $(stat -c%s "${OUTPUT}") bytes"
  rm -f "${OUTPUT}"
  echo "=== Test passed ==="
else
  echo "ERROR: Output file not created"
  exit 1
fi
