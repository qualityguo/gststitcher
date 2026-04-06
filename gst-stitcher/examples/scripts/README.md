# Homography 生成脚本使用指南

本目录包含用于生成 homography 矩阵 JSON 配置文件的便捷脚本。

## 脚本

### `generate_homography.sh` - Homography 生成工具

支持 2 或 3 张图片的标定，提供灵活的参数配置。

**用法：**
```bash
./generate_homography.sh [OPTIONS] -o output.json img1.png img2.png [img3.png]
```

**选项：**
- `-o <path>` - 输出 JSON 文件路径（必需）
- `-r <threshold>` - RANSAC 重投影阈值（默认：4.0）
- `-t <ratio>` - 最小内点比例（默认：0.3）
- `-v` - 详细输出（显示特征检测细节）
- `-h` - 显示帮助信息

**示例：**

1. **基本用法（2张图片）：**
```bash
./generate_homography.sh -o homography.json left.png right.png
```

2. **3张图片标定：**
```bash
./generate_homography.sh -o homography_3img.png \
  left.png center.png right.png
```

3. **严格参数（更精确但可能失败）：**
```bash
./generate_homography.sh -r 3.0 -t 0.4 -o homography.json img1.png img2.png
```

4. **详细输出模式：**
```bash
./generate_homography.sh -v -o homography.json img1.png img2.png
```

## 生成的 JSON 格式

### 2张图片：
```json
{
    "homographies": [
        {
            "images": {
                "target": 1,
                "reference": 0
            },
            "matrix": {
                "h00": 0.6525,
                "h01": 0.0251,
                "h02": 1165.4677,
                "h10": -0.1007,
                "h11": 0.8759,
                "h12": 107.5538,
                "h20": -0.000091,
                "h21": 0.0000008,
                "h22": 1.0
            }
        }
    ]
}
```

### 3张图片：
```json
{
    "homographies": [
        {
            "images": {
                "target": 0,
                "reference": 1
            },
            "matrix": { /* 左图→中图的矩阵 */ }
        },
        {
            "images": {
                "target": 2,
                "reference": 1
            },
            "matrix": { /* 右图→中图的矩阵 */ }
        }
    ]
}
```

## 在 gst-stitcher 中使用

### 方法1：使用文件
```bash
export GST_PLUGIN_PATH=$(pwd)/build/gst

gst-launch-1.0 -e \
  gststitcher name=s homography-file=homography.json \
  filesrc location=img1.png ! pngdec ! videoconvert ! \
    "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=img2.png ! pngdec ! videoconvert ! \
    "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! videoconvert ! pngenc ! filesink location=output.png
```

### 方法2：使用内联 JSON
```bash
HOMOGRAPHY=$(cat homography.json | tr -d '\n' | tr -d ' ')

gst-launch-1.0 -e \
  gststitcher name=s homography-list="${HOMOGRAPHY}" \
  videotestsrc pattern=snow ! "video/x-raw,format=RGBA" ! s.sink_0 \
  videotestsrc pattern=ball ! "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! videoconvert ! autovideosink
```

## 参数调优建议

### RANSAC 阈值 (`-r`)
- **默认值：** 4.0
- **降低到 3.0** - 更严格，只接受非常准确的匹配
- **提高到 6.0** - 更宽松，接受更多匹配点
- **适用场景：**
  - 高质量图片、清晰重叠：3.0-4.0
  - 运动模糊、低光照：5.0-6.0

### 最小内点比例 (`-t`)
- **默认值：** 0.3 (30%)
- **提高到 0.4** - 需要更多一致匹配点
- **降低到 0.2** - 接受较少匹配点
- **适用场景：**
  - 大重叠区域（>50%）：0.4-0.5
  - 小重叠区域（30-40%）：0.2-0.3

## 故障排查

### 问题：标定失败（内点太少）
**解决方案：**
```bash
# 降低阈值要求
./generate_homography.sh -r 6.0 -t 0.2 -o homography.json img1.png img2.png
```

### 问题：生成的 homography 不准确
**解决方案：**
```bash
# 使用更严格的参数
./generate_homography.sh -r 3.0 -t 0.4 -v -o homography.json img1.png img2.png
```

### 问题：图片没有足够的特征点
**解决方案：**
- 确保图片有清晰的纹理和角点
- 避免纯色区域过多
- 增加图片分辨率

## 工作流程示例

完整的工作流程示例：

```bash
# 1. 准备图片
cp /path/to/your/left.png tests/data/
cp /path/to/your/right.png tests/data/

# 2. 生成 homography
./examples/scripts/generate_homography.sh \
  -o tests/data/my_homography.json \
  tests/data/left.png \
  tests/data/right.png

# 3. 测试拼接
export GST_PLUGIN_PATH=$(pwd)/build/gst

gst-launch-1.0 -e \
  gststitcher name=s homography-file=tests/data/my_homography.json \
  filesrc location=tests/data/left.png ! pngdec ! videoconvert ! \
    "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=tests/data/right.png ! pngdec ! videoconvert ! \
    "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! videoconvert ! pngenc ! filesink location=tests/data/output.png

# 4. 查看结果
ls -lh tests/data/output.png
```

## 技术细节

### 标定流程
1. **特征检测**：使用 ORB 特征检测器（每张图最多 2000 个关键点）
2. **特征匹配**：使用 BruteForce-Hamming 距离匹配
3. **筛选优质匹配**： Lowe's ratio test (ratio < 0.7)
4. **RANSAC**：计算 homography 矩阵并过滤外点
5. **输出 JSON**：保存为 gst-stitcher 兼容格式

### 性能参考
- **特征检测**：<1s per image (2000 keypoints)
- **特征匹配**：<1s
- **RANSAC 计算**：<1s
- **总耗时**：2-3s（2张4K图片）

## 相关文档

- [测试指南](../../docs/测试指南.md) - 完整的测试流程
- [设计文档](../../docs/设计文档.md) - 架构设计
- [项目 README](../../docs/README.md) - 项目概述
