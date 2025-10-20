#include <stdio.h>

#include "visualize.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"


#define APP_DARK        (Clay_Color){  48,  52,  70, 255 }
#define APP_VDARK       (Clay_Color){  41,  44,  60, 255 }
#define APP_VVDARK      (Clay_Color){  35,  38,  52, 255 }
#define APP_PURPLE      (Clay_Color){ 202, 158, 230, 255 }
#define APP_PALE_PURPLE (Clay_Color){ 247, 224, 255, 255 }

#define ID(str) .id = CLAY_ID(str)
#define BORDER_WIDTH_ALL(width) (Clay_BorderWidth){ .left = width, .right = width, .top = width, .bottom = width }
const Clay_Sizing sizing_expand = {
    .width = CLAY_SIZING_GROW(),
    .height = CLAY_SIZING_GROW(),
};

enum {
    FONT_BODY_INDEX,
    FONT_COUNT,
};
Font fonts[FONT_COUNT];

void draw_vertex(DrawCtx *ctx, Vertex v) {
    double x = (v.x - ctx->min_x) / (ctx->max_x - ctx->min_x) * (ctx->window_width*0.9) + ctx->window_width*0.05;
    double y = (v.y - ctx->min_y) / (ctx->max_y - ctx->min_y) * ctx->window_height*0.9 + ctx->window_height*0.05;
    // Draw the vertex at (x, y)
    DrawCircle((int)x, (int)y, 5, CLAY_COLOR_TO_RAYLIB_COLOR(APP_PURPLE));
}
void draw_edge(DrawCtx *ctx, Vertex v1, Vertex v2) {
    double x1 = (v1.x - ctx->min_x) / (ctx->max_x - ctx->min_x) * (ctx->window_width*0.9) + ctx->window_width*0.05;
    double y1 = (v1.y - ctx->min_y) / (ctx->max_y - ctx->min_y) * ctx->window_height*0.9 + ctx->window_height*0.05;
    double x2 = (v2.x - ctx->min_x) / (ctx->max_x - ctx->min_x) * (ctx->window_width*0.9) + ctx->window_width*0.05;
    double y2 = (v2.y - ctx->min_y) / (ctx->max_y - ctx->min_y) * ctx->window_height*0.9 + ctx->window_height*0.05;
    // Draw the edge from (x1, y1) to (x2, y2)
    DrawLine((int)x1, (int)y1, (int)x2, (int)y2, CLAY_COLOR_TO_RAYLIB_COLOR(APP_PALE_PURPLE));
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    // Call your Delaunay triangulation function
    MyMesh mesh;
    int result = Cdelaunay(argv[1], argv[2], &mesh);

    // Find max and min coordinates for normalization
    double min_x = __DBL_MAX__, min_y = __DBL_MAX__;
    double max_x = -__DBL_MAX__, max_y = -__DBL_MAX__;
    for (int i = 0; i < mesh.num_vertices; i++) {
        if (mesh.vertices[i].x < min_x) min_x = mesh.vertices[i].x;
        if (mesh.vertices[i].x > max_x) max_x = mesh.vertices[i].x;
        if (mesh.vertices[i].y < min_y) min_y = mesh.vertices[i].y;
        if (mesh.vertices[i].y > max_y) max_y = mesh.vertices[i].y;
    }

    if (result == 0) {
        printf("Delaunay triangulation completed successfully.\n");
    } else {
        printf("Error in Delaunay triangulation.\n");
    }

    DrawCtx ctx = {
        .window_width = 800, .window_height = 600,
        .min_x = min_x, .min_y = min_y, .max_x = max_x, .max_y = max_y
    };

    const uint32_t minMemoryRequired = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(
        minMemoryRequired, malloc(minMemoryRequired)
    );
    Clay_Initialize(arena, (Clay_Dimensions){ ctx.window_width, ctx.window_height }, (Clay_ErrorHandler){ 0 });   
    
    Clay_Raylib_Initialize(ctx.window_width, ctx.window_height, "TriVisuSuper :-)", 
        FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE
    );

    fonts[FONT_BODY_INDEX] = LoadFontEx("fonts/Roboto-Regular.ttf", 40, NULL, 0);
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

    for (uint32_t i = FONT_BODY_INDEX; i < FONT_COUNT; i++) {
        SetTextureFilter(fonts[i].texture, TEXTURE_FILTER_BILINEAR);
    }

    while (!WindowShouldClose()) {
        Clay_SetPointerState(
            RAY_VECTOR2_TO_CLAY_VECTOR2(GetMousePosition()),
            IsMouseButtonDown(MOUSE_BUTTON_LEFT)
        );
        Clay_UpdateScrollContainers(
            true,
            RAY_VECTOR2_TO_CLAY_VECTOR2(GetMouseDelta()),
            GetFrameTime()
        );

        ctx.window_width = GetScreenWidth();
        ctx.window_height = GetScreenHeight();
        Clay_SetLayoutDimensions((Clay_Dimensions){ ctx.window_width, ctx.window_height });

        Clay_BeginLayout();

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        BeginDrawing();

        CLAY({
            ID("OuterContainer"),
            .layout = {
                .sizing = sizing_expand,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }) {

        }
        
        ClearBackground(CLAY_COLOR_TO_RAYLIB_COLOR(APP_DARK));
        Clay_Raylib_Render(renderCommands, fonts);

        // Draw edges
        for (int i = 0; i < mesh.num_faces; i++) {
            HalfEdge* he = &mesh.halfedges[mesh.faces[i].halfedge];
            for (int j = 0; j < 3; j++) {
                Vertex v1 = mesh.vertices[he->vertex];
                Vertex v2 = mesh.vertices[mesh.halfedges[he->next].vertex];
                if (v1.x > v2.x || v1.y > v2.y) {
                    draw_edge(&ctx, v1, v2);
                }
                if (he->twin == -1) {
                    draw_edge(&ctx, v1, v2);
                }
                he = &mesh.halfedges[he->next];
            }
        }
        // Draw vertices
        for (int i = 0; i < mesh.num_vertices+4; i++) {
            draw_vertex(&ctx, mesh.vertices[i]);
        }


        
        EndDrawing();
    }

    Clay_Raylib_Close();

    return result;
}


