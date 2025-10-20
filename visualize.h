#ifndef _VISUALIZE_H_
#define _VISUALIZE_H_

#include "functions.h"


typedef struct DrawCtx {
    int window_width, window_height;
    double min_x, min_y, max_x, max_y;
} DrawCtx;

void draw_vertex(DrawCtx *ctx, Vertex v);
void draw_edge(DrawCtx *ctx, Vertex v1, Vertex v2);

#endif // _VISUALIZE_H_