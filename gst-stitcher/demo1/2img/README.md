# 测试脚本使用说明

## 目录结构

```
2img/
├── imgs/
│   ├── ImageA.png          # 原始图片A
│   ├── ImageB.png          # 原始图片B
│   ├── homography.json     # H矩阵文件
│   ├── stitching.map/      # Map文件目录
│   ├── output_json.png     # H矩阵拼接结果
│   └── output_map.png      # Map拼接结果
└── scripts/
    ├── 01_generate_homography.sh  # 生成H矩阵
    ├── 02_generate_map.sh          # 生成Map文件
    ├── 03_stitch_with_json.sh      # H矩阵拼接
    └── 04_stitch_with_map.sh       # Map文件拼接
```

## 使用方法

### 方式1: H矩阵拼接

```bash
# 1. 生成H矩阵
./scripts/01_generate_homography.sh

# 2. 使用H矩阵拼接
./scripts/03_stitch_with_json.sh
```

### 方式2: Map文件拼接

```bash
# 1. 生成H矩阵
./scripts/01_generate_homography.sh

# 2. 生成Map文件
./scripts/02_generate_map.sh

# 3. 使用Map文件拼接
./scripts/04_stitch_with_map.sh
```

## 参数修改

打开脚本文件，修改`配置参数`部分的变量即可。

例如 `scripts/02_generate_map.sh`:

```bash
# ========== 配置参数 ==========
IMGS_DIR="./imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
MAPGEN_TOOL="${BUILD_DIR}/calibrate/gst-stitcher-mapgen"
HOMOGRAPHY_FILE="${IMGS_DIR}/homography.json"
MAP_FILE="${IMGS_DIR}/stitching.map"
CANVAS_WIDTH=1920     # 修改这里
CANVAS_HEIGHT=1080    # 修改这里
NUM_INPUTS=2          # 修改这里
# =============================
```
