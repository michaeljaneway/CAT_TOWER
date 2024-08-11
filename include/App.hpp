#pragma once
#include "main.hpp"

// Main Application Class
class App
{
private:
    // App render texture
    //--------------------------------------------------------------------------------------
    RenderTexture2D target;

    // World Values
    //--------------------------------------------------------------------------------------
    std::unique_ptr<flecs::world> ecs_world;

    // Map and map-related values
    //--------------------------------------------------------------------------------------
    std::unique_ptr<Map> map;

    // Grid-based Map
    //--------------------------------------------------------------------------------------
    std::vector<std::vector<uint8_t>> object_map;
    std::vector<std::vector<uint8_t>> object_checkp_map;
    std::vector<std::vector<uint8_t>> object_reset_map;
    // Set screen w and h
    float screen_w;
    float screen_h;

    // Counter for speedrunning
    //--------------------------------------------------------------------------------------

    float time_counter;

    // Debug GUI Values
    //--------------------------------------------------------------------------------------

    bool render_colliders;
    bool render_positions;

    // Audio
    //--------------------------------------------------------------------------------------

    // Has the Raylib audio module been initialized
    bool is_audio_initialized;

    // Gamestate & FPS clock
    //--------------------------------------------------------------------------------------

    // Gamestate Values
    plt::GameState game_state;
    plt::GameState prev_game_state;

    std::chrono::system_clock::time_point last_frame;

    // Fonts
    //--------------------------------------------------------------------------------------
    Font lookout_font;
    Font fear_font;
    Font absolute_font;
    //--------------------------------------------------------------------------------------

    // Textures
    //--------------------------------------------------------------------------------------

    // Default Texture
    Texture2D def_tex;

    void loadTexFromImg(std::string img_file, Texture2D *tex);

    // Shaders
    //--------------------------------------------------------------------------------------

    // Balatro Background Shader
    Shader bal_shader;
    RenderTexture2D bal_texture;
    float delta_t_bal;
    std::map<std::string, int> bal_shader_uni;

    Shader chromatic_abb_shader;
    std::map<std::string, int> chrom_shader_uni;

    //--------------------------------------------------------------------------------------

    // Audio
    //--------------------------------------------------------------------------------------
    std::vector<Music> game_music;

    void handleGameMusic();
    void playGameMusic(Music &mus);

    // Sounds
    Sound jump_sound;

    //--------------------------------------------------------------------------------------

    // ECS Systems
    //--------------------------------------------------------------------------------------

    // Initialize systems and attatch them to the ECS world
    void initFlecsSystems();

    // Get input from player
    void PlayerSystem(flecs::entity e, plt::Position &pos, plt::Player &player);
    float player_vert_progress; // Player Vertical Progress

    Vector2i getPlayerPos();

    // Map position system
    void MapPosSystem();
    Rectangle map_dest;

    // Render the world after all updates
    void RenderSystem();

    // Moves an entity at pos by mov, then returns if the movement was successful (ie. not blocked)
    bool gridMove(Vector2i pos, Vector2i mov);

    // Move in a specified direction infinitely until blocked, returning the info of where it stopped and what it was blocked by
    MoveInfo infGridMove(Vector2i pos, Direction dir);

    // Check what type of entity is at the specified location
    GridVal gridCheck(Vector2i pos);

    bool print_grid;

public:
    App(RenderTexture2D target);
    ~App();

    // Update the application (audio will be updated more often than display)
    void update();
};