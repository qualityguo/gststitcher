#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "feature_matcher.h"
#include "homography_solver.h"
#include "json_writer.h"

static void print_usage(const char *prog)
{
    std::cerr << "Usage: " << prog << " [OPTIONS] -i <image0> <image1> [<image2>] -o <output.json>"
              << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -i <images>   Input image paths (2 or 3 images)" << std::endl;
    std::cerr << "  -o <path>     Output JSON file path" << std::endl;
    std::cerr << "  -r <thresh>   RANSAC reprojection threshold (default: 4.0)" << std::endl;
    std::cerr << "  -t <ratio>    Minimum inlier ratio (default: 0.3)" << std::endl;
    std::cerr << "  -v            Verbose output" << std::endl;
    std::cerr << "  -h            Show this help" << std::endl;
}

int main(int argc, char *argv[])
{
    std::vector<std::string> input_paths;
    std::string output_path;
    double ransac_thresh = 4.0;
    double min_inlier_ratio = 0.3;
    bool verbose = false;

    /* Parse arguments */
    int i = 1;
    while (i < argc) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            i++;
            while (i < argc && argv[i][0] != '-') {
                input_paths.push_back(argv[i]);
                i++;
            }
            continue;
        } else if (arg == "-o" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "-r" && i + 1 < argc) {
            ransac_thresh = std::atof(argv[++i]);
        } else if (arg == "-t" && i + 1 < argc) {
            min_inlier_ratio = std::atof(argv[++i]);
        } else if (arg == "-v") {
            verbose = true;
        } else if (arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
        i++;
    }

    if (input_paths.size() < 2) {
        std::cerr << "Error: need at least 2 input images" << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    if (input_paths.size() > 3) {
        std::cerr << "Error: maximum 3 input images supported" << std::endl;
        return 1;
    }
    if (output_path.empty()) {
        std::cerr << "Error: output path required (-o)" << std::endl;
        return 1;
    }

    /* Load images */
    std::vector<cv::Mat> images;
    for (const auto &path : input_paths) {
        cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
        if (img.empty()) {
            std::cerr << "Error: cannot load image: " << path << std::endl;
            return 1;
        }
        images.push_back(img);
        if (verbose)
            std::cout << "Loaded " << path << " (" << img.cols << "x" << img.rows << ")" << std::endl;
    }

    /* Convert to grayscale */
    std::vector<cv::Mat> gray(images.size());
    for (size_t j = 0; j < images.size(); j++)
        cv::cvtColor(images[j], gray[j], cv::COLOR_BGR2GRAY);

    /* Detect features */
    std::vector<FeatureResult> features(images.size());
    for (size_t j = 0; j < images.size(); j++) {
        features[j] = feature_detect(gray[j]);
        if (verbose)
            std::cout << "Image " << j << ": " << features[j].keypoints.size()
                      << " keypoints" << std::endl;
    }

    if (images.size() == 2) {
        /* 2-image calibration: image1 is target, image0 is reference */
        auto matches = match_features(features[1], features[0]);
        if (verbose)
            std::cout << "Good matches: " << matches.size() << std::endl;

        if (matches.size() < 4) {
            std::cerr << "Error: not enough matches (" << matches.size() << "), need at least 4" << std::endl;
            return 1;
        }

        cv::Mat H = solve_homography(matches, ransac_thresh, min_inlier_ratio);
        if (H.empty()) {
            std::cerr << "Error: failed to compute homography" << std::endl;
            return 1;
        }

        std::cout << "Homography (image1 → image0):" << std::endl;
        std::cout << H << std::endl;

        if (!write_homography_json(output_path, 1, 0, H))
            return 1;

    } else {
        /* 3-image calibration: center image (1) is reference */
        /* Left pair: image0 → image1 */
        auto matches_left = match_features(features[0], features[1]);
        if (verbose)
            std::cout << "Left pair good matches: " << matches_left.size() << std::endl;

        if (matches_left.size() < 4) {
            std::cerr << "Error: not enough matches for left pair" << std::endl;
            return 1;
        }

        cv::Mat H_left = solve_homography(matches_left, ransac_thresh, min_inlier_ratio);
        if (H_left.empty()) {
            std::cerr << "Error: failed to compute left homography" << std::endl;
            return 1;
        }

        /* Right pair: image2 → image1 */
        auto matches_right = match_features(features[2], features[1]);
        if (verbose)
            std::cout << "Right pair good matches: " << matches_right.size() << std::endl;

        if (matches_right.size() < 4) {
            std::cerr << "Error: not enough matches for right pair" << std::endl;
            return 1;
        }

        cv::Mat H_right = solve_homography(matches_right, ransac_thresh, min_inlier_ratio);
        if (H_right.empty()) {
            std::cerr << "Error: failed to compute right homography" << std::endl;
            return 1;
        }

        std::cout << "Homography left (image0 → image1):" << std::endl;
        std::cout << H_left << std::endl;
        std::cout << "Homography right (image2 → image1):" << std::endl;
        std::cout << H_right << std::endl;

        if (!write_homography_json_3img(output_path, H_left, H_right))
            return 1;
    }

    return 0;
}
