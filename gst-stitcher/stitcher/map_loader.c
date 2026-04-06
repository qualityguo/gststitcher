#define _POSIX_C_SOURCE 200809L
#include "map_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdarg.h>

/* strdup wrapper - provide our own if not available */
#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200809L
static char *strdup_impl(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *d = (char *)malloc(len);
    if (d) memcpy(d, s, len);
    return d;
}
#define strdup(s) strdup_impl(s)
#endif

/* Minimal JSON parser for our specific use case */
typedef struct JSONValue {
    char *key;
    char *string_value;
    double number_value;
    int is_object;
    int is_array;
    struct JSONValue *child;
    struct JSONValue *next;
} JSONValue;

static void json_free(JSONValue *json) {
    if (!json) return;
    if (json->child) json_free(json->child);
    if (json->next) json_free(json->next);
    if (json->string_value) free(json->string_value);
    if (json->key) free(json->key);
    free(json);
}

/* Very simple JSON parser - only handles what we need */
static JSONValue *parse_json_value(const char **str);
static void skip_ws(const char **str) {
    while (**str == ' ' || **str == '\t' || **str == '\n' || **str == '\r') (*str)++;
}

static JSONValue *parse_json_object(const char **str) {
    JSONValue *obj = NULL;
    JSONValue **tail = &obj;

    (*str)++; /* skip '{' */
    skip_ws(str);

    if (**str == '}') { (*str)++; return obj; } /* Empty object */

    while (**str) {
        skip_ws(str);
        if (**str != '"') break;
        (*str)++; /* skip opening quote */

        /* Parse key */
        char key[256] = {0};
        size_t key_len = 0;
        while (**str && **str != '"' && key_len < sizeof(key) - 1) {
            key[key_len++] = **str;
            (*str)++;
        }
        (*str)++; /* skip closing quote */

        skip_ws(str);
        if (**str != ':') break;
        (*str)++;
        skip_ws(str);

        /* Parse value */
        JSONValue *val = parse_json_value(str);
        if (val) {
            val->key = strdup(key);
            *tail = val;
            tail = &val->next;
        }

        skip_ws(str);
        if (**str == ',') { (*str)++; continue; }
        if (**str == '}') { (*str)++; break; }
    }

    return obj;
}

static JSONValue *parse_json_array(const char **str) {
    JSONValue *arr = NULL;
    JSONValue **tail = &arr;

    (*str)++; /* skip '[' */
    skip_ws(str);

    if (**str == ']') { (*str)++; return arr; } /* Empty array */

    while (**str) {
        JSONValue *val = parse_json_value(str);
        if (val) {
            val->is_array = 1;
            *tail = val;
            tail = &val->next;
        }
        skip_ws(str);
        if (**str == ',') { (*str)++; continue; }
        if (**str == ']') { (*str)++; break; }
    }

    return arr;
}

static JSONValue *parse_json_value(const char **str) {
    skip_ws(str);

    JSONValue *val = (JSONValue *)calloc(1, sizeof(JSONValue));
    if (!val) return NULL;

    if (**str == '{') {
        val->is_object = 1;
        val->child = parse_json_object(str);
    } else if (**str == '[') {
        val->is_array = 1;
        val->child = parse_json_array(str);
    } else if (**str == '"') {
        (*str)++; /* skip opening quote */
        char buf[1024] = {0};
        size_t len = 0;
        while (**str && **str != '"' && len < sizeof(buf) - 1) {
            buf[len++] = **str;
            (*str)++;
        }
        (*str)++; /* skip closing quote */
        val->string_value = strdup(buf);
    } else if (**str == '-' || (**str >= '0' && **str <= '9')) {
        char *end = NULL;
        val->number_value = strtod(*str, &end);
        *str = end;
    } else if (strncmp(*str, "true", 4) == 0) {
        val->number_value = 1;
        *str += 4;
    } else if (strncmp(*str, "false", 4) == 0) {
        val->number_value = 0;
        *str += 5;
    } else if (strncmp(*str, "null", 4) == 0) {
        *str += 4;
    } else {
        free(val);
        return NULL;
    }

    return val;
}

static JSONValue *json_parse(const char *str) {
    skip_ws(&str);
    /* Create a parent object node to hold the top-level object's children */
    JSONValue *val = (JSONValue *)calloc(1, sizeof(JSONValue));
    if (!val) return NULL;
    val->is_object = 1;
    val->child = parse_json_object(&str);
    return val;
}

static JSONValue *json_get_object_item(JSONValue *object, const char *key) {
    JSONValue *item = object->child;
    while (item) {
        if (item->key && strcmp(item->key, key) == 0) {
            return item;
        }
        item = item->next;
    }
    return NULL;
}

static int json_get_array_size(JSONValue *array) {
    int size = 0;
    JSONValue *item = array->child;
    while (item) { size++; item = item->next; }
    return size;
}

static JSONValue *json_get_array_item(JSONValue *array, int index) {
    JSONValue *item = array->child;
    while (item && index-- > 0) item = item->next;
    return item;
}

#define cJSON_is_number(v) ((v)->string_value == NULL && !(v)->is_object && !(v)->is_array)
#define cJSON_is_string(v) ((v)->string_value != NULL)
#define cJSON_is_object(v) ((v)->is_object)
#define cJSON_is_array(v) ((v)->is_array)
#define cJSON_GetObjectItemCaseSensitive json_get_object_item
#define cJSON_GetArraySize json_get_array_size
#define cJSON_GetArrayItem json_get_array_item
#define cJSON_Delete json_free
#define cJSON_Parse json_parse
#define cJSON JSONValue

/* Helper to set error messages */
static void set_error(char **error_msg, const char *format, ...) {
    if (error_msg == NULL)
        return;

    va_list args;
    *error_msg = (char *)malloc(1024);
    if (*error_msg) {
        va_start(args, format);
        vsnprintf(*error_msg, 1024, format, args);
        va_end(args);
    }
}

/* Helper to read entire file into memory */
static char *read_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buffer, 1, size, f);
    fclose(f);

    buffer[read] = '\0';
    if (out_size) *out_size = read;
    return buffer;
}

/* Check if path is a directory */
static int is_directory(const char *path) {
    fprintf(stderr, "is_directory: ENTER, path=%s\n", path ? path : "NULL");
    fflush(stderr);

    struct stat st;
    fprintf(stderr, "is_directory: about to call stat()\n");
    fflush(stderr);

    int result = stat(path, &st);
    fprintf(stderr, "is_directory: stat() returned %d\n", result);
    fflush(stderr);

    if (result != 0) {
        fprintf(stderr, "is_directory: stat() failed, returning 0\n");
        fflush(stderr);
        return 0;
    }

    int is_dir = S_ISDIR(st.st_mode);
    fprintf(stderr, "is_directory: S_ISDIR=%d\n", is_dir);
    fflush(stderr);

    return is_dir;
}

MapFileDataV2 *map_file_load_v2(const char *path, char **error_msg) {
    fprintf(stderr, "map_file_load_v2: ENTER, path=%s\n", path ? path : "NULL");
    fflush(stderr);

    MapFileDataV2 *data = NULL;
    char *json_content = NULL;
    cJSON *json = NULL;
    char metadata_path[512];
    char coords_path[512];

    fprintf(stderr, "map_file_load_v2: Checking if directory\n");
    fflush(stderr);

    /* Check if path is a directory */
    fprintf(stderr, "map_file_load_v2: calling is_directory()\n");
    fflush(stderr);

    int is_dir = is_directory(path);

    fprintf(stderr, "map_file_load_v2: is_directory() returned %d\n", is_dir);
    fflush(stderr);

    if (!is_dir) {
        fprintf(stderr, "map_file_load_v2: path is not a directory\n");
        fflush(stderr);
        set_error(error_msg, "Map path is not a directory: %s", path);
        return NULL;
    }

    fprintf(stderr, "map_file_load_v2: path is a directory, continuing\n");
    fflush(stderr);

    /* Build metadata.json path */
    fprintf(stderr, "map_file_load_v2: about to call snprintf for metadata_path\n");
    fflush(stderr);

    snprintf(metadata_path, sizeof(metadata_path), "%s/metadata.json", path);

    fprintf(stderr, "map_file_load_v2: metadata_path=%s\n", metadata_path);
    fflush(stderr);

    /* Read metadata.json */
    fprintf(stderr, "map_file_load_v2: about to call read_file\n");
    fflush(stderr);

    size_t json_size;
    json_content = read_file(metadata_path, &json_size);

    fprintf(stderr, "map_file_load_v2: read_file returned, json_content=%p, json_size=%zu\n",
            (void*)json_content, json_size);
    fflush(stderr);
    if (!json_content) {
        set_error(error_msg, "Failed to read metadata.json from: %s", metadata_path);
        return NULL;
    }

    /* Parse JSON */
    fprintf(stderr, "map_file_load_v2: about to call cJSON_Parse\n");
    fflush(stderr);

    json = cJSON_Parse(json_content);

    fprintf(stderr, "map_file_load_v2: cJSON_Parse returned, json=%p\n", (void*)json);
    fflush(stderr);

    if (!json) {
        fprintf(stderr, "map_file_load_v2: cJSON_Parse FAILED\n");
        fflush(stderr);
        set_error(error_msg, "Failed to parse metadata.json");
        goto error;
    }

    fprintf(stderr, "map_file_load_v2: cJSON_Parse succeeded\n");
    fflush(stderr);

    /* Verify magic and version */
    fprintf(stderr, "map_file_load_v2: about to get magic item\n");
    fflush(stderr);

    cJSON *magic = cJSON_GetObjectItemCaseSensitive(json, "magic");

    fprintf(stderr, "map_file_load_v2: about to get version item\n");
    fflush(stderr);

    cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");

    fprintf(stderr, "map_file_load_v2: magic=%p, version=%p\n", (void*)magic, (void*)version);
    fflush(stderr);

    /* Debug: print the JSON to see what we actually parsed */
    fprintf(stderr, "map_file_load_v2: JSON content (first 200 chars):\n%.200s\n", json_content);
    fflush(stderr);

    /* Check if magic or version is NULL before accessing */
    if (!magic) {
        fprintf(stderr, "map_file_load_v2: ERROR - magic is NULL!\n");
        fflush(stderr);
        set_error(error_msg, "Missing 'magic' field in metadata.json");
        goto error;
    }

    if (!version) {
        fprintf(stderr, "map_file_load_v2: ERROR - version is NULL!\n");
        fflush(stderr);
        set_error(error_msg, "Missing 'version' field in metadata.json");
        goto error;
    }

    if (!cJSON_is_string(magic) || strcmp(magic->string_value, "GSTSTMAP") != 0) {
        set_error(error_msg, "Invalid or missing magic number");
        goto error;
    }
    if (!cJSON_is_number(version) || version->number_value != 2) {
        set_error(error_msg, "Unsupported version: %.0f (expected 2)", version->number_value);
        goto error;
    }

    /* Allocate data structure */
    data = (MapFileDataV2 *)calloc(1, sizeof(MapFileDataV2));
    if (!data) {
        set_error(error_msg, "Failed to allocate memory");
        goto error;
    }

    /* Parse canvas dimensions */
    cJSON *canvas = cJSON_GetObjectItemCaseSensitive(json, "canvas");
    if (!canvas || !cJSON_is_object(canvas)) {
        set_error(error_msg, "Missing or invalid 'canvas' object");
        goto error;
    }
    cJSON *canvas_w = cJSON_GetObjectItemCaseSensitive(canvas, "width");
    cJSON *canvas_h = cJSON_GetObjectItemCaseSensitive(canvas, "height");
    if (!cJSON_is_number(canvas_w) || !cJSON_is_number(canvas_h)) {
        set_error(error_msg, "Missing canvas dimensions");
        goto error;
    }
    data->canvas_w = (int32_t)canvas_w->number_value;
    data->canvas_h = (int32_t)canvas_h->number_value;

    /* Parse inputs array */
    cJSON *inputs = cJSON_GetObjectItemCaseSensitive(json, "inputs");
    if (!inputs || !cJSON_is_array(inputs)) {
        set_error(error_msg, "Missing or invalid 'inputs' array");
        goto error;
    }

    data->num_inputs = cJSON_GetArraySize(inputs);
    if (data->num_inputs <= 0 || data->num_inputs > STITCHER_MAX_INPUTS) {
        set_error(error_msg, "Invalid number of inputs: %d", data->num_inputs);
        goto error;
    }

    /* Allocate per-input arrays */
    data->warp_x = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->warp_y = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->warp_w = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->warp_h = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->input_w = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->input_h = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->crop_left = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->crop_right = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->crop_top = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->crop_bottom = (int32_t *)calloc(data->num_inputs, sizeof(int32_t));
    data->coords = (ValidatedCoordinate **)calloc(data->num_inputs, sizeof(ValidatedCoordinate *));

    if (!data->warp_x || !data->warp_y || !data->warp_w || !data->warp_h ||
        !data->input_w || !data->input_h ||
        !data->crop_left || !data->crop_right || !data->crop_top || !data->crop_bottom ||
        !data->coords) {
        set_error(error_msg, "Failed to allocate memory for input arrays");
        goto error;
    }

    /* Parse each input */
    for (int i = 0; i < data->num_inputs; i++) {
        cJSON *input = cJSON_GetArrayItem(inputs, i);
        if (!input || !cJSON_is_object(input)) {
            set_error(error_msg, "Invalid input at index %d", i);
            goto error;
        }

        /* Parse input index */
        cJSON *index = cJSON_GetObjectItemCaseSensitive(input, "index");
        if (!cJSON_is_number(index) || (int)index->number_value != i) {
            set_error(error_msg, "Invalid or missing index for input %d", i);
            goto error;
        }

        /* Parse dimensions */
        cJSON *width = cJSON_GetObjectItemCaseSensitive(input, "width");
        cJSON *height = cJSON_GetObjectItemCaseSensitive(input, "height");
        if (!cJSON_is_number(width) || !cJSON_is_number(height)) {
            set_error(error_msg, "Missing dimensions for input %d", i);
            goto error;
        }
        data->input_w[i] = (int32_t)width->number_value;
        data->input_h[i] = (int32_t)height->number_value;

        /* Parse crop */
        cJSON *crop = cJSON_GetObjectItemCaseSensitive(input, "crop");
        if (crop && cJSON_is_object(crop)) {
            cJSON *crop_left = cJSON_GetObjectItemCaseSensitive(crop, "left");
            cJSON *crop_right = cJSON_GetObjectItemCaseSensitive(crop, "right");
            cJSON *crop_top = cJSON_GetObjectItemCaseSensitive(crop, "top");
            cJSON *crop_bottom = cJSON_GetObjectItemCaseSensitive(crop, "bottom");
            data->crop_left[i] = crop_left && cJSON_is_number(crop_left) ? (int32_t)crop_left->number_value : 0;
            data->crop_right[i] = crop_right && cJSON_is_number(crop_right) ? (int32_t)crop_right->number_value : 0;
            data->crop_top[i] = crop_top && cJSON_is_number(crop_top) ? (int32_t)crop_top->number_value : 0;
            data->crop_bottom[i] = crop_bottom && cJSON_is_number(crop_bottom) ? (int32_t)crop_bottom->number_value : 0;
        }

        /* Parse warp region */
        cJSON *warp_region = cJSON_GetObjectItemCaseSensitive(input, "warp_region");
        if (!warp_region || !cJSON_is_object(warp_region)) {
            set_error(error_msg, "Missing warp_region for input %d", i);
            goto error;
        }
        cJSON *w_x = cJSON_GetObjectItemCaseSensitive(warp_region, "x");
        cJSON *w_y = cJSON_GetObjectItemCaseSensitive(warp_region, "y");
        cJSON *w_w = cJSON_GetObjectItemCaseSensitive(warp_region, "width");
        cJSON *w_h = cJSON_GetObjectItemCaseSensitive(warp_region, "height");
        if (!cJSON_is_number(w_x) || !cJSON_is_number(w_y) ||
            !cJSON_is_number(w_w) || !cJSON_is_number(w_h)) {
            set_error(error_msg, "Missing warp region dimensions for input %d", i);
            goto error;
        }
        data->warp_x[i] = (int32_t)w_x->number_value;
        data->warp_y[i] = (int32_t)w_y->number_value;
        data->warp_w[i] = (int32_t)w_w->number_value;
        data->warp_h[i] = (int32_t)w_h->number_value;

        /* Load coordinate data file */
        cJSON *coord_data = cJSON_GetObjectItemCaseSensitive(input, "coordinate_data");
        if (!cJSON_is_string(coord_data)) {
            set_error(error_msg, "Missing coordinate_data filename for input %d", i);
            goto error;
        }

        snprintf(coords_path, sizeof(coords_path), "%s/%s", path, coord_data->string_value);

        /* Read binary coordinate file */
        FILE *f = fopen(coords_path, "rb");
        if (!f) {
            set_error(error_msg, "Failed to open coordinate file: %s", coords_path);
            goto error;
        }

        size_t num_coords = data->warp_w[i] * data->warp_h[i];
        data->coords[i] = (ValidatedCoordinate *)malloc(num_coords * sizeof(ValidatedCoordinate));
        if (!data->coords[i]) {
            fclose(f);
            set_error(error_msg, "Failed to allocate memory for coordinates (input %d)", i);
            goto error;
        }

        size_t read = fread(data->coords[i], sizeof(ValidatedCoordinate), num_coords, f);
        fclose(f);

        if (read != num_coords) {
            set_error(error_msg, "Incomplete coordinate file: %s (expected %zu, got %zu)",
                     coords_path, num_coords, read);
            goto error;
        }
    }

    /* Store JSON metadata */
    data->metadata_json = json_content;
    json_content = NULL;  /* Don't free below */

    cJSON_Delete(json);
    return data;

error:
    if (json_content) free(json_content);
    if (json) cJSON_Delete(json);
    if (data) map_file_free_v2(data);
    return NULL;
}

void map_file_free_v2(MapFileDataV2 *data) {
    if (!data) return;

    if (data->metadata_json) free(data->metadata_json);

    if (data->coords) {
        for (int i = 0; i < data->num_inputs; i++) {
            if (data->coords[i]) free(data->coords[i]);
        }
        free(data->coords);
    }

    free(data->warp_x);
    free(data->warp_y);
    free(data->warp_w);
    free(data->warp_h);
    free(data->input_w);
    free(data->input_h);
    free(data->crop_left);
    free(data->crop_right);
    free(data->crop_top);
    free(data->crop_bottom);

    free(data);
}

int map_file_validate_v2(const MapFileDataV2 *data, char **error_msg) {
    if (!data) {
        set_error(error_msg, "NULL data");
        return 0;
    }

    if (data->canvas_w <= 0 || data->canvas_h <= 0) {
        set_error(error_msg, "Invalid canvas dimensions: %dx%d",
                 data->canvas_w, data->canvas_h);
        return 0;
    }

    if (data->num_inputs <= 0 || data->num_inputs > STITCHER_MAX_INPUTS) {
        set_error(error_msg, "Invalid number of inputs: %d", data->num_inputs);
        return 0;
    }

    for (int i = 0; i < data->num_inputs; i++) {
        if (data->input_w[i] <= 0 || data->input_h[i] <= 0) {
            set_error(error_msg, "Invalid input dimensions for image %d: %dx%d",
                     i, data->input_w[i], data->input_h[i]);
            return 0;
        }

        if (data->warp_w[i] <= 0 || data->warp_h[i] <= 0) {
            set_error(error_msg, "Invalid warp region for image %d: %dx%d",
                     i, data->warp_w[i], data->warp_h[i]);
            return 0;
        }

        if (!data->coords[i]) {
            set_error(error_msg, "Missing coordinate data for image %d", i);
            return 0;
        }

        /* Validate warp region is within canvas */
        if (data->warp_x[i] < 0 || data->warp_y[i] < 0 ||
            data->warp_x[i] + data->warp_w[i] > data->canvas_w ||
            data->warp_y[i] + data->warp_h[i] > data->canvas_h) {
            set_error(error_msg, "Warp region out of bounds for image %d", i);
            return 0;
        }
    }

    return 1;
}

const ValidatedCoordinate *map_file_get_coord_v2(const MapFileDataV2 *data,
                                                   int image_index,
                                                   int canvas_x, int canvas_y) {
    if (!data || image_index < 0 || image_index >= data->num_inputs)
        return NULL;

    /* Check if within warp region */
    int warp_x = data->warp_x[image_index];
    int warp_y = data->warp_y[image_index];
    int warp_w = data->warp_w[image_index];
    int warp_h = data->warp_h[image_index];

    if (canvas_x < warp_x || canvas_x >= warp_x + warp_w ||
        canvas_y < warp_y || canvas_y >= warp_y + warp_h)
        return NULL;

    /* Calculate index */
    int local_x = canvas_x - warp_x;
    int local_y = canvas_y - warp_y;
    int idx = local_y * warp_w + local_x;

    return &data->coords[image_index][idx];
}

void map_file_print_info_v2(const MapFileDataV2 *data, FILE *fp) {
    if (!data) {
        fprintf(fp, "NULL data\n");
        return;
    }

    fprintf(fp, "=== Map File v2 ===\n");
    fprintf(fp, "Canvas: %dx%d\n", data->canvas_w, data->canvas_h);
    fprintf(fp, "Inputs: %d\n", data->num_inputs);
    fprintf(fp, "\n");

    for (int i = 0; i < data->num_inputs; i++) {
        fprintf(fp, "Input %d:\n", i);
        fprintf(fp, "  Source: %dx%d\n", data->input_w[i], data->input_h[i]);
        fprintf(fp, "  Crop: L=%d R=%d T=%d B=%d\n",
               data->crop_left[i], data->crop_right[i],
               data->crop_top[i], data->crop_bottom[i]);
        fprintf(fp, "  Warp region: x=%d, y=%d, w=%d, h=%d\n",
               data->warp_x[i], data->warp_y[i],
               data->warp_w[i], data->warp_h[i]);
        fprintf(fp, "  Warp pixels: %d\n", data->warp_w[i] * data->warp_h[i]);

        /* Check first and last coordinates */
        if (data->coords[i]) {
            ValidatedCoordinate *first = &data->coords[i][0];
            ValidatedCoordinate *last = &data->coords[i][data->warp_w[i] * data->warp_h[i] - 1];

            float fx0 = first->fx / 255.0f;
            float fy0 = first->fy / 255.0f;
            float fx1 = last->fx / 255.0f;
            float fy1 = last->fy / 255.0f;

            fprintf(fp, "  First coord: x0=%u, y0=%u, fx=%.3f, fy=%.3f, flags=0x%02x\n",
                   first->x0, first->y0, fx0, fy0, first->flags);
            fprintf(fp, "  Last coord: x0=%u, y0=%u, fx=%.3f, fy=%.3f, flags=0x%02x\n",
                   last->x0, last->y0, fx1, fy1, last->flags);
        }
        fprintf(fp, "\n");
    }
}
