#ifndef MAP_GENERATOR_H
#define MAP_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "../stitcher/stitcher_types.h"
#include "../stitcher/canvas.h"

/*
 * Map file generator API
 *
 * Generates pre-computed coordinate mapping files from homography matrices.
 * This allows the online stitching to use fast lookups instead of computing
 * coordinate transformations for each pixel.
 */

/**
 * Generate a v2 map file from homography configuration
 *
 * Creates a directory containing:
 *   - metadata.json: JSON metadata with canvas and warp region info
 *   - coords_0.bin, coords_1.bin, ...: Binary coordinate data for each input
 *
 * @param homography_json Path to homography JSON file
 * @param output_dir Path to output directory (will be created if needed)
 * @param input_sizes Array of input image dimensions
 * @param num_inputs Number of inputs
 * @param crops Array of crop parameters (can be NULL)
 * @param error_msg Error message output
 * @return true on success, false on failure
 */
bool generate_map_file_v2(
    const char *homography_json,
    const char *output_dir,
    const ImageSize *input_sizes,
    int num_inputs,
    const StitcherCrop *crops,
    char **error_msg);

#endif /* MAP_GENERATOR_H */
