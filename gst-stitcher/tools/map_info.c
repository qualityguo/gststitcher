#include "../stitcher/map_loader.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <map_directory>\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "Map directory should contain:\n");
        fprintf(stderr, "  - metadata.json: JSON metadata\n");
        fprintf(stderr, "  - coords_0.bin, coords_1.bin, ...: Binary coordinate data\n");
        return 1;
    }

    char *error = NULL;
    MapFileDataV2 *data = map_file_load_v2(argv[1], &error);

    if (!data) {
        fprintf(stderr, "Error loading map file: %s\n", error ? error : "unknown");
        if (error) free(error);
        return 1;
    }

    map_file_print_info_v2(data, stdout);

    /* Additional debug info */
    printf("\n=== Detailed Analysis ===\n");
    for (int i = 0; i < data->num_inputs; i++) {
        int warp_pixels = data->warp_w[i] * data->warp_h[i];
        int valid_coords = 0;
        int edge_x = 0;
        int edge_y = 0;

        /* Count valid coordinates and edge cases */
        for (int j = 0; j < warp_pixels; j++) {
            ValidatedCoordinate *coord = &data->coords[i][j];
            if (coord->flags & COORD_FLAG_IS_VALID) {
                valid_coords++;
                if (!(coord->flags & COORD_FLAG_VALID_X1)) edge_x++;
                if (!(coord->flags & COORD_FLAG_VALID_Y1)) edge_y++;
            }
        }

        printf("Image %d:\n", i);
        printf("  Warp region: x=%d, y=%d, w=%d, h=%d\n",
               data->warp_x[i], data->warp_y[i],
               data->warp_w[i], data->warp_h[i]);
        printf("  Warp pixels: %d\n", warp_pixels);
        printf("  Valid coordinates: %d (%.1f%%)\n", valid_coords,
               100.0f * valid_coords / warp_pixels);
        printf("  Edge X cases (x1=x0): %d\n", edge_x);
        printf("  Edge Y cases (y1=y0): %d\n", edge_y);
        printf("  Canvas bounds: x=[%d, %d), y=[%d, %d)\n",
               data->warp_x[i], data->warp_x[i] + data->warp_w[i],
               data->warp_y[i], data->warp_y[i] + data->warp_h[i]);

        /* Check first and last pixels */
        if (data->coords[i]) {
            ValidatedCoordinate *first = &data->coords[i][0];
            ValidatedCoordinate *last = &data->coords[i][warp_pixels - 1];

            float fx0 = first->fx / 255.0f;
            float fy0 = first->fy / 255.0f;
            float fx1 = last->fx / 255.0f;
            float fy1 = last->fy / 255.0f;

            int x1_0 = (first->flags & COORD_FLAG_VALID_X1) ? first->x0 + 1 : first->x0;
            int y1_0 = (first->flags & COORD_FLAG_VALID_Y1) ? first->y0 + 1 : first->y0;
            int x1_last = (last->flags & COORD_FLAG_VALID_X1) ? last->x0 + 1 : last->x0;
            int y1_last = (last->flags & COORD_FLAG_VALID_Y1) ? last->y0 + 1 : last->y0;

            printf("  First pixel: x0=%u, y0=%u, x1=%d, y1=%d, fx=%.3f, fy=%.3f, flags=0x%02x\n",
                   first->x0, first->y0, x1_0, y1_0, fx0, fy0, first->flags);
            printf("  Last pixel: x0=%u, y0=%u, x1=%d, y1=%d, fx=%.3f, fy=%.3f, flags=0x%02x\n",
                   last->x0, last->y0, x1_last, y1_last, fx1, fy1, last->flags);
        }
        printf("\n");
    }

    map_file_free_v2(data);
    return 0;
}
