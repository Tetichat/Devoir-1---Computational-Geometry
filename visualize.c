#include <stdio.h>

#include "visualize.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"


#define APP_TRANSPARENT (Clay_Color){   0,   0,   0,   0 }
#define APP_DARK        (Clay_Color){  48,  52,  70, 255 }
#define APP_VDARK       (Clay_Color){  41,  44,  60, 255 }
#define APP_VVDARK      (Clay_Color){  35,  38,  52, 255 }
#define APP_PURPLE      (Clay_Color){ 202, 158, 230, 255 }
#define APP_PALE_PURPLE (Clay_Color){ 247, 224, 255, 255 }

#define ClayColorAlpha(_col, _alpha) ((Clay_Color){ .r = (_col).r, .g = (_col).g, .b = (_col).b, .a = (_alpha) })

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

static inline void geoToViewport(DrawCtx *ctx, double x, double y, double *vpx, double *vpy) {
    double L = ctx->mesh->L + 0.05;
    
    double 
        min_x = ctx->min_x - L, // +- L to include the big rectangle, 
        min_y = ctx->min_y - L, // won't be necessary in the future.
        max_x = ctx->max_x + L,
        max_y = ctx->max_y + L
    ;

    int
        window_width = ctx->window_width,
        window_height = ctx->window_height
    ;

    double lx = (x-min_x) / (max_x-min_x);
    *vpx = (lx)   * (window_width) + (ctx->panning_x);

    double ly = (y-min_y) / (max_y-min_y);
    *vpy = (1-ly) * (window_height) + (ctx->panning_y);
}

static inline void viewportToGeo(DrawCtx *ctx, double vpx, double vpy, double *x, double *y) {
    double L = ctx->mesh->L + 0.05;
    
    double 
        min_x = ctx->min_x - L,
        min_y = ctx->min_y - L,
        max_x = ctx->max_x + L,
        max_y = ctx->max_y + L
    ;

    int
        window_width = ctx->window_width,
        window_height = ctx->window_height
    ;

    double lx = (vpx) / (window_width);
    *x = (lx)   * (max_x-min_x) + min_x;
    
    double ly = (vpy) / (window_height);
    *y = (1-ly) * (max_y-min_y) + min_y;
}

void draw_vertex(DrawCtx *ctx, Vertex v) {
    double x = v.x, y = v.y;
    geoToViewport(ctx, x, y, &x, &y);

    // Draw the vertex at (x, y)
    DrawCircle((int)x, (int)y, 5, CLAY_COLOR_TO_RAYLIB_COLOR(APP_PURPLE));
}

void draw_edge(DrawCtx *ctx, Vertex v1, Vertex v2) {    
    double 
    x1 = v1.x, y1 = v1.y,
    x2 = v2.x, y2 = v2.y
    ;
    geoToViewport(ctx, x1, y1, &x1, &y1);
    geoToViewport(ctx, x2, y2, &x2, &y2);
    
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

    // Add a maximum of 100 extra vertices for user clicks
    int max_additional_vertices = 100;
    mesh.max_vertices += max_additional_vertices;
    mesh.vertices = (Vertex*)realloc(mesh.vertices, mesh.max_vertices * sizeof(Vertex));
    mesh.max_halfedges += max_additional_vertices * 3; // Each new vertex can create up to 3
    mesh.halfedges = (HalfEdge*)realloc(mesh.halfedges, mesh.max_halfedges * sizeof(HalfEdge));
    mesh.max_faces += max_additional_vertices; // Each new vertex can create up to 1 new
    mesh.faces = (Face*)realloc(mesh.faces, mesh.max_faces * sizeof(Face));

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
        .min_x = min_x, .min_y = min_y, .max_x = max_x, .max_y = max_y,
        .mesh = &mesh,
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

        CLAY({
            ID("OuterContainer"),
            .layout = {
                .sizing = sizing_expand,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = APP_TRANSPARENT,
        }) {

            CLAY({
                ID("Header"),
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_GROW(),
                        .height = CLAY_SIZING_PERCENT(0.1),
                    },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = CLAY_PADDING_ALL(10),
                    .childGap = 5,
                },
                .backgroundColor = ClayColorAlpha(APP_VDARK, 220),
            }) {

                // TODO: Add buttons to load .dat files, export, clear...

                Clay_String str = CLAY_STRING("Test");
                CLAY_TEXT(str, CLAY_TEXT_CONFIG({
                    .fontId = FONT_BODY_INDEX,
                    .fontSize = 24,
                    .textColor = APP_PALE_PURPLE,
                }));
            }
        }
 
        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        BeginDrawing();

        ClearBackground(CLAY_COLOR_TO_RAYLIB_COLOR(APP_DARK));
        
        // Draw edges
        for (int i = 0; i < mesh.num_faces; i++) {
            HalfEdge* he = &mesh.halfedges[mesh.faces[i].halfedge];
            for (int j = 0; j < 3; j++) {
                Vertex v1 = mesh.vertices[he->vertex];
                Vertex v2 = mesh.vertices[mesh.halfedges[he->next].vertex];
                if (v1.x < v2.x || he->twin == -1) {
                    draw_edge(&ctx, v1, v2);
                }
                he = &mesh.halfedges[he->next];
            }
        }
        
        // Draw vertices
        for (int i = 0; i < mesh.num_vertices+4; i++) {
            draw_vertex(&ctx, mesh.vertices[i]);
        }
        
        // Click event to add a new point
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            double x = mousePos.x, y = mousePos.y;
            viewportToGeo(&ctx, x, y, &x, &y);
            
            Vertex newVertex = {x, y}; 
            // TODO: Add it to the vertices
        
        // Press the middle mouse button to pan around
        } else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector2 mouseDelta = GetMouseDelta();
            double dx = mouseDelta.x, dy = mouseDelta.y;

            ctx.panning_x += dx;
            ctx.panning_y += dy;
        }
        
        // TODO: Add zoom

        // Recenter (set panning to zero) with 'C'
        if (IsKeyPressed(KEY_C)) {
            ctx.panning_x = 0;
            ctx.panning_y = 0;
        }
        
        Clay_Raylib_Render(renderCommands, fonts);

        EndDrawing();
    }
    
    Clay_Raylib_Close();
    
    return result;
}


