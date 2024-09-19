// Include the main header file
#include <main.hpp>

// Flecs static macro
#define FLECS_STATIC

// Immediate-mode GUI Library implementation (only once)
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Tiled loader
#define CUTE_TILED_IMPLEMENTATION
#include "cute/cute_tiled.h"

// Collision
#define CUTE_C2_IMPLEMENTATION
#include "cute/cute_c2.hpp"

// The constant screen resolution
const int screen_w_const = 1280;
const int screen_h_const = 720;

// The resolution of the window the application is in (browser window size)
int screen_w = screen_w_const;  
int screen_h = screen_h_const;

// Main app
std::unique_ptr<App> main_app;

// Render texture on which the application will be rendered
RenderTexture2D target;

// Render texture destination rectangle  
Rectangle tex_dest;

// Calculate the application's render texture destination rect
// --------------------------------------------------------------------------------------
void calcTexDest();
   
// Scale and translate mouse input to reflect its position on the render texture
// --------------------------------------------------------------------------------------
void transformMouseInput();

// Emscripten's designated main loop function
// --------------------------------------------------------------------------------------
void updateAndDraw();

// C++ Main  
// --------------------------------------------------------------------------------------
int main(int argc, char const *argv[])
{
    // Set antialiasing
    // SetConfigFlags(FLAG_MSAA_4X_HINT);

    // Init window
    InitWindow(screen_w, screen_h, "Raylib Web Test");

    // Create render target of desired resolution
    target = LoadRenderTexture(screen_w, screen_h);

    // Initialize the main App
    main_app = std::make_unique<App>(target);

    // This function is deprecated but its replacement doesn't produce the same result
    emscripten_set_canvas_size(1, 1);

    // Set the emscripten main loop
    emscripten_set_main_loop(updateAndDraw, 0, 1);

    // De-Initialization
    UnloadRenderTexture(target);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

void transformMouseInput()
{
    // Translate mouse position on the canvas to mouse position
    float scale = std::min((float)GetScreenWidth() / screen_w_const, (float)GetScreenHeight() / screen_h_const);
    SetMouseOffset(-(GetScreenWidth() - (screen_w_const * scale)) * 0.5f, -(GetScreenHeight() - (screen_h_const * scale)) * 0.5f);
    SetMouseScale(1 / scale, 1 / scale);
}

void calcTexDest()
{
    // Determine HTML app window size and set raylib render window to this value
    double temp_w, temp_h;
    EMSCRIPTEN_RESULT res = emscripten_get_element_css_size("#canvas", &temp_w, &temp_h);
    screen_w = (int)temp_w;
    screen_h = (int)temp_h;
    SetWindowSize(screen_w, screen_h);

    // Determine if screen size will be limited by width or height (ie. letterboxing)
    float ratio_x = (float)screen_w / (float)screen_w_const;
    float ratio_y = (float)screen_h / (float)screen_h_const;

    if (ratio_x < ratio_y)
    {
        tex_dest.width = screen_w;
        tex_dest.height = (int)(ratio_x * (float)screen_h_const);
        tex_dest.x = 0;
        tex_dest.y = (screen_h - tex_dest.height) / 2;
    }
    else
    {
        tex_dest.width = (int)(ratio_y * (float)screen_w_const);
        tex_dest.height = screen_h;
        tex_dest.x = (screen_w - tex_dest.width) / 2;
        tex_dest.y = 0;
    }
}

void updateAndDraw()
{
    calcTexDest();
    transformMouseInput();

    // Draw all application to the texture
    main_app->update();

    // Draw the transformed app render texture to the window
    // Start drawing and clear tthe background
    BeginDrawing();
    ClearBackground(GRAY);

    // Draw render texture to screen, scaled if required
    DrawTexturePro(target.texture,
                   Rectangle{0, 0, (float)target.texture.width, -(float)target.texture.height},
                   tex_dest,
                   Vector2{0, 0}, 0.0f,
                   WHITE);

    // End drawing to the window
    EndDrawing();
}