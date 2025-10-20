#ifndef _VISUALIZE_H_
#define _VISUALIZE_H_

#include "functions.h"


typedef struct DrawCtx {
    int window_width, window_height;
} DrawCtx;

void draw_vertex(DrawCtx *ctx, Vertex v);

#endif // _VISUALIZE_H_