#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include <opencv2/core.hpp>
#include <string>

/*
 * Write homography result to a JSON file compatible with gststitcher.
 * H maps target image → reference image coordinates.
 *
 * JSON format:
 * {
 *   "homographies": [
 *     {
 *       "images": { "target": target_idx, "reference": ref_idx },
 *       "matrix": { "h00":..., "h01":..., ..., "h22":... }
 *     }
 *   ]
 * }
 */
bool write_homography_json(const std::string &path,
    int target_idx, int ref_idx,
    const cv::Mat &H);

/*
 * Write 3-image homography result (two pairs).
 * H_left maps image 0 → image 1.
 * H_right maps image 2 → image 1.
 */
bool write_homography_json_3img(const std::string &path,
    const cv::Mat &H_left, const cv::Mat &H_right);

#endif /* JSON_WRITER_H */
