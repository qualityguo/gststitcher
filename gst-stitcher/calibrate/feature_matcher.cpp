#include "feature_matcher.h"
#include <opencv2/features2d.hpp>

FeatureResult feature_detect(const cv::Mat &gray)
{
    FeatureResult result;

    auto orb = cv::ORB::create(2000);
    orb->detectAndCompute(gray, cv::noArray(),
        result.keypoints, result.descriptors);

    return result;
}

std::vector<MatchPair> match_features(
    const FeatureResult &features1,
    const FeatureResult &features2,
    float ratio_thresh)
{
    std::vector<MatchPair> good_matches;

    if (features1.descriptors.empty() || features2.descriptors.empty())
        return good_matches;

    cv::Ptr<cv::BFMatcher> matcher = cv::BFMatcher::create(cv::NORM_HAMMING);
    std::vector<std::vector<cv::DMatch>> knn_matches;
    matcher->knnMatch(features1.descriptors, features2.descriptors, knn_matches, 2);

    for (const auto &match : knn_matches) {
        if (match.size() < 2)
            continue;

        /* Lowe's ratio test */
        if (match[0].distance < ratio_thresh * match[1].distance) {
            MatchPair pair;
            pair.src = features1.keypoints[match[0].queryIdx].pt;
            pair.dst = features2.keypoints[match[0].trainIdx].pt;
            good_matches.push_back(pair);
        }
    }

    return good_matches;
}
