#ifndef FEATURE_MATCHER_H
#define FEATURE_MATCHER_H

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <vector>

typedef struct {
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
} FeatureResult;

typedef struct {
    cv::Point2f src;
    cv::Point2f dst;
} MatchPair;

FeatureResult feature_detect(const cv::Mat &gray);

std::vector<MatchPair> match_features(
    const FeatureResult &features1,
    const FeatureResult &features2,
    float ratio_thresh = 0.75f);

#endif /* FEATURE_MATCHER_H */
