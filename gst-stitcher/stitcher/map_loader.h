#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include "stitcher_types.h"
#include <stdint.h>
#include <stdio.h>

/*
 * Map file loader API
 *
 * Loads and validates pre-computed coordinate mapping files.
 * Map files contain pre-calculated transformations from canvas
 * coordinates to source image coordinates, eliminating the need
 * for per-pixel homography calculations during real-time processing.
 */

/**
 * Load a v2 map file from disk
 *
 * @param path Path to the map directory (containing metadata.json and coords_*.bin)
 * @param error_msg Error message output (caller must free)
 * @return MapFileDataV2 pointer on success, NULL on failure
 *
 * The caller is responsible for freeing the returned data with map_file_free_v2()
 *
 * Expected directory structure:
 *   path/
 *   ├── metadata.json       - JSON metadata
 *   ├── coords_0.bin        - Binary coordinates for image 0
 *   ├── coords_1.bin        - Binary coordinates for image 1
 *   └── ...
 */
MapFileDataV2 *map_file_load_v2(const char *path, char **error_msg);

/**
 * Free v2 map file data
 *
 * @param data Map file data to free (can be NULL)
 */
void map_file_free_v2(MapFileDataV2 *data);

/**
 * Validate v2 map file data
 *
 * @param data Map file data to validate
 * @param error_msg Error message output
 * @return 1 if valid, 0 otherwise
 *
 * Checks:
 * - JSON metadata is valid
 * - Dimensions are valid
 * - All coordinate flags are consistent
 */
int map_file_validate_v2(const MapFileDataV2 *data, char **error_msg);

/**
 * Get validated coordinate for a specific image and pixel
 *
 * @param data Map file data
 * @param image_index Image index (0 to num_inputs-1)
 * @param canvas_x Canvas X coordinate (relative to warp region)
 * @param canvas_y Canvas Y coordinate (relative to warp region)
 * @return Pointer to ValidatedCoordinate, or NULL if out of bounds
 */
const ValidatedCoordinate *map_file_get_coord_v2(const MapFileDataV2 *data,
                                                   int image_index,
                                                   int canvas_x, int canvas_y);

/**
 * Print map file information (for debugging)
 *
 * @param data Map file data
 * @param fp File pointer to print to (e.g., stderr)
 */
void map_file_print_info_v2(const MapFileDataV2 *data, FILE *fp);

#endif /* MAP_LOADER_H */
