#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "map_generator.h"

static void print_usage(const char *prog_name)
{
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Generate pre-computed coordinate map files from homography matrices.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -i <file>    Path to homography JSON file (required)\n");
    fprintf(stderr, "  -o <dir>     Path to output map directory (required)\n");
    fprintf(stderr, "  -w <width>   Input image width (required)\n");
    fprintf(stderr, "  -h <height>  Input image height (required)\n");
    fprintf(stderr, "  -n <count>   Number of inputs (default: 2)\n");
    fprintf(stderr, "  --crop <l,r,t,b>  Crop parameters (default: 0,0,0,0)\n");
    fprintf(stderr, "  -v           Verbose output\n");
    fprintf(stderr, "  --help       Show this help message\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Output:\n");
    fprintf(stderr, "  Creates a directory containing:\n");
    fprintf(stderr, "    - metadata.json: JSON metadata\n");
    fprintf(stderr, "    - coords_0.bin, coords_1.bin, ...: Binary coordinate data\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "  %s -i homography.json -o stitching.map -w 1920 -h 1080 -n 2\n", prog_name);
}

int main(int argc, char *argv[])
{
    const char *homography_file = NULL;
    const char *output_file = NULL;
    int input_width = 0;
    int input_height = 0;
    int num_inputs = 2;
    StitcherCrop crop = {0, 0, 0, 0};
    bool verbose = false;
    char *error_msg = NULL;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -i requires an argument\n");
                return 1;
            }
            homography_file = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-w") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -w requires an argument\n");
                return 1;
            }
            input_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -h requires an argument\n");
                return 1;
            }
            input_height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-n") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -n requires an argument\n");
                return 1;
            }
            num_inputs = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--crop") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --crop requires an argument\n");
                return 1;
            }
            if (sscanf(argv[++i], "%d,%d,%d,%d",
                       &crop.left, &crop.right, &crop.top, &crop.bottom) != 4) {
                fprintf(stderr, "Error: Invalid crop format. Use: left,right,top,bottom\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Validate required arguments */
    if (!homography_file) {
        fprintf(stderr, "Error: -i (homography file) is required\n");
        print_usage(argv[0]);
        return 1;
    }

    if (!output_file) {
        fprintf(stderr, "Error: -o (output file) is required\n");
        print_usage(argv[0]);
        return 1;
    }

    if (input_width <= 0 || input_height <= 0) {
        fprintf(stderr, "Error: -w and -h must be positive integers\n");
        return 1;
    }

    if (num_inputs < 1 || num_inputs > STITCHER_MAX_INPUTS) {
        fprintf(stderr, "Error: Number of inputs must be between 1 and %d\n",
                STITCHER_MAX_INPUTS);
        return 1;
    }

    if (verbose) {
        fprintf(stderr, "Map file generator\n");
        fprintf(stderr, "==================\n");
        fprintf(stderr, "Homography file: %s\n", homography_file);
        fprintf(stderr, "Output file: %s\n", output_file);
        fprintf(stderr, "Input size: %dx%d\n", input_width, input_height);
        fprintf(stderr, "Number of inputs: %d\n", num_inputs);
        fprintf(stderr, "Crop: L=%d R=%d T=%d B=%d\n",
                crop.left, crop.right, crop.top, crop.bottom);
        fprintf(stderr, "\n");
    }

    /* Prepare input sizes array */
    ImageSize *input_sizes = (ImageSize *)calloc(num_inputs, sizeof(ImageSize));
    if (input_sizes == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }

    for (int i = 0; i < num_inputs; i++) {
        input_sizes[i].w = input_width;
        input_sizes[i].h = input_height;
    }

    /* Generate map file */
    StitcherCrop crops[STITCHER_MAX_INPUTS];
    for (int i = 0; i < num_inputs; i++) {
        crops[i] = crop;
    }

    bool success = generate_map_file_v2(homography_file, output_file,
                                        input_sizes, num_inputs,
                                        crops, &error_msg);

    free(input_sizes);

    if (!success) {
        fprintf(stderr, "Error: %s\n", error_msg ? error_msg : "unknown error");
        free(error_msg);
        return 1;
    }

    if (verbose) {
        fprintf(stderr, "Map file generated successfully: %s\n", output_file);
    }

    return 0;
}
