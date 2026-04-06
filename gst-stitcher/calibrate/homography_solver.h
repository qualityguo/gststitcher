#ifndef HOMOGRAPHY_SOLVER_H
#define HOMOGRAPHY_SOLVER_H

#include <opencv2/core.hpp>
#include "feature_matcher.h"

cv::Mat solve_homography(
    const std::vector<MatchPair> &matches,
    double ransac_reproj_thresh = 4.0,
    double min_inlier_ratio = 0.3);

#endif /* HOMOGRAPHY_SOLVER_H */
