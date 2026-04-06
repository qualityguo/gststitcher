#include <gst/gst.h>
#include <stdio.h>
#include <dlfcn.h>

int main(int argc, char *argv[]) {
    GstPlugin *plugin;
    void *handle;

    gst_init(&argc, &argv);

    printf("Testing direct plugin loading...\n\n");

    /* Try loading with dlopen first */
    handle = dlopen("/home/ghq/gststitcher/gst-stitcher/build/gst/libgststitcher.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "✗ dlopen failed: %s\n", dlerror());
        return 1;
    }
    printf("✓ dlopen succeeded\n");

    /* Check symbols */
    void *desc = dlsym(handle, "gst_plugin_desc");
    if (!desc) {
        fprintf(stderr, "✗ dlsym(gst_plugin_desc) failed: %s\n", dlerror());
    } else {
        printf("✓ gst_plugin_desc symbol found at %p\n", desc);
    }

    void *reg = dlsym(handle, "gst_plugin_gststitcher_register");
    if (!reg) {
        fprintf(stderr, "✗ dlsym(gst_plugin_gststitcher_register) failed: %s\n", dlerror());
    } else {
        printf("✓ gst_plugin_gststitcher_register symbol found at %p\n", reg);
    }

    void *get_desc = dlsym(handle, "gst_plugin_gststitcher_get_desc");
    if (!get_desc) {
        fprintf(stderr, "✗ dlsym(gst_plugin_gststitcher_get_desc) failed: %s\n", dlerror());
    } else {
        printf("✓ gst_plugin_gststitcher_get_desc symbol found at %p\n", get_desc);
    }

    dlclose(handle);

    printf("\nNow trying GStreamer plugin loading...\n");

    /* Try loading via GStreamer */
    plugin = gst_plugin_load_by_name("gststitcher");
    if (!plugin) {
        fprintf(stderr, "✗ Failed to load plugin via gst_plugin_load_by_name\n");
        return 1;
    }

    printf("✓ Plugin loaded successfully!\n");
    printf("  Name: %s\n", gst_plugin_get_name(plugin));
    printf("  Description: %s\n", gst_plugin_get_description(plugin));
    printf("  Version: %s\n", gst_plugin_get_version(plugin));
    printf("  License: %s\n", gst_plugin_get_license(plugin));
    printf("  Source: %s\n", gst_plugin_get_source(plugin));

    gst_object_unref(plugin);

    return 0;
}
