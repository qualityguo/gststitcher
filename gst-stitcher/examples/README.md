# gst-stitcher 示例

本目录包含 gst-stitcher 的使用示例和工具脚本。

## 快速开始

### 图像拼接演示

查看 `../demo1/` 目录中的完整演示：

```bash
cd ../demo1

# 2张图片拼接
cd 2img
./scripts/01_generate_homography.sh  # 生成H矩阵
./scripts/03_stitch_with_json.sh     # 拼接

# 3张图片拼接
cd ../3img
./scripts/01_generate_homography.sh  # 生成H矩阵
./scripts/03_stitch_with_json.sh     # 拼接
```

详见：[demo1/README.md](../demo1/README.md)

## 工具脚本

### Homography 生成工具

`scripts/generate_homography.sh` - 自动生成图像间的 homography 矩阵

**支持：**
- 2张或3张图片标定
- 可配置的 RANSAC 参数
- 自动特征检测和匹配

**基本用法：**
```bash
cd scripts
./generate_homography.sh -o output.json img1.png img2.png
```

详细说明：[scripts/README.md](./scripts/README.md)

## GStreamer 使用示例

### 图像拼接

```bash
export GST_PLUGIN_PATH=$(pwd)/build/gst

gst-launch-1.0 -e \
  gststitcher name=s homography-file=homography.json border-width=50 \
  filesrc location=img1.png ! pngdec ! videoconvert ! \
    "video/x-raw,format=RGBA" ! s.sink_0 \
  filesrc location=img2.png ! pngdec ! videoconvert ! \
    "video/x-raw,format=RGBA" ! s.sink_1 \
  s. ! videoconvert ! pngenc ! filesink location=output.png
```

### 视频拼接

```bash
gst-launch-1.0 -e \
  gststitcher name=s homography-file=homography.json \
  v4l2src device=/dev/video0 ! videoconvert ! \
    "video/x-raw,format=RGBA,width=640,height=480" ! s.sink_0 \
  v4l2src device=/dev/video1 ! videoconvert ! \
    "video/x-raw,format=RGBA,width=640,height=480" ! s.sink_1 \
  s. ! videoconvert ! autovideosink
```

## 属性配置

### gststitcher 元素属性

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `homography-file` | 字符串 | `NULL` | homography JSON 文件路径 |
| `map-file` | 字符串 | `NULL` | 预计算的 map 文件路径 |
| `border-width` | 整数 | `10` | 融合区域宽度（像素），0表示无融合 |
| `output-width` | 整数 | `-1` | 输出宽度（-1表示自动计算） |
| `output-height` | 整数 | `-1` | 输出高度（-1表示自动计算） |

### Sink Pad 属性

每个输入 pad（sink_0, sink_1, ...）支持裁剪属性：

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `left` | 整数 | `0` | 裁剪左侧像素 |
| `right` | 整数 | `0` | 裁剪右侧像素 |
| `top` | 整数 | `0` | 裁剪顶部像素 |
| `bottom` | 整数 | `0` | 裁剪底部像素 |

## Homography JSON 格式

```json
{
  "homographies": [
    {
      "images": {
        "target": 1,
        "reference": 0
      },
      "matrix": {
        "h00": 0.75, "h01": -0.03, "h02": 557.2,
        "h10": -0.007, "h11": 0.99, "h12": 60.2,
        "h20": 0.0, "h21": 0.0, "h22": 1.0
      }
    }
  ]
}
```

**字段说明：**
- `images.target` - 目标图像索引
- `images.reference` - 参考图像索引
- `matrix.h00-h22` - 3x3 单应性矩阵（行主序）

## 故障排查

### 插件无法加载

```bash
# 检查插件路径
export GST_PLUGIN_PATH=$(pwd)/build/gst

# 验证插件已加载
gst-inspect-1.0 gststitcher
```

### 输出为黑屏

```bash
# 启用调试输出
export GST_DEBUG=3,GST_STITCHER:4

# 验证 homography 文件格式
cat homography.json | python3 -m json.tool
```

### 标定失败

确保图像有足够的重叠区域（>20%）和丰富的纹理特征。

## 相关文档

- [图像拼接演示](../demo1/README.md)
- [Homography 脚本详细说明](./scripts/README.md)
