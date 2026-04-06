# 3张图片拼接演示

本目录展示使用 gst-stitcher 进行3张图片拼接的完整流程，实现**左图 + 中间图 + 右图**的全景拼接。

## 目录结构

```
3img/
├── imgs/
│   ├── desert_left.jpg           # 左侧图片
│   ├── desert_center.jpg         # 中间图片（参考图）
│   ├── desert_right.jpg          # 右侧图片
│   ├── homography.json           # H矩阵文件（包含2个变换）
│   ├── stitching.map/            # Map文件目录
│   ├── output_json.png           # H矩阵拼接结果
│   └── output_map.png            # Map拼接结果
├── scripts/
│   ├── 01_generate_homography.sh  # 生成H矩阵
│   ├── 02_generate_map.sh          # 生成Map文件
│   ├── 03_stitch_with_json.sh      # H矩阵拼接
│   ├── 04_stitch_with_map.sh       # Map文件拼接
│   └── PROGRESS.md                 # 开发进度记录
└── README.md
```

## 使用方法

### 方式1: H矩阵拼接

```bash
cd /home/ghq/gststitcher/gst-stitcher/demo1/3img

# 1. 生成H矩阵
./scripts/01_generate_homography.sh

# 2. 使用H矩阵拼接
./scripts/03_stitch_with_json.sh
```

### 方式2: Map文件拼接

```bash
cd /home/ghq/gststitcher/gst-stitcher/demo1/3img

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
IMGS_DIR="/home/ghq/gststitcher/gst-stitcher/demo1/3img/imgs"
BUILD_DIR="/home/ghq/gststitcher/gst-stitcher/build"
MAPGEN_TOOL="${BUILD_DIR}/calibrate/gst-stitcher-mapgen"
HOMOGRAPHY_FILE="${IMGS_DIR}/homography.json"
MAP_FILE="${IMGS_DIR}/stitching.map"
INPUT_WIDTH=3840     # 输入图像宽度
INPUT_HEIGHT=2880    # 输入图像高度
NUM_INPUTS=3         # 输入图像数量
# =============================
```

## 拼接原理

### 3图拼接策略

```
左图 ──────┐
           ├──> 中间图 (参考)
右图 ──────┘
```

**变换关系：**
- `homography[0]`: 左图 → 中间图
- `homography[1]`: 右图 → 中间图

中间图作为参考坐标系，左右两张图分别变换到中间图的视角。

## 技术细节

### 标定流程

1. **特征检测**：每张图提取 ORB 特征点
2. **特征匹配**：
   - 左图 ↔ 中间图
   - 右图 ↔ 中间图
3. **RANSAC 计算**：分别计算两个 homography 矩阵
4. **输出 JSON**：保存为包含2个变换的配置文件

### 修复历史

主要修复记录见 `scripts/PROGRESS.md`，包括：
- Canvas计算修复 (warp region起点计算)
- Map生成图像尺寸修复 (从1920×1440更新为3840×2880)

## 故障排查

### 问题：拼接结果错位

**原因：** 图片顺序错误或重叠不够

**解决：**
1. 确认图片顺序：左 → 中 → 右
2. 检查相邻图片是否有足够的重叠区域（>20%）
3. 确保图片拍摄角度一致

### 问题：拼接缝明显

**解决：** 增大 `border-width` 值
```bash
# 在 03_stitch_with_json.sh 或 04_stitch_with_map.sh 中修改
BORDER_WIDTH=200  # 从100增加到200
```

### 问题：标定失败

**可能原因：**
1. 图片重叠区域太小
2. 图片缺乏纹理特征
3. 拍摄角度差异过大

**解决方法：**
使用更宽松的参数重新生成homography

## 与2张图片拼接的区别

| 特性 | 2张图片 | 3张图片 |
|------|---------|---------|
| Homography数量 | 1个 | 2个 |
| 输入源 | 2个 sink pad | 3个 sink pad |
| 参考图 | ImageA | desert_center (中间图) |
| 输出宽度 | 较窄 | 更宽 |
| Map文件 | coords_0.bin, coords_1.bin | coords_0.bin, coords_1.bin, coords_2.bin |
