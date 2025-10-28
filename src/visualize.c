#include <stdio.h>

#include "lloyd.h"
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
#define APP_FADED_RED   (Clay_Color){ 222,  74,  76, 255 }
#define APP_PEACHY_PINK (Clay_Color){ 227, 125, 126, 255 }

#define ClayColorAlpha(_col, _alpha) ((Clay_Color){ .r = (_col).r, .g = (_col).g, .b = (_col).b, .a = (_alpha) })

#define ID(str) .id = CLAY_ID(str)
#define BORDER_WIDTH_ALL(width) (Clay_BorderWidth){ .left = width, .right = width, .top = width, .bottom = width }
const Clay_Sizing sizing_expand = {
    .width = CLAY_SIZING_GROW(),
    .height = CLAY_SIZING_GROW(),
};

#define Min(x, y) CLAY__MIN(x, y)
#define Max(x, y) CLAY__MAX(x, y)

enum {
    FONT_BODY_INDEX,
    FONT_COUNT,
};
Font fonts[FONT_COUNT];

const int max_additional_vertices = 100;
int added_vertices = 0;

static inline void geoToViewport(DrawCtx *ctx, double x, double y, double *vpx, double *vpy) {
    double L = ctx->mesh->L + 0.05;
    
    double 
        min_x = ctx->min_x - L, // +- L to include the big rectangle, 
        min_y = ctx->min_y - L, // won't be necessary in the future.
        max_x = ctx->max_x + L,
        max_y = ctx->max_y + L
    ;

    int
        window_width  = ctx->window_width,
        window_height = ctx->window_height
    ;

    // --- Zooming ---
    double h_x = max_x - min_x, h_y = max_y - min_y;
    h_x /= ctx->zoom;
    h_y /= ctx->zoom;
    
    double
    c_x = (min_x + max_x) / 2.,
    c_y = (min_y + max_y) / 2.
    ;
    
    min_x = c_x - h_x / 2.;
    max_x = c_x + h_x / 2.;
    min_y = c_y - h_y / 2.;
    max_y = c_y + h_y / 2.;
    // 

    // Scaling factor between the viewport and the physical space
    double units_per_pixel_x = h_x / window_width;
    double units_per_pixel_y = h_y / window_height;

    // --- Panning ---
    double 
        geo_panning_x = ctx->panning_x * units_per_pixel_x,
        geo_panning_y = ctx->panning_y * units_per_pixel_y
    ;
    min_x -= geo_panning_x;
    max_x -= geo_panning_x;
    min_y += geo_panning_y;
    max_y += geo_panning_y;
    //

    double lx = (x-min_x) / h_x;
    *vpx = (lx)   * (window_width);
    
    double ly = (y-min_y) / h_y;
    *vpy = (1-ly) * (window_height);
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

    // --- Zooming ---
    double h_x = max_x - min_x, h_y = max_y - min_y;
    h_x /= ctx->zoom;
    h_y /= ctx->zoom;
    
    double
    c_x = (min_x + max_x) / 2.,
    c_y = (min_y + max_y) / 2.
    ;
    
    min_x = c_x - h_x / 2.;
    max_x = c_x + h_x / 2.;
    min_y = c_y - h_y / 2.;
    max_y = c_y + h_y / 2.;
    // 

    // Scaling factor between the viewport and the physical space
    double units_per_pixel_x = h_x / window_width;
    double units_per_pixel_y = h_y / window_height;

    // --- Panning ---
    double 
        geo_panning_x = ctx->panning_x * units_per_pixel_x,
        geo_panning_y = ctx->panning_y * units_per_pixel_y
    ;
    // We don't use max_x nor min_y anymore but this way it's complete
    min_x -= geo_panning_x;
    max_x -= geo_panning_x;
    min_y += geo_panning_y;
    max_y += geo_panning_y;
    //

    *x = min_x + units_per_pixel_x * vpx;
    *y = max_y - units_per_pixel_y * vpy;
}

void draw_init(DrawCtx *ctx, MyMesh *mesh) {
    // Find max and min coordinates for normalization
    double min_x = __DBL_MAX__, min_y = __DBL_MAX__;
    double max_x = -__DBL_MAX__, max_y = -__DBL_MAX__;
    for (int i = 0; i < mesh->num_vertices; i++) {
        if (mesh->vertices[i].x < min_x) min_x = mesh->vertices[i].x;
        if (mesh->vertices[i].x > max_x) max_x = mesh->vertices[i].x;
        if (mesh->vertices[i].y < min_y) min_y = mesh->vertices[i].y;
        if (mesh->vertices[i].y > max_y) max_y = mesh->vertices[i].y;
    }

    *ctx = (DrawCtx){
        .window_width = 800, .window_height = 600,
        .min_x = min_x, .min_y = min_y, .max_x = max_x, .max_y = max_y,
        .mesh = mesh,

        .zoom = 1.,

        .voronoi_button = {
            .label = CLAY_STRING_CONST("Voronoi"),
            .text = CLAY_STRING_CONST("V - Draw Voronoi"),
            .on = 0,
        },
        .delaunay_button = {
            .label = CLAY_STRING_CONST("Delaunay"),
            .text = CLAY_STRING_CONST("D - Draw Delaunay"),
            .on = 1,
        },

        .update_needed = 1,
        .header_toggle = 1,
    };
}

void draw_vertex(DrawCtx *ctx, Vertex v, int size, Color color) {
    double x = v.x, y = v.y;
    geoToViewport(ctx, x, y, &x, &y);

    // Draw the vertex at (x, y)
    DrawCircle((int)x, (int)y, size, color);
}

void draw_edge(DrawCtx *ctx, Vertex v1, Vertex v2, Color color) {    
    double 
    x1 = v1.x, y1 = v1.y,
    x2 = v2.x, y2 = v2.y
    ;
    geoToViewport(ctx, x1, y1, &x1, &y1);
    geoToViewport(ctx, x2, y2, &x2, &y2);
    
    // Draw the edge from (x1, y1) to (x2, y2)
    DrawLine((int)x1, (int)y1, (int)x2, (int)y2, color);
    // printf("%lf, %lf, %lf, %lf\n", x1, y1, x2, y2);
    // exit(0);
}

void draw_handle_user(DrawCtx *ctx) {
    // Update header height in ctx
    if (!ctx->header_toggle) ctx->header_height = 0;
    else
        ctx->header_height = (int)Clay_GetElementData(CLAY_ID("Header")).boundingBox.height;

    // Click event to add a new point
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();
        double x = mousePos.x, y = mousePos.y;

        if (y >= ctx->header_height) { // Clicked below header
            viewportToGeo(ctx, x, y, &x, &y);
            
            Vertex newVertex = {x, y}; 
            ctx->update_needed = 1;

            // TODO: Tidy up and realloc on insertion if cap exceeded
            if (added_vertices == max_additional_vertices) {
                printf("You can't add more vertices\n");
                return;
            }
            added_vertices++;

            ctx->mesh->vertices[ctx->mesh->num_vertices++] = newVertex;
            rebuild_triangulation(ctx->mesh);
            double min_x = __DBL_MAX__, min_y = __DBL_MAX__;
            double max_x = -__DBL_MAX__, max_y = -__DBL_MAX__;
            for (int i = 0; i < ctx->mesh->num_vertices; i++) {
                if (ctx->mesh->vertices[i].x < min_x) min_x = ctx->mesh->vertices[i].x;
                if (ctx->mesh->vertices[i].x > max_x) max_x = ctx->mesh->vertices[i].x;
                if (ctx->mesh->vertices[i].y < min_y) min_y = ctx->mesh->vertices[i].y;
                if (ctx->mesh->vertices[i].y > max_y) max_y = ctx->mesh->vertices[i].y;
            }
            ctx->min_x = min_x;
            ctx->min_y = min_y;
            ctx->max_x = max_x;
            ctx->max_y = max_y;
        }
        
    // Press the middle mouse button to pan around
    } else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 mouseDelta = GetMouseDelta();
        double dx = mouseDelta.x, dy = mouseDelta.y;

        ctx->panning_x += dx;
        ctx->panning_y += dy;
    }
    
    float zoom;
    if ((zoom = GetMouseWheelMove()) != 0.) {
        ctx->zoom += zoom / 10.;
        ctx->zoom = Clamp(ctx->zoom, 0.5, 5.0);
    }

    // Recenter (set panning to zero and zoom to 1) with 'C'
    if (IsKeyPressed(KEY_C)) {
        ctx->panning_x = 0;
        ctx->panning_y = 0;
        ctx->zoom = 1.;
    } 
    else if (IsKeyPressed(KEY_H)) {
        ctx->header_toggle = !ctx->header_toggle; 
    }
    else if (IsKeyPressed(KEY_V)) {
        ctx->voronoi_button.on = !ctx->voronoi_button.on; 
    }
    else if (IsKeyPressed(KEY_D)) {
        ctx->delaunay_button.on = !ctx->delaunay_button.on; 
    }
    else if (IsKeyPressed(KEY_SLASH)) { // Equal
        // Using a lerp to have a smoother evolution
        for (int vertex_id = 0; vertex_id < ctx->mesh->num_vertices; vertex_id++) {
            Vertex centroid = compute_voronoi_cell_centroid(ctx->mesh, vertex_id); 
            ctx->mesh->vertices[vertex_id].x = Lerp(ctx->mesh->vertices[vertex_id].x, centroid.x, 0.3);
            ctx->mesh->vertices[vertex_id].y = Lerp(ctx->mesh->vertices[vertex_id].y, centroid.y, 0.3);
            ctx->update_needed = 1;
        }

        // TODO: Tidy up
        rebuild_triangulation(ctx->mesh);
        double min_x = __DBL_MAX__, min_y = __DBL_MAX__;
        double max_x = -__DBL_MAX__, max_y = -__DBL_MAX__;
        for (int i = 0; i < ctx->mesh->num_vertices; i++) {
            if (ctx->mesh->vertices[i].x < min_x) min_x = ctx->mesh->vertices[i].x;
            if (ctx->mesh->vertices[i].x > max_x) max_x = ctx->mesh->vertices[i].x;
            if (ctx->mesh->vertices[i].y < min_y) min_y = ctx->mesh->vertices[i].y;
            if (ctx->mesh->vertices[i].y > max_y) max_y = ctx->mesh->vertices[i].y;
        }
        ctx->min_x = min_x;
        ctx->min_y = min_y;
        ctx->max_x = max_x;
        ctx->max_y = max_y;
    }

    // int key;
    // if ((key = GetKeyPressed()) > 0) {
    //     printf("%d\n", key);
    // }
}

void render_shortcuts_grid() {
    static Clay_String shortcuts[] = {
        CLAY_STRING_CONST("Esc - Quit"),
        CLAY_STRING_CONST("C - Recenter"),
        CLAY_STRING_CONST("H - Toggle header"),
        // New shortcuts go here...
    };

    const uint32_t sc_count = sizeof(shortcuts) / sizeof(*shortcuts);
    if (sc_count == 0) return;

    const uint32_t max_sc_per_col = 3;
    const uint32_t col_count = 1 + sc_count / max_sc_per_col;

    for (uint32_t col_id = 0; col_id < col_count; col_id++) {
        CLAY({
            .id = CLAY_IDI("ShortcutColumn", col_id),
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIT(),
                    .height = CLAY_SIZING_FIT(),
                },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = 1,
            },
            .backgroundColor = ClayColorAlpha(APP_VVDARK, 220),
        }) {

            for (
                uint32_t sc_id = max_sc_per_col*col_id; 
                sc_id < Min(sc_count, max_sc_per_col*(col_id+1)); 
                sc_id++
            ) {
                Clay_String sc = shortcuts[sc_id];
                CLAY_TEXT(sc, CLAY_TEXT_CONFIG({
                    .fontId = FONT_BODY_INDEX,
                    .fontSize = 20,
                    .textColor = APP_PALE_PURPLE,
                    .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                }));
            }     
        }

    }
}

void render_button(Button *button) {
    uint32_t id = (uint32_t)(uintptr_t)button;

    CLAY({
        CLAY_IDI("_Button", id),
        .layout = {
            .padding = 3,
            .childGap = 5,
        },
    }) {

        CLAY_TEXT(button->text, CLAY_TEXT_CONFIG({
            .fontId = FONT_BODY_INDEX,
            .fontSize = 20,
            .textColor = APP_PALE_PURPLE,
            .textAlignment = CLAY_TEXT_ALIGN_CENTER,
        }));

        CLAY({
            .id = CLAY_IDI("_ButtonInner", id),
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(20),
                    .height = CLAY_SIZING_FIXED(20),
                },
            },
            .cornerRadius = CLAY_CORNER_RADIUS(3),
            .border = { 
                .color = APP_PURPLE,
                .width = BORDER_WIDTH_ALL(2), 
            },
        }) {
    
            if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                button->on = !button->on;
    
            if (button->on) {
                CLAY({
                    .id = CLAY_IDI("_ButtonInnerContent", id),
                    .layout = {
                        .sizing = sizing_expand,
                    },
                    .backgroundColor = APP_PALE_PURPLE,
                    .cornerRadius = CLAY_CORNER_RADIUS(3),
                }) {};
            }
        };
    }
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    // Call your Delaunay triangulation function
    MyMesh *meshp;
    int result = Cdelaunay(argv[1], argv[2], &meshp);
    if (result == 0) {
        printf("Delaunay triangulation completed successfully.\n");
    } else {
        printf("Error in Delaunay triangulation.\n");
    }
    MyMesh mesh = *meshp;
    meshp = NULL; // Obsolete pointer

    // Add a maximum of 100 extra vertices for user clicks
    mesh.max_vertices += max_additional_vertices;
    mesh.vertices = (Vertex*)realloc(mesh.vertices, mesh.max_vertices * sizeof(Vertex));
    mesh.max_halfedges += max_additional_vertices * 3; // Each new vertex can create up to 3
    mesh.halfedges = (HalfEdge*)realloc(mesh.halfedges, mesh.max_halfedges * sizeof(HalfEdge));
    mesh.max_faces += max_additional_vertices; // Each new vertex can create up to 1 new
    mesh.faces = (Face*)realloc(mesh.faces, mesh.max_faces * sizeof(Face));

    DrawCtx ctx;
    draw_init(&ctx, &mesh);

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

    int *adj_faces = malloc(sizeof(int) * 3 * mesh.num_faces);
    Vertex *adj_verts = malloc(sizeof(Vertex) * mesh.num_faces);
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

            int headerAlpha = ctx.header_toggle ? 220 : 0;

            CLAY({
                ID("Header"),
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_GROW(),
                        .height = CLAY_SIZING_FIT(),
                    },
                    .childGap = 5,
                    .padding = CLAY_PADDING_ALL(5),
                },
                .backgroundColor = ClayColorAlpha(APP_VDARK, headerAlpha),
            }) {

                // TODO: Add buttons to load .dat files, export, clear...

                CLAY({
                    ID("Shortcuts"),
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_FIT(),
                            .height = CLAY_SIZING_GROW(),
                        },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .childGap = 5,
                    },
                    .cornerRadius = 3,
                }) {

                    if (ctx.header_toggle)
                        render_shortcuts_grid(headerAlpha);
                }

                CLAY({
                    ID("HeaderPadding"),
                    .layout = {
                        .sizing = sizing_expand,
                    }
                }) {}

                CLAY({
                    ID("HeaderButtons"),
                    .layout = {
                        .childGap = 5,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = ClayColorAlpha(APP_VVDARK, headerAlpha),
                    .cornerRadius = 3,
                }) {

                    if (ctx.header_toggle) {
                        render_button(&ctx.voronoi_button);
                        render_button(&ctx.delaunay_button);
                    }
                }
            }
        }
 
        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        BeginDrawing();

        ClearBackground(CLAY_COLOR_TO_RAYLIB_COLOR(APP_DARK));
        
        if (ctx.delaunay_button.on) {
            // Draw edges
            for (int i = 0; i < mesh.num_faces; i++) {
                HalfEdge* he = &mesh.halfedges[mesh.faces[i].halfedge];
                for (int j = 0; j < 3; j++) {
                    Vertex v1 = mesh.vertices[he->vertex];
                    Vertex v2 = mesh.vertices[mesh.halfedges[he->next].vertex];
                    if (v1.x < v2.x || he->twin == -1) {
                        draw_edge(&ctx, v1, v2, CLAY_COLOR_TO_RAYLIB_COLOR(APP_PALE_PURPLE));
                    }
                    he = &mesh.halfedges[he->next];
                }
            }
            
            // Draw vertices (only the actual vertices, not the extremity points)
            for (int i = 0; i < mesh.num_vertices; i++) {
                draw_vertex(&ctx, mesh.vertices[i], 3, CLAY_COLOR_TO_RAYLIB_COLOR(APP_PURPLE));
                draw_vertex(&ctx, compute_voronoi_cell_centroid(&mesh, i), 3, GREEN);
            }
        }

        if (ctx.update_needed) { // Mesh has changed -> Recompute adjacency

            adj_faces = realloc(adj_faces, sizeof(int) * 3 * mesh.num_faces);
            adj_verts = realloc(adj_verts, sizeof(Vertex) * mesh.num_faces);

            // Element of adj_faces at i+0 to i+2 is the index of the face that 
            // face i is bordering, -1 if face is on boundary.
            // adj_verts are the circumcenters of the faces (vertices of voronoi).
            memset(adj_faces, -1, sizeof(int) * 3 * mesh.num_faces);    
            memset(adj_verts, -1, sizeof(Vertex) * mesh.num_faces);

            for (int face_id = 0; face_id < mesh.num_faces; face_id++) {
                HalfEdge he = mesh.halfedges[mesh.faces[face_id].halfedge];
                
                for (int i = 0; i < 3; i++) {
                    if (he.twin != -1) {
                        HalfEdge twin = mesh.halfedges[he.twin];
                        adj_faces[3*face_id+i] = twin.face;
                    }
                    
                    he = mesh.halfedges[he.next];
                }
    
                adj_verts[face_id] = compute_circumcenter(&mesh, face_id);
            }
            
            ctx.update_needed = 0;
        }

        // Drawing the Voronoi diagram 
        if (ctx.voronoi_button.on) {
            for (int face_id = 0; face_id < mesh.num_faces; face_id++) {
                Vertex v = adj_verts[face_id];
                draw_vertex(&ctx, v, 3, CLAY_COLOR_TO_RAYLIB_COLOR(APP_FADED_RED));
                
                for (int i = 0; i < 3; i++) {
                    int other_face = adj_faces[face_id*3+i];
                    if (other_face == -1) continue;               
                    
                    Vertex w = adj_verts[other_face];
                    draw_edge(&ctx, v, w, CLAY_COLOR_TO_RAYLIB_COLOR(APP_PEACHY_PINK));
                }
            }
        }
        
        // Render the UI on top
        Clay_Raylib_Render(renderCommands, fonts);

        EndDrawing();

        draw_handle_user(&ctx);
    }
    
    Clay_Raylib_Close();
    
    free(adj_faces);
    free(adj_verts);
    
    // Free the mesh arrays (they were reallocated locally)
    free(mesh.vertices);
    free(mesh.halfedges); 
    free(mesh.faces);
    free(meshp); // Free the original mesh structure
    
    return result;
}