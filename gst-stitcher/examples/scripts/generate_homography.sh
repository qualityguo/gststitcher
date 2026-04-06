#!/bin/bash
# Generate homography matrix JSON file using calibration tool
# Usage: ./generate_homography.sh [OPTIONS] -o output.json image1.png image2.png [image3.png]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
CALIBRATE_TOOL="${PROJECT_DIR}/../build/calibrate/gst-stitcher-calibrate"

# Default parameters
RANSAC_THRESH=4.0
MIN_INLIER_RATIO=0.3
VERBOSE=0
OUTPUT=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_usage() {
    echo "Usage: $0 [OPTIONS] -o output.json image1.png image2.png [image3.png]"
    echo ""
    echo "Generate homography matrix JSON file for gst-stitcher using OpenCV calibration."
    echo ""
    echo "Required arguments:"
    echo "  -o <path>              Output JSON file path"
    echo "  image1.png             First input image (reference for 2-image, left for 3-image)"
    echo "  image2.png             Second input image"
    echo "  image3.png             Optional third input image (for 3-image stitching)"
    echo ""
    echo "Options:"
    echo "  -r <threshold>         RANSAC reprojection threshold (default: 4.0)"
    echo "  -t <ratio>             Minimum inlier ratio (default: 0.3)"
    echo "  -v                     Verbose output (show feature detection details)"
    echo "  -h                     Show this help message"
    echo ""
    echo "Example usage:"
    echo "  # 2-image calibration"
    echo "  $0 -o homography.json img_left.png img_right.png"
    echo ""
    echo "  # 3-image calibration with verbose output"
    echo "  $0 -v -o homography_3img.png img_left.png img_center.png img_right.png"
    echo ""
    echo "  # With stricter RANSAC parameters"
    echo "  $0 -r 3.0 -t 0.4 -o homography.json img1.png img2.png"
    exit 1
}

# Parse arguments
while getopts "o:r:t:vh" opt; do
    case $opt in
        o)
            OUTPUT="$OPTARG"
            ;;
        r)
            RANSAC_THRESH="$OPTARG"
            ;;
        t)
            MIN_INLIER_RATIO="$OPTARG"
            ;;
        v)
            VERBOSE=1
            ;;
        h)
            print_usage
            ;;
        \?)
            echo -e "${RED}Error: Invalid option -$OPTARG${NC}" >&2
            print_usage
            ;;
    esac
done

shift $((OPTIND-1))

# Check required arguments
if [ -z "$OUTPUT" ]; then
    echo -e "${RED}Error: Output file path required (-o)${NC}" >&2
    print_usage
fi

if [ $# -lt 2 ]; then
    echo -e "${RED}Error: At least 2 input images required${NC}" >&2
    print_usage
fi

if [ $# -gt 3 ]; then
    echo -e "${RED}Error: Maximum 3 input images supported${NC}" >&2
    print_usage
fi

# Check if calibrate tool exists
if [ ! -f "$CALIBRATE_TOOL" ]; then
    echo -e "${RED}Error: Calibrate tool not found at $CALIBRATE_TOOL${NC}" >&2
    echo "Please build the project first:" >&2
    echo "  cd $PROJECT_DIR && meson setup build && ninja -C build" >&2
    exit 1
fi

# Check if input images exist
for img in "$@"; do
    if [ ! -f "$img" ]; then
        echo -e "${RED}Error: Image file not found: $img${NC}" >&2
        exit 1
    fi
done

# Build command
CMD="$CALIBRATE_TOOL -i $@ -o $OUTPUT -r $RANSAC_THRESH -t $MIN_INLIER_RATIO"
if [ $VERBOSE -eq 1 ]; then
    CMD="$CMD -v"
fi

# Print info
echo -e "${GREEN}=== gst-stitcher Homography Generator ===${NC}"
echo "Input images: $@"
echo "Output file: $OUTPUT"
echo "RANSAC threshold: $RANSAC_THRESH"
echo "Minimum inlier ratio: $MIN_INLIER_RATIO"
echo ""

# Run calibration
echo -e "${YELLOW}Running calibration...${NC}"
if eval $CMD; then
    echo ""
    echo -e "${GREEN}✓ Success! Homography matrix saved to: $OUTPUT${NC}"

    # Show preview of generated JSON
    if [ -f "$OUTPUT" ]; then
        echo ""
        echo -e "${YELLOW}Generated JSON preview:${NC}"
        head -n 10 "$OUTPUT"
        echo "..."
        echo ""
        echo -e "${YELLOW}File size: $(wc -c < "$OUTPUT") bytes${NC}"
    fi

    echo ""
    echo "Usage with gst-stitcher:"
    echo "  gst-launch-1.0 -e gststitcher name=s homography-file=$OUTPUT ..."
    echo ""
    exit 0
else
    echo -e "${RED}✗ Calibration failed${NC}" >&2
    echo "Troubleshooting:" >&2
    echo "  1. Ensure images have sufficient overlap (>30%)" >&2
    echo "  2. Try lowering RANSAC threshold (-r 3.0)" >&2
    echo "  3. Try lowering inlier ratio (-t 0.2)" >&2
    echo "  4. Use -v flag to see detailed matching info" >&2
    exit 1
fi
