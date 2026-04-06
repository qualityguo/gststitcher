#include "homography_solver.h"
#include <opencv2/calib3d.hpp>
#include <iostream>

cv::Mat solve_homography(
    const std::vector<MatchPair> &matches,
    double ransac_reproj_thresh,
    double min_inlier_ratio)
{
    if (matches.size() < 4)
        return cv::Mat();

    /* Separate into src (target) and dst (reference) point arrays */
    std::vector<cv::Point2f> src_pts, dst_pts;
    src_pts.reserve(matches.size());
    dst_pts.reserve(matches.size());

    for (const auto &m : matches) {
        src_pts.push_back(m.src);
        dst_pts.push_back(m.dst);
    }

    /* Find homography using RANSAC: maps src → dst */
    cv::Mat mask;
    cv::Mat H = cv::findHomography(src_pts, dst_pts, cv::RANSAC,
        ransac_reproj_thresh, mask);

    if (H.empty()) {
        std::cerr << "Warning: findHomography returned empty matrix" << std::endl;
        return cv::Mat();
    }

    /* Count inliers */
    int inliers = cv::countNonZero(mask);
    double inlier_ratio = (double)inliers / (double)matches.size();

    std::cout << "RANSAC inliers: " << inliers << " / " << matches.size()
              << " (" << (inlier_ratio * 100.0) << "%)" << std::endl;

    if (inlier_ratio < min_inlier_ratio) {
        std::cerr << "Warning: inlier ratio (" << inlier_ratio
                  << ") below threshold (" << min_inlier_ratio << ")" << std::endl;
        return cv::Mat();
    }

    return H;
}
