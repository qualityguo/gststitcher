#include "json_writer.h"
#include <fstream>
#include <iomanip>
#include <iostream>

static bool write_matrix(std::ofstream &out, const cv::Mat &H)
{
    if (H.rows != 3 || H.cols != 3 || H.type() != CV_64F)
        return false;

    const char *fields[3][3] = {
        {"h00", "h01", "h02"},
        {"h10", "h11", "h12"},
        {"h20", "h21", "h22"},
    };

    out << "            \"matrix\": {" << std::endl;
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            out << "                \"" << fields[r][c] << "\": "
                << std::setprecision(16) << H.at<double>(r, c);
            if (r < 2 || c < 2)
                out << ",";
            out << std::endl;
        }
    }
    out << "            }" << std::endl;
    return true;
}

bool write_homography_json(const std::string &path,
    int target_idx, int ref_idx, const cv::Mat &H)
{
    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "Error: cannot open " << path << " for writing" << std::endl;
        return false;
    }

    out << "{" << std::endl;
    out << "    \"homographies\": [" << std::endl;
    out << "        {" << std::endl;
    out << "            \"images\": {" << std::endl;
    out << "                \"target\": " << target_idx << "," << std::endl;
    out << "                \"reference\": " << ref_idx << std::endl;
    out << "            }," << std::endl;

    write_matrix(out, H);

    out << "        }" << std::endl;
    out << "    ]" << std::endl;
    out << "}" << std::endl;

    out.close();
    std::cout << "Wrote homography to " << path << std::endl;
    return true;
}

bool write_homography_json_3img(const std::string &path,
    const cv::Mat &H_left, const cv::Mat &H_right)
{
    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "Error: cannot open " << path << " for writing" << std::endl;
        return false;
    }

    out << "{" << std::endl;
    out << "    \"homographies\": [" << std::endl;

    /* Pair 0: image 0 → image 1 (reference) */
    out << "        {" << std::endl;
    out << "            \"images\": {" << std::endl;
    out << "                \"target\": 0," << std::endl;
    out << "                \"reference\": 1" << std::endl;
    out << "            }," << std::endl;
    write_matrix(out, H_left);
    out << "        }," << std::endl;

    /* Pair 1: image 2 → image 1 (reference) */
    out << "        {" << std::endl;
    out << "            \"images\": {" << std::endl;
    out << "                \"target\": 2," << std::endl;
    out << "                \"reference\": 1" << std::endl;
    out << "            }," << std::endl;
    write_matrix(out, H_right);
    out << "        }" << std::endl;

    out << "    ]" << std::endl;
    out << "}" << std::endl;

    out.close();
    std::cout << "Wrote 3-image homography to " << path << std::endl;
    return true;
}
