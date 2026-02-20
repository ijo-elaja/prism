#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#undef RAYGUI_IMPLEMENTATION

#include "utils.h"
#include <unistd.h>

static const int INITIAL_WIDTH = 800;
static const int INITIAL_HEIGHT = 600;
static const ulong MAX_PIXELS = 268435456; // 16384^2

typedef struct ppm_image {
    uint width;
    uint height;
    uint max_color;
    Color* pixels;
} ppm_image_t;

typedef enum app_mode {
    MODE_CREATE_IMAGE,
    MODE_EDITING,
} app_mode_t;

typedef enum tool_type {
    TOOL_BRUSH,
    TOOL_FILL,
} tool_type_t;

typedef struct state {
    int monitor_id;
    int width;
    int height;
    int target_fps;
    Color clear_color;
    Color text_color;
    Font default_font;

    app_mode_t mode;
    ppm_image_t* image;
    char current_filepath[256];

    // UI state for create dialog
    char image_width_str[16];
    char image_height_str[16];
    char max_color_str[16];
    char open_filepath_str[256];
    int focused_textbox; // 0=width, 1=height, 2=max_color, 3=open_path, -1=none

    // Editing state
    float zoom;
    float pan_x;
    float pan_y;
    Color brush_color;
    tool_type_t current_tool;
    int brush_radius;

    // Color picker state
    bool color_picker_active;
    int color_r;
    int color_g;
    int color_b;
} state_t;

void free_state(state_t* state) {
    if(state->image) {
        free(state->image->pixels);
        free(state->image);
    }
    free(state);
}

ppm_image_t* create_ppm_image(uint width, uint height, uint max_color) {
    if((ulong)width * height > MAX_PIXELS) {
        error("Image too large: %u * %u > %lu", width, height, MAX_PIXELS);
        return NULL;
    }

    ppm_image_t* img = make(ppm_image_t);
    img->width = width;
    img->height = height;
    img->max_color = max_color;
    img->pixels = make(Color, width * height);

    // Initialize with white
    Color white = { 255, 255, 255, 255 };
    for(ulong i = 0; i < (ulong)width * height; i++) {
        img->pixels[i] = white;
    }

    return img;
}

bool file_dialog_open(char* filepath, size_t filepath_size) {
    // Use zenity for file dialog on Linux
    FILE* cmd = popen("zenity --file-selection --filename=$HOME --title=\"Open PPM Image\" 2>/dev/null", "r");
    if(!cmd) {
        error("Failed to open file dialog");
        return false;
    }

    if(fgets(filepath, filepath_size, cmd) == NULL) {
        pclose(cmd);
        return false;
    }

    pclose(cmd);

    // Remove trailing newline
    size_t len = strlen(filepath);
    if(len > 0 && filepath[len - 1] == '\n') {
        filepath[len - 1] = '\0';
    }

    return filepath[0] != '\0';
}

bool file_dialog_save(char* filepath, size_t filepath_size) {
    // Use zenity for save dialog on Linux
    FILE* cmd = popen("zenity --file-selection --save --filename=$HOME/output.ppm --title=\"Save PPM Image\" 2>/dev/null", "r");
    if(!cmd) {
        error("Failed to open save dialog");
        return false;
    }

    if(fgets(filepath, filepath_size, cmd) == NULL) {
        pclose(cmd);
        return false;
    }

    pclose(cmd);

    // Remove trailing newline
    size_t len = strlen(filepath);
    if(len > 0 && filepath[len - 1] == '\n') {
        filepath[len - 1] = '\0';
    }

    return filepath[0] != '\0';
}

bool save_ppm_image(ppm_image_t* img, const char* filepath) {
    FILE* f = fopen(filepath, "w");
    if(!f) {
        error("Failed to open file for writing: %s", filepath);
        return false;
    }

    fprintf(f, "P3\n%u %u\n%u\n", img->width, img->height, img->max_color);

    float scale = img->max_color / 255.0f;
    for(ulong i = 0; i < (ulong)img->width * img->height; i++) {
        Color c = img->pixels[i];
        uint r = (uint)(c.r * scale);
        uint g = (uint)(c.g * scale);
        uint b = (uint)(c.b * scale);
        fprintf(f, "%u %u %u\n", r, g, b);
    }

    fclose(f);
    log("Saved PPM image to %s (%u x %u)", filepath, img->width, img->height);
    return true;
}

ppm_image_t* load_ppm_image(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if(!f) {
        error("Failed to open file for reading: %s", filepath);
        return NULL;
    }

    char magic[3];
    uint width, height, max_color;
    if(fscanf(f, "%2s", magic) != 1 || magic[0] != 'P' || magic[1] != '3') {
        error("Invalid PPM file format: %s", filepath);
        fclose(f);
        return NULL;
    }

    if(fscanf(f, "%u %u %u", &width, &height, &max_color) != 3) {
        error("Failed to read PPM header: %s", filepath);
        fclose(f);
        return NULL;
    }

    ppm_image_t* img = create_ppm_image(width, height, max_color);
    if(!img) {
        fclose(f);
        return NULL;
    }

    float scale = 255.0f / max_color;
    for(ulong i = 0; i < (ulong)width * height; i++) {
        uint r, g, b;
        if(fscanf(f, "%u %u %u", &r, &g, &b) != 3) {
            error("Failed to read pixel data from PPM: %s", filepath);
            free(img->pixels);
            free(img);
            fclose(f);
            return NULL;
        }
        img->pixels[i] = (Color){ (unsigned char)(r * scale), (unsigned char)(g * scale), (unsigned char)(b * scale), 255 };
    }

    fclose(f);
    log("Loaded PPM image from %s (%u x %u)", filepath, width, height);
    return img;
}

void calculate_zoom_to_fit(state_t* state) {
    if(!state->image) return;

    // Available screen area (accounting for toolbar and panels)
    float available_width = state->width - 20.0f;
    float available_height = state->height - 210.0f;

    // Calculate zoom needed to fit image
    float zoom_for_width = available_width / state->image->width;
    float zoom_for_height = available_height / state->image->height;

    // Use the smaller zoom to ensure it fits
    state->zoom = zoom_for_width < zoom_for_height ? zoom_for_width : zoom_for_height;

    // Center the image
    float image_screen_width = state->image->width * state->zoom;
    float image_screen_height = state->image->height * state->zoom;
    state->pan_x = (available_width - image_screen_width) / 2.0f + 10.0f;
    state->pan_y = (available_height - image_screen_height) / 2.0f + 60.0f;
}

static inline bool colors_equal(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

void flood_fill(ppm_image_t* img, int x, int y, Color new_color) {
    if(x < 0 || x >= (int)img->width || y < 0 || y >= (int)img->height) return;

    Color old_color = img->pixels[y * img->width + x];
    if(colors_equal(old_color, new_color)) return;

    // Simple iterative flood fill with stack
    int* stack_x = make(int, img->width * img->height);
    int* stack_y = make(int, img->width * img->height);
    int stack_size = 0;

    stack_x[stack_size] = x;
    stack_y[stack_size] = y;
    stack_size++;

    while(stack_size > 0) {
        stack_size--;
        int cx = stack_x[stack_size];
        int cy = stack_y[stack_size];

        if(cx < 0 || cx >= (int)img->width || cy < 0 || cy >= (int)img->height) continue;

        Color current = img->pixels[cy * img->width + cx];
        if(!colors_equal(current, old_color)) continue;

        img->pixels[cy * img->width + cx] = new_color;

        // Add neighbors to stack
        if(stack_size + 4 < img->width * img->height) {
            stack_x[stack_size] = cx + 1; stack_y[stack_size] = cy; stack_size++;
            stack_x[stack_size] = cx - 1; stack_y[stack_size] = cy; stack_size++;
            stack_x[stack_size] = cx; stack_y[stack_size] = cy + 1; stack_size++;
            stack_x[stack_size] = cx; stack_y[stack_size] = cy - 1; stack_size++;
        }
    }

    free(stack_x);
    free(stack_y);
}

void paint_brush(ppm_image_t* img, int x, int y, Color color, int radius) {
    for(int py = y - radius; py <= y + radius; py++) {
        for(int px = x - radius; px <= x + radius; px++) {
            if(px >= 0 && px < (int)img->width && py >= 0 && py < (int)img->height) {
                int dx = px - x;
                int dy = py - y;
                if(dx * dx + dy * dy <= radius * radius) {
                    img->pixels[py * img->width + px] = color;
                }
            }
        }
    }
}

state_t* init(void) {
    state_t* state = make(state_t);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "Prism | PPM Image Editor");
    SetWindowMonitor(1);

    state->monitor_id = GetCurrentMonitor();

    state->width = GetRenderWidth();
    state->height = GetRenderHeight();
    state->target_fps = GetMonitorRefreshRate(state->monitor_id);

    SetTargetFPS(state->target_fps);

    GuiLoadStyleDefault();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

    state->clear_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    state->text_color = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
    state->default_font = GuiGetFont();

    state->mode = MODE_CREATE_IMAGE;
    state->image = NULL;
    state->zoom = 1.0f;
    state->pan_x = 0.0f;
    state->pan_y = 0.0f;
    state->brush_color = BLACK;
    state->focused_textbox = -1;
    state->current_tool = TOOL_BRUSH;
    state->brush_radius = 5;
    state->color_picker_active = false;
    state->color_r = 0;
    state->color_g = 0;
    state->color_b = 0;
    state->current_filepath[0] = '\0';

    snprintf(state->image_width_str, sizeof(state->image_width_str), "512");
    snprintf(state->image_height_str, sizeof(state->image_height_str), "512");
    snprintf(state->max_color_str, sizeof(state->max_color_str), "255");
    snprintf(state->open_filepath_str, sizeof(state->open_filepath_str), "");

    return state;
}

void update(state_t* state) {
    // Handle window resize
    state->width = GetRenderWidth();
    state->height = GetRenderHeight();

    if(state->mode == MODE_EDITING && state->image) {
        // Zoom with mouse wheel
        state->zoom += GetMouseWheelMove() * 0.1f;
        if(state->zoom < 0.1f) state->zoom = 0.1f;
        if(state->zoom > 10.0f) state->zoom = 10.0f;

        // Pan with middle mouse button or space + drag
        if(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_BUTTON_LEFT))) {
            state->pan_x += GetMouseDelta().x;
            state->pan_y += GetMouseDelta().y;
        }

        // Paint/Fill with left click
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsKeyDown(KEY_SPACE)) {
            Vector2 mouse_pos = GetMousePosition();
            float canvas_x = (mouse_pos.x - state->pan_x) / state->zoom;
            float canvas_y = (mouse_pos.y - state->pan_y) / state->zoom;

            int px = (int)canvas_x;
            int py = (int)canvas_y;

            if(px >= 0 && px < (int)state->image->width && py >= 0 && py < (int)state->image->height) {
                if(state->current_tool == TOOL_BRUSH) {
                    paint_brush(state->image, px, py, state->brush_color, state->brush_radius);
                } else if(state->current_tool == TOOL_FILL) {
                    flood_fill(state->image, px, py, state->brush_color);
                }
            }
        }

        // Update brush color from RGB sliders
        state->brush_color = (Color){ state->color_r, state->color_g, state->color_b, 255 };
    }
}

void draw_create_image_dialog(state_t* state) {
    int dialog_x = (state->width - 500) / 2;
    int dialog_y = (state->height - 400) / 2;

    GuiLabel((Rectangle) { dialog_x, dialog_y, 500, 30 }, "Create New or Open PPM Image");

    // Create section
    GuiLabel((Rectangle) { dialog_x + 20, dialog_y + 50, 200, 20 }, "Create New:");

    GuiLabel((Rectangle) { dialog_x + 20, dialog_y + 75, 100, 20 }, "Width:");
    if(GuiTextBox((Rectangle) { dialog_x + 130, dialog_y + 75, 150, 25 }, state->image_width_str, 15, state->focused_textbox == 0)) {
        if(state->focused_textbox != 0) state->focused_textbox = 0;
    }

    GuiLabel((Rectangle) { dialog_x + 20, dialog_y + 110, 100, 20 }, "Height:");
    if(GuiTextBox((Rectangle) { dialog_x + 130, dialog_y + 110, 150, 25 }, state->image_height_str, 15, state->focused_textbox == 1)) {
        if(state->focused_textbox != 1) state->focused_textbox = 1;
    }

    GuiLabel((Rectangle) { dialog_x + 20, dialog_y + 145, 100, 20 }, "Max Color:");
    if(GuiTextBox((Rectangle) { dialog_x + 130, dialog_y + 145, 150, 25 }, state->max_color_str, 15, state->focused_textbox == 2)) {
        if(state->focused_textbox != 2) state->focused_textbox = 2;
    }

    if(GuiButton((Rectangle) { dialog_x + 300, dialog_y + 75, 150, 95 }, "Create New")) {
        uint width = atoi(state->image_width_str);
        uint height = atoi(state->image_height_str);
        uint max_color = atoi(state->max_color_str);

        if(width > 0 && height > 0 && max_color > 0 && max_color <= 65535) {
            state->image = create_ppm_image(width, height, max_color);
            if(state->image) {
                state->mode = MODE_EDITING;
                calculate_zoom_to_fit(state);
                state->current_filepath[0] = '\0';
                log("Created %ux%u PPM image with max color %u", width, height, max_color);
            }
        } else {
            error("Invalid parameters");
        }
    }

    // Open section
    GuiLabel((Rectangle) { dialog_x + 20, dialog_y + 260, 200, 20 }, "Or Open File:");

    if(GuiButton((Rectangle) { dialog_x + 130, dialog_y + 300, 150, 30 }, "Open File...")) {
        if(file_dialog_open(state->open_filepath_str, sizeof(state->open_filepath_str))) {
            ppm_image_t* loaded = load_ppm_image(state->open_filepath_str);
            if(loaded) {
                if(state->image) {
                    free(state->image->pixels);
                    free(state->image);
                }
                state->image = loaded;
                state->mode = MODE_EDITING;
                calculate_zoom_to_fit(state);
                snprintf(state->current_filepath, sizeof(state->current_filepath), "%s", state->open_filepath_str);
            } else {
                error("Failed to load PPM file");
            }
        }
    }
}

void draw_editing_canvas(state_t* state) {
    if(!state->image) return;

    // Draw image with current zoom and pan - no gaps/grid
    for(uint y = 0; y < state->image->height; y++) {
        for(uint x = 0; x < state->image->width; x++) {
            float screen_x = state->pan_x + x * state->zoom;
            float screen_y = state->pan_y + y * state->zoom;

            // Add 1 pixel to prevent gaps between rectangles
            float size = state->zoom + 1.0f;
            DrawRectangle((int)screen_x, (int)screen_y, (int)size, (int)size,
                state->image->pixels[y * state->image->width + x]);
        }
    }

    // Top toolbar
    int toolbar_y = 10;
    GuiLabel((Rectangle) { 10, toolbar_y, 200, 20 }, TextFormat("Image: %ux%u", state->image->width, state->image->height));
    GuiLabel((Rectangle) { 10, toolbar_y + 25, 200, 20 }, TextFormat("Zoom: %.1fx", state->zoom));

    int button_x = 220;
    if(GuiButton((Rectangle) { button_x, toolbar_y, 70, 30 }, "New")) {
        free(state->image->pixels);
        free(state->image);
        state->image = NULL;
        state->mode = MODE_CREATE_IMAGE;
        state->current_filepath[0] = '\0';
    }
    button_x += 75;

    if(GuiButton((Rectangle) { button_x, toolbar_y, 70, 30 }, "Open")) {
        char filepath[256];
        if(file_dialog_open(filepath, sizeof(filepath))) {
            ppm_image_t* loaded = load_ppm_image(filepath);
            if(loaded) {
                if(state->image) {
                    free(state->image->pixels);
                    free(state->image);
                }
                state->image = loaded;
                snprintf(state->current_filepath, sizeof(state->current_filepath), "%s", filepath);
                calculate_zoom_to_fit(state);
                log("Opened file: %s", filepath);
            } else {
                error("Failed to load PPM file");
            }
        }
    }
    button_x += 75;

    if(GuiButton((Rectangle) { button_x, toolbar_y, 70, 30 }, "Save")) {
        char filepath[256];
        if(state->current_filepath[0] != '\0') {
            // Save to current file
            if(save_ppm_image(state->image, state->current_filepath)) {
                log("Saved to: %s", state->current_filepath);
            }
        } else {
            // Open save dialog
            if(file_dialog_save(filepath, sizeof(filepath))) {
                if(save_ppm_image(state->image, filepath)) {
                    snprintf(state->current_filepath, sizeof(state->current_filepath), "%s", filepath);
                    log("Saved to: %s", filepath);
                }
            }
        }
    }
    button_x += 75;

    GuiLabel((Rectangle) { button_x, toolbar_y, 300, 20 }, TextFormat("File: %s", state->current_filepath[0] ? state->current_filepath : "[Untitled]"));

    // Tool selection panel - bottom left
    int tool_panel_y = state->height - 150;
    GuiLabel((Rectangle) { 10, tool_panel_y, 100, 20 }, "Tools:");

    if(GuiButton((Rectangle) { 10, tool_panel_y + 25, 70, 25 }, state->current_tool == TOOL_BRUSH ? "[Brush]" : "Brush")) {
        state->current_tool = TOOL_BRUSH;
    }
    if(GuiButton((Rectangle) { 85, tool_panel_y + 25, 70, 25 }, state->current_tool == TOOL_FILL ? "[Fill]" : "Fill")) {
        state->current_tool = TOOL_FILL;
    }

    // Brush radius slider (only for brush tool)
    if(state->current_tool == TOOL_BRUSH) {
        GuiLabel((Rectangle) { 10, tool_panel_y + 55, 150, 20 }, TextFormat("Radius: %d", state->brush_radius));
        float radius_f = state->brush_radius;
        GuiSlider((Rectangle) { 10, tool_panel_y + 75, 150, 20 }, "", "", & radius_f, 1, 50);
        state->brush_radius = (int)radius_f;
    }

    // Color picker - bottom right
    if(GuiButton((Rectangle) { state->width - 240, tool_panel_y, 50, 40 }, "Color")) {
        state->color_picker_active = !state->color_picker_active;
    }

    // Color preview box
    DrawRectangle(state->width - 180, tool_panel_y, 50, 40, (Color) { state->color_r, state->color_g, state->color_b, 255 });
    DrawRectangleLines(state->width - 180, tool_panel_y, 50, 40, WHITE);

    // Color picker sliders
    if(state->color_picker_active) {
        int picker_y = tool_panel_y - 100;
        GuiLabel((Rectangle) { state->width - 240, picker_y, 100, 20 }, "Red:");
        float r_f = state->color_r;
        GuiSlider((Rectangle) { state->width - 240, picker_y + 20, 230, 20 }, "", "", & r_f, 0, 255);
        state->color_r = (int)r_f;

        GuiLabel((Rectangle) { state->width - 240, picker_y + 45, 100, 20 }, "Green:");
        float g_f = state->color_g;
        GuiSlider((Rectangle) { state->width - 240, picker_y + 65, 230, 20 }, "", "", & g_f, 0, 255);
        state->color_g = (int)g_f;

        GuiLabel((Rectangle) { state->width - 240, picker_y + 90, 100, 20 }, "Blue:");
        float b_f = state->color_b;
        GuiSlider((Rectangle) { state->width - 240, picker_y + 110, 230, 20 }, "", "", & b_f, 0, 255);
        state->color_b = (int)b_f;
    }
}

void draw(state_t* state) {
    ClearBackground(state->clear_color);

    if(state->mode == MODE_CREATE_IMAGE) {
        draw_create_image_dialog(state);
    } else if(state->mode == MODE_EDITING) {
        draw_editing_canvas(state);
    }
}

int main(void) {
    state_t* state = init();

    log("Running at %dx%d@%dhz", state->width, state->height, state->target_fps);

    while(!WindowShouldClose()) {
        BeginDrawing();

        update(state);
        draw(state);

        EndDrawing();
    }

    CloseWindow();

    free_state(state);

    return 0;
}
