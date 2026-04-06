#include <stdio.h>
#include "gststitcher.h"
#include "map_loader.h"

GST_DEBUG_CATEGORY(gst_stitcher_debug);
#define GST_CAT_DEFAULT gst_stitcher_debug

enum {
    PROP_0,
    PROP_HOMOGRAPHY_LIST,
    PROP_HOMOGRAPHY_FILE,
    PROP_MAP_FILE,
    PROP_BORDER_WIDTH,
    PROP_OUTPUT_WIDTH,
    PROP_OUTPUT_HEIGHT,
};

#define DEFAULT_BORDER_WIDTH 0
#define DEFAULT_OUTPUT_DIM   -1

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw, format=RGBA, "
        "width=[1,2147483647], height=[1,2147483647], "
        "framerate=[0/1,2147483647/1]"));

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink_%u",
    GST_PAD_SINK, GST_PAD_REQUEST,
    GST_STATIC_CAPS("video/x-raw, format=RGBA, "
        "width=[1,2147483647], height=[1,2147483647], "
        "framerate=[0/1,2147483647/1]"));

G_DEFINE_TYPE_WITH_CODE(GstStitcher, gst_stitcher, GST_TYPE_AGGREGATOR,
    GST_DEBUG_CATEGORY_INIT(gst_stitcher_debug, "stitcher", 0, "Stitcher"));

static void gst_stitcher_finalize(GObject *object)
{
    GstStitcher *self = GST_STITCHER(object);

    g_free(self->homography_list);
    g_free(self->homography_file);
    g_free(self->map_file);

    if (self->config)
        stitcher_config_free(self->config);
    if (self->canvas_info)
        canvas_free(self->canvas_info);
    if (self->overlaps)
        blender_free_overlaps(self->overlaps);
    if (self->map_data)
        map_file_free_v2(self->map_data);
    if (self->backend && self->backend_ctx && self->backend->cleanup)
        self->backend->cleanup(self->backend_ctx);

    G_OBJECT_CLASS(gst_stitcher_parent_class)->finalize(object);
}

static void gst_stitcher_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec)
{
    GstStitcher *self = GST_STITCHER(object);
    switch (prop_id) {
    case PROP_HOMOGRAPHY_LIST: g_value_set_string(value, self->homography_list); break;
    case PROP_HOMOGRAPHY_FILE: g_value_set_string(value, self->homography_file); break;
    case PROP_MAP_FILE:        g_value_set_string(value, self->map_file);        break;
    case PROP_BORDER_WIDTH:    g_value_set_int(value, self->border_width);       break;
    case PROP_OUTPUT_WIDTH:    g_value_set_int(value, self->output_width);        break;
    case PROP_OUTPUT_HEIGHT:   g_value_set_int(value, self->output_height);       break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);           break;
    }
}

static void gst_stitcher_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec)
{
    GstStitcher *self = GST_STITCHER(object);
    switch (prop_id) {
    case PROP_HOMOGRAPHY_LIST:
        g_free(self->homography_list);
        self->homography_list = g_value_dup_string(value);
        break;
    case PROP_HOMOGRAPHY_FILE:
        g_free(self->homography_file);
        self->homography_file = g_value_dup_string(value);
        break;
    case PROP_MAP_FILE:
        g_free(self->map_file);
        self->map_file = g_value_dup_string(value);
        break;
    case PROP_BORDER_WIDTH: self->border_width  = g_value_get_int(value); break;
    case PROP_OUTPUT_WIDTH: self->output_width  = g_value_get_int(value); break;
    case PROP_OUTPUT_HEIGHT:self->output_height = g_value_get_int(value); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);    break;
    }
}

static GstPad *gst_stitcher_request_new_pad(GstElement *element,
    GstPadTemplate *templ, const gchar *name, const GstCaps *caps)
{
    GstPad *pad = GST_PAD(
        g_object_new(GST_TYPE_STITCHER_PAD,
            "name", name,
            "direction", templ->direction,
            "template", templ,
            NULL));

    if (!gst_element_add_pad(element, pad))
        return NULL;

    return pad;
}

static void gst_stitcher_release_pad(GstElement *element, GstPad *pad)
{
    gst_element_remove_pad(element, pad);
}

static gboolean gst_stitcher_do_init(GstStitcher *self)
{
    GError *error = NULL;
    char *map_error = NULL;

    /* Determine which mode to use */
    self->use_map_mode = (self->map_file != NULL);

    if (self->use_map_mode) {
        /* Map file mode */

        /* Load map file v2 */
        self->map_data = map_file_load_v2(self->map_file, &map_error);
        if (!self->map_data) {
            GST_ELEMENT_ERROR(self, LIBRARY, SETTINGS,
                ("Failed to load map file: %s", map_error ? map_error : "unknown"),
                ("Map file load error"));
            if (map_error) free(map_error);
            return FALSE;
        }

        /* Validate map file */
        if (!map_file_validate_v2(self->map_data, &map_error)) {
            GST_ELEMENT_ERROR(self, LIBRARY, SETTINGS,
                ("Map file validation failed: %s", map_error ? map_error : "unknown"),
                ("Map file validation error"));
            map_file_free_v2(self->map_data);
            self->map_data = NULL;
            if (map_error) free(map_error);
            return FALSE;
        }

        /* Collect input sizes from pad caps */
        GstIterator *it = gst_element_iterate_sink_pads(GST_ELEMENT(self));
        int pad_count = 0;
        gboolean done = FALSE;
        gint fps_n = 30, fps_d = 1;

        while (!done) {
            GValue item = G_VALUE_INIT;
            switch (gst_iterator_next(it, &item)) {
            case GST_ITERATOR_OK: {
                GstPad *pad = g_value_get_object(&item);
                GstCaps *caps = gst_pad_get_current_caps(pad);
                if (caps) {
                    if (pad_count == 0) {
                        GstStructure *s = gst_caps_get_structure(caps, 0);
                        gst_structure_get_fraction(s, "framerate", &fps_n, &fps_d);
                    }
                    pad_count++;
                    gst_caps_unref(caps);
                }
                g_value_unset(&item);
                break;
            }
            case GST_ITERATOR_DONE: done = TRUE; break;
            default: done = TRUE; break;
            }
        }
        gst_iterator_free(it);

        if (pad_count != self->map_data->num_inputs) {
            GST_ELEMENT_ERROR(self, STREAM, FORMAT,
                ("Pad count (%d) does not match map file inputs (%d)",
                 pad_count, self->map_data->num_inputs),
                ("Configuration mismatch"));
            map_file_free_v2(self->map_data);
            self->map_data = NULL;
            return FALSE;
        }

        /* Create canvas_info from map file data */
        self->canvas_info = (CanvasInfo *)g_malloc0(sizeof(CanvasInfo));
        self->canvas_info->canvas_w = self->map_data->canvas_w;
        self->canvas_info->canvas_h = self->map_data->canvas_h;
        self->canvas_info->num_images = self->map_data->num_inputs;

        for (int i = 0; i < self->canvas_info->num_images; i++) {
            self->canvas_info->images[i].offset_x = 0;  /* Map coordinates are absolute */
            self->canvas_info->images[i].offset_y = 0;
            self->canvas_info->images[i].warp_x = self->map_data->warp_x[i];
            self->canvas_info->images[i].warp_y = self->map_data->warp_y[i];
            self->canvas_info->images[i].warp_w = self->map_data->warp_w[i];
            self->canvas_info->images[i].warp_h = self->map_data->warp_h[i];
        }

        /* Allow user override of output dimensions */
        if (self->output_width > 0)
            self->canvas_info->canvas_w = self->output_width;
        if (self->output_height > 0)
            self->canvas_info->canvas_h = self->output_height;

        /* Initialize map backend */
        self->backend = stitcher_backend_map_get();
        self->backend_ctx = self->backend->init(
            self->canvas_info->canvas_w, self->canvas_info->canvas_h);

        /* Set map data to backend */
        map_backend_set_map_data_v2(self->backend_ctx, self->map_data);

        /* Detect overlap regions for blending */
        self->overlaps = blender_detect_overlaps(self->canvas_info);

        /* Set src pad caps */
        GstCaps *src_caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "RGBA",
            "width", G_TYPE_INT, self->canvas_info->canvas_w,
            "height", G_TYPE_INT, self->canvas_info->canvas_h,
            "framerate", GST_TYPE_FRACTION, fps_n, fps_d,
            NULL);
        gst_aggregator_set_src_caps(GST_AGGREGATOR(self), src_caps);
        gst_caps_unref(src_caps);

        self->initialized = TRUE;
        return TRUE;
    }

    /* Homography mode (original logic) */
    if (self->homography_file) {
        self->config = stitcher_config_parse_file(self->homography_file, &error);
    } else if (self->homography_list) {
        self->config = stitcher_config_parse_json(
            self->homography_list, strlen(self->homography_list), &error);
    } else {
        GST_ELEMENT_ERROR(self, LIBRARY, SETTINGS,
            ("No configuration provided"),
            ("Set map-file, homography-list, or homography-file property"));
        return FALSE;
    }

    if (!self->config) {
        GST_ELEMENT_ERROR(self, LIBRARY, SETTINGS,
            ("Failed to parse homography: %s", error ? error->message : "unknown"),
            ("Homography parse error"));
        if (error) g_error_free(error);
        return FALSE;
    }

    /* Collect input sizes from pad caps */
    GstIterator *it = gst_element_iterate_sink_pads(GST_ELEMENT(self));
    ImageSize sizes[STITCHER_MAX_INPUTS];
    StitcherCrop crops[STITCHER_MAX_INPUTS];
    int pad_count = 0;
    gboolean done = FALSE;
    gint fps_n = 30, fps_d = 1; /* default framerate, overridden by first pad */

    while (!done) {
        GValue item = G_VALUE_INIT;
        switch (gst_iterator_next(it, &item)) {
        case GST_ITERATOR_OK: {
            GstPad *pad = g_value_get_object(&item);
            GstCaps *caps = gst_pad_get_current_caps(pad);
            if (caps) {
                GstStructure *s = gst_caps_get_structure(caps, 0);
                sizes[pad_count].w = 0;
                sizes[pad_count].h = 0;
                gst_structure_get_int(s, "width", &sizes[pad_count].w);
                gst_structure_get_int(s, "height", &sizes[pad_count].h);

                GstStitcherPad *spad = GST_STITCHER_PAD(pad);
                crops[pad_count].left   = spad->crop_left;
                crops[pad_count].right  = spad->crop_right;
                crops[pad_count].top    = spad->crop_top;
                crops[pad_count].bottom = spad->crop_bottom;

                /* Use first pad's framerate for output */
                if (pad_count == 0)
                    gst_structure_get_fraction(s, "framerate", &fps_n, &fps_d);

                pad_count++;
                gst_caps_unref(caps);
            }
            g_value_unset(&item);
            break;
        }
        case GST_ITERATOR_DONE: done = TRUE; break;
        default: done = TRUE; break;
        }
    }
    gst_iterator_free(it);

    if (pad_count != self->config->num_inputs) {
        GST_ELEMENT_ERROR(self, STREAM, FORMAT,
            ("Pad count (%d) does not match homography inputs (%d)",
             pad_count, self->config->num_inputs),
            ("Configuration mismatch"));
        return FALSE;
    }

    /* Compute canvas geometry */
    self->canvas_info = canvas_compute(self->config, sizes, crops);
    if (!self->canvas_info) {
        GST_ELEMENT_ERROR(self, LIBRARY, SETTINGS,
            ("Canvas computation failed"), ("Geometry error"));
        return FALSE;
    }

    /* Allow user override of output dimensions */
    if (self->output_width > 0)
        self->canvas_info->canvas_w = self->output_width;
    if (self->output_height > 0)
        self->canvas_info->canvas_h = self->output_height;

    /* Detect overlap regions */
    self->overlaps = blender_detect_overlaps(self->canvas_info);

    /* Initialize backend */
    self->backend = stitcher_backend_cpu_get();
    self->backend_ctx = self->backend->init(
        self->canvas_info->canvas_w, self->canvas_info->canvas_h);

    /* Set src pad caps */
    GstCaps *src_caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "RGBA",
        "width", G_TYPE_INT, self->canvas_info->canvas_w,
        "height", G_TYPE_INT, self->canvas_info->canvas_h,
        "framerate", GST_TYPE_FRACTION, fps_n, fps_d,
        NULL);
    gst_aggregator_set_src_caps(GST_AGGREGATOR(self), src_caps);
    gst_caps_unref(src_caps);

    self->initialized = TRUE;
    return TRUE;
}

static GstFlowReturn gst_stitcher_aggregate(GstAggregator *aggregator,
    gboolean timeout)
{
    GstStitcher *self = GST_STITCHER(aggregator);
    (void)timeout;

    /* Check if we have buffers on all pads, and detect EOS */
    GstIterator *check_it = gst_element_iterate_sink_pads(GST_ELEMENT(self));
    gboolean all_have_buffers = TRUE;
    gboolean any_eos = FALSE;
    gboolean check_done = FALSE;
    while (!check_done) {
        GValue item = G_VALUE_INIT;
        switch (gst_iterator_next(check_it, &item)) {
        case GST_ITERATOR_OK: {
            GstPad *pad = GST_PAD(g_value_get_object(&item));
            GstAggregatorPad *apad = GST_AGGREGATOR_PAD(pad);

            /* Check if pad has buffer */
            if (!gst_aggregator_pad_peek_buffer(apad)) {
                /* Check if pad is EOS by checking if it's flushed */
                GstObject *parent = gst_object_get_parent(GST_OBJECT(pad));
                if (!parent) {
                    /* Pad has been removed - treat as EOS */
                    any_eos = TRUE;
                }
                if (parent) gst_object_unref(parent);

                all_have_buffers = FALSE;

                /* If we're initialized but one pad has no buffer,
                 * it likely means EOS on that pad */
                if (self->initialized) {
                    any_eos = TRUE;
                }
                check_done = TRUE;
            }
            g_value_unset(&item);
            break;
        }
        case GST_ITERATOR_DONE:
        default:
            check_done = TRUE;
            break;
        }
    }
    gst_iterator_free(check_it);

    /* If we detected EOS on any pad, propagate EOS downstream */
    if (any_eos && self->initialized) {
        return GST_FLOW_EOS;
    }

    if (!all_have_buffers) {
        return GST_FLOW_OK;
    }

    /* Lazy initialization on first call */
    if (!self->initialized) {
        if (!gst_stitcher_do_init(self))
            return GST_FLOW_ERROR;
    }

    int cw = self->canvas_info->canvas_w;
    int ch = self->canvas_info->canvas_h;
    int canvas_stride = cw * 4;

    /* Allocate output buffer */
    GstBuffer *outbuf = gst_buffer_new_allocate(NULL, canvas_stride * ch, NULL);
    if (!outbuf)
        return GST_FLOW_ERROR;

    GstMapInfo outmap;
    gst_buffer_map(outbuf, &outmap, GST_MAP_WRITE);
    memset(outmap.data, 0, outmap.size);

    /* Collect input buffers indexed by pad number */
    GstBuffer *inbufs[STITCHER_MAX_INPUTS] = {NULL};
    GstMapInfo inmaps[STITCHER_MAX_INPUTS];
    gboolean mapped[STITCHER_MAX_INPUTS] = {FALSE};
    int pad_crops[STITCHER_MAX_INPUTS][4]; /* cl, cr, ct, cb */
    int pad_dims[STITCHER_MAX_INPUTS][2];  /* w, h */
    int max_idx = 0;
    GstClockTime max_timestamp = GST_CLOCK_TIME_NONE;

    GstIterator *it = gst_element_iterate_sink_pads(GST_ELEMENT(self));
    gboolean done = FALSE;
    while (!done) {
        GValue item = G_VALUE_INIT;
        switch (gst_iterator_next(it, &item)) {
        case GST_ITERATOR_OK: {
            GstPad *pad = GST_PAD(g_value_get_object(&item));
            /* Parse pad name "sink_N" to get index */
            const char *name = GST_PAD_NAME(pad);
            int pad_idx = -1;
            if (sscanf(name, "sink_%d", &pad_idx) == 1 && pad_idx >= 0 && pad_idx < STITCHER_MAX_INPUTS) {
                GstAggregatorPad *apad = GST_AGGREGATOR_PAD(pad);
                inbufs[pad_idx] = gst_aggregator_pad_pop_buffer(apad);
                if (inbufs[pad_idx]) {
                    gst_buffer_map(inbufs[pad_idx], &inmaps[pad_idx], GST_MAP_READ);
                    mapped[pad_idx] = TRUE;
                    /* Track maximum timestamp for output buffer */
                    GstClockTime ts = GST_BUFFER_PTS(inbufs[pad_idx]);
                    if (GST_CLOCK_TIME_IS_VALID(ts) &&
                        (!GST_CLOCK_TIME_IS_VALID(max_timestamp) || ts > max_timestamp)) {
                        max_timestamp = ts;
                    }
                }
                GstStitcherPad *spad = GST_STITCHER_PAD(pad);
                pad_crops[pad_idx][0] = spad->crop_left;
                pad_crops[pad_idx][1] = spad->crop_right;
                pad_crops[pad_idx][2] = spad->crop_top;
                pad_crops[pad_idx][3] = spad->crop_bottom;

                GstCaps *caps = gst_pad_get_current_caps(pad);
                if (caps) {
                    GstStructure *s = gst_caps_get_structure(caps, 0);
                    pad_dims[pad_idx][0] = 0;
                    pad_dims[pad_idx][1] = 0;
                    gst_structure_get_int(s, "width", &pad_dims[pad_idx][0]);
                    gst_structure_get_int(s, "height", &pad_dims[pad_idx][1]);
                    gst_caps_unref(caps);
                }
                if (pad_idx >= max_idx) max_idx = pad_idx + 1;
            }
            g_value_unset(&item);
            break;
        }
        case GST_ITERATOR_DONE: done = TRUE; break;
        default: done = TRUE; break;
        }
    }
    gst_iterator_free(it);

    /* Global canvas offset (baked into effective H_inv) */
    int off_x = self->canvas_info->images[0].offset_x;
    int off_y = self->canvas_info->images[0].offset_y;

    /* Warp each image to canvas */
    uint8_t *saved_overlap = NULL;
    for (int i = 0; i < max_idx; i++) {
        if (!mapped[i]) continue;

        int cl = pad_crops[i][0], cr = pad_crops[i][1];
        int ct = pad_crops[i][2], cb = pad_crops[i][3];
        int src_w = pad_dims[i][0], src_h = pad_dims[i][1];
        int eff_w = src_w - cl - cr;
        int eff_h = src_h - ct - cb;

        if (eff_w <= 0 || eff_h <= 0) continue;

        /* Adjust source pointer for crop */
        const uint8_t *src_data = inmaps[i].data + ct * src_w * 4 + cl * 4;
        int src_stride = src_w * 4;

        /* Save overlap region before warping image 1 (2-input MVP) */
        if (i == 1 && self->overlaps && self->overlaps->num_regions > 0 && self->border_width > 0) {
            StitcherRect *r = &self->overlaps->regions[0];
            saved_overlap = blender_save_region(outmap.data, canvas_stride, r);
        }

        if (self->use_map_mode) {
            /* Map mode: set image index and warp without homography */
            map_backend_set_image_index(self->backend_ctx, i);

            /* In map mode, the warp function should iterate over the warp region dimensions,
             * not the source image dimensions. The map contains pre-computed transformations
             * for the entire warp region, which may be larger or smaller than the source. */
            int map_warp_w = self->canvas_info->images[i].warp_w;
            int map_warp_h = self->canvas_info->images[i].warp_h;

            self->backend->warp(self->backend_ctx,
                src_data, eff_w, eff_h, src_stride,
                outmap.data, canvas_stride,
                self->canvas_info->images[i].warp_x,
                self->canvas_info->images[i].warp_y,
                map_warp_w, map_warp_h,  /* Use map warp region dimensions, not source dimensions */
                NULL, 0, 0);  /* h_eff ignored in map mode */
        } else {
            /* Homography mode: compute homography matrix */
            /* Build effective H_inv that maps canvas coords → source coords:
               H_eff = H_inv * T(-off_x, -off_y)
               For canvas pixel (cx, cy), H_eff * (cx,cy,1) = H_inv * (cx-off_x, cy-off_y, 1) */
            float h_eff[3][3];

            if (i == self->config->reference_index) {
                /* Reference: H_inv = identity, so H_eff = T(-off_x, -off_y) */
                h_eff[0][0] = 1.0f; h_eff[0][1] = 0.0f; h_eff[0][2] = -(float)off_x;
                h_eff[1][0] = 0.0f; h_eff[1][1] = 1.0f; h_eff[1][2] = -(float)off_y;
                h_eff[2][0] = 0.0f; h_eff[2][1] = 0.0f; h_eff[2][2] = 1.0f;
            } else {
                /* Target: find H_inv and compose with translation */
                const float (*h_inv)[3] = NULL;
                for (int p = 0; p < self->config->num_inputs - 1; p++) {
                    if (self->config->pairs[p].target == i) {
                        h_inv = (const float (*)[3])self->config->pairs[p].homography_inv.h;
                        break;
                    }
                }
                if (!h_inv) continue;

                /* H_eff = H_inv * T(-off_x, -off_y) */
                for (int r = 0; r < 3; r++) {
                    h_eff[r][0] = h_inv[r][0];
                    h_eff[r][1] = h_inv[r][1];
                    h_eff[r][2] = h_inv[r][0] * (-(float)off_x)
                                + h_inv[r][1] * (-(float)off_y)
                                + h_inv[r][2];
                }
            }

            self->backend->warp(self->backend_ctx,
                src_data, eff_w, eff_h, src_stride,
                outmap.data, canvas_stride,
                self->canvas_info->images[i].warp_x,
                self->canvas_info->images[i].warp_y,
                self->canvas_info->images[i].warp_w,
                self->canvas_info->images[i].warp_h,
                h_eff, 0, 0);
        }
    }

    /* Apply blending in overlap regions */
    if (saved_overlap && self->overlaps && self->overlaps->num_regions > 0) {
        StitcherRect *r = &self->overlaps->regions[0];
        blender_apply(self->backend, self->backend_ctx,
            outmap.data, canvas_stride,
            saved_overlap, r->w * 4,
            r, self->border_width);
        blender_free_saved(saved_overlap);
    }

    /* Unmap all input buffers */
    for (int i = 0; i < max_idx; i++) {
        if (mapped[i]) {
            gst_buffer_unmap(inbufs[i], &inmaps[i]);
            gst_buffer_unref(inbufs[i]);
        }
    }

    gst_buffer_unmap(outbuf, &outmap);

    /* Set timestamp and duration on output buffer */
    GstFlowReturn ret;
    if (GST_CLOCK_TIME_IS_VALID(max_timestamp)) {
        GST_BUFFER_PTS(outbuf) = max_timestamp;
        /* Use default duration or could compute from framerate */
        /* For now, use a reasonable default for 30fps */
        GST_BUFFER_DURATION(outbuf) = GST_SECOND / 30;
    } else {
        GST_WARNING_OBJECT(self, "No valid timestamp found on input buffers");
        GST_BUFFER_PTS(outbuf) = 0;
        GST_BUFFER_DURATION(outbuf) = GST_SECOND / 30;
    }

    ret = gst_aggregator_finish_buffer(aggregator, outbuf);
    return ret;
}

static gboolean gst_stitcher_start(GstAggregator *aggregator)
{
    GstStitcher *self = GST_STITCHER(aggregator);

    /* Reset state for new stream */
    self->initialized = FALSE;
    self->use_map_mode = FALSE;
    self->config = NULL;
    self->canvas_info = NULL;
    self->overlaps = NULL;
    self->backend_ctx = NULL;
    self->map_data = NULL;

    return TRUE;
}

static gboolean gst_stitcher_stop(GstAggregator *aggregator)
{
    GstStitcher *self = GST_STITCHER(aggregator);

    if (self->backend && self->backend_ctx && self->backend->cleanup) {
        self->backend->cleanup(self->backend_ctx);
        self->backend_ctx = NULL;
    }

    g_clear_pointer(&self->config, stitcher_config_free);
    /* In map mode, canvas_info was allocated with g_malloc0, not canvas_compute */
    if (self->canvas_info) {
        if (self->use_map_mode) {
            g_free(self->canvas_info);
        } else {
            canvas_free(self->canvas_info);
        }
        self->canvas_info = NULL;
    }
    g_clear_pointer(&self->overlaps, blender_free_overlaps);
    g_clear_pointer(&self->map_data, map_file_free_v2);
    self->initialized = FALSE;
    self->use_map_mode = FALSE;

    return TRUE;
}

static void gst_stitcher_class_init(GstStitcherClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
    GstAggregatorClass *agg_class = GST_AGGREGATOR_CLASS(klass);

    gobject_class->finalize = gst_stitcher_finalize;
    gobject_class->get_property = gst_stitcher_get_property;
    gobject_class->set_property = gst_stitcher_set_property;

    /* Set element metadata */
    gst_element_class_set_metadata(gstelement_class,
        "gststitcher",
        "Generic/Video",
        "Video stitching element",
        "qualityguo");

    gstelement_class->request_new_pad = gst_stitcher_request_new_pad;
    gstelement_class->release_pad = gst_stitcher_release_pad;

    agg_class->start = gst_stitcher_start;
    agg_class->aggregate = gst_stitcher_aggregate;
    agg_class->stop = gst_stitcher_stop;

    g_object_class_install_property(gobject_class, PROP_HOMOGRAPHY_LIST,
        g_param_spec_string("homography-list", "Homography List",
            "Inline JSON homography configuration",
            NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_HOMOGRAPHY_FILE,
        g_param_spec_string("homography-file", "Homography File",
            "Path to JSON homography configuration file",
            NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_MAP_FILE,
        g_param_spec_string("map-file", "Map File",
            "Path to pre-computed map file",
            NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_BORDER_WIDTH,
        g_param_spec_int("border-width", "Border Width",
            "Blend region width in pixels",
            0, G_MAXINT, DEFAULT_BORDER_WIDTH,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_OUTPUT_WIDTH,
        g_param_spec_int("output-width", "Output Width",
            "Override output width (-1 = auto)",
            -1, G_MAXINT, DEFAULT_OUTPUT_DIM,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_OUTPUT_HEIGHT,
        g_param_spec_int("output-height", "Output Height",
            "Override output height (-1 = auto)",
            -1, G_MAXINT, DEFAULT_OUTPUT_DIM,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    gst_element_class_add_static_pad_template(gstelement_class, &src_template);
    gst_element_class_add_static_pad_template(gstelement_class, &sink_template);

    gst_element_class_set_static_metadata(gstelement_class,
        "Stitcher", "Video/Effect",
        "Stitch multiple video streams into a panoramic output",
        "gst-stitcher project");
}

static void gst_stitcher_init(GstStitcher *self)
{
    self->homography_list = NULL;
    self->homography_file = NULL;
    self->map_file = NULL;
    self->border_width = DEFAULT_BORDER_WIDTH;
    self->output_width = DEFAULT_OUTPUT_DIM;
    self->output_height = DEFAULT_OUTPUT_DIM;
    self->initialized = FALSE;
    self->use_map_mode = FALSE;
    self->config = NULL;
    self->canvas_info = NULL;
    self->overlaps = NULL;
    self->backend = NULL;
    self->backend_ctx = NULL;
    self->map_data = NULL;
}
