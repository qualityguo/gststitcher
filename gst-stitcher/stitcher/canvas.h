#ifndef CANVAS_H
#define CANVAS_H

#include "stitcher_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int w;
    int h;
} ImageSize;

CanvasInfo *canvas_compute(const StitcherConfig *config,
    const ImageSize *sizes,
    const StitcherCrop *crops);

void canvas_free(CanvasInfo *info);

#ifdef __cplusplus
}
#endif

#endif /* CANVAS_H */
