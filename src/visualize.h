#ifndef _VISUALIZE_H_
#define _VISUALIZE_H_

#include <raylib.h>
#include <stdint.h>

#include "clay.h"
#include "functions.h"


typedef struct Button {
    Clay_String label;
    Clay_String text;
    uint8_t on;
} Button;

typedef struct DrawCtx {
    MyMesh *mesh;
    
    int window_width, window_height;
    double min_x, min_y, max_x, max_y;
    int header_height; // If clicking in this area, ignore for adding points

    int panning_x, panning_y;
    
    float zoom;

    Button voronoi_button;
} DrawCtx;

extern uint8_t _headerToggle;
extern uint8_t _newInsert;

void draw_init(DrawCtx *ctx, MyMesh *mesh);
void draw_vertex(DrawCtx *ctx, Vertex v, int size, Color color);
void draw_edge(DrawCtx *ctx, Vertex v1, Vertex v2, Color color);
void draw_handle_user(DrawCtx *ctx);

#endif // _VISUALIZE_H_