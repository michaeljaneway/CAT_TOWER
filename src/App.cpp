#include <App.hpp>

// App Initialization & Destruction
// ==================================================

// Constructor
App::App(RenderTexture2D target)
{
    // Set screen w and h
    this->target = target;
    this->screen_w = target.texture.width;
    this->screen_h = target.texture.height;

    // Timing initialization
    //--------------------------------------------------------------------------------------

    // Set gameplay timer
    time_counter = 0;
    time_limit = 560.0;

    // Debug flag initialization
    //--------------------------------------------------------------------------------------

    render_colliders = false;
    render_positions = false;

    // Audio flag initialization
    //--------------------------------------------------------------------------------------

    is_audio_initialized = false;

    // Load fonts
    //--------------------------------------------------------------------------------------

    lookout_font = LoadFontEx("fonts/Lookout 7.ttf", 128, 0, 250);
    fear_font = LoadFontEx("fonts/Fear 11.ttf", 128, 0, 250);
    absolute_font = LoadFontEx("fonts/Absolute 10.ttf", 128, 0, 250);

    // Initialize gamestate
    //--------------------------------------------------------------------------------------

    game_state = plt::GameState_MainMenu;
    prev_game_state = plt::GameState_MainMenu;

    // Initialize FLECS World
    //--------------------------------------------------------------------------------------

    ecs_world = std::make_unique<flecs::world>();
    initFlecsSystems();

    // Initialize the Map
    //--------------------------------------------------------------------------------------

    map = std::make_unique<Map>(ecs_world.get(), &object_map);

    // Destination w and h stay the same
    RenderTexture2D map_tex = map->getRenderTexture();
    map_dest.width = map_tex.texture.width * 3.f;
    map_dest.height = map_tex.texture.height * 3.f;

    map_dest.x = screen_w / 2 - map_dest.width / 2;
    map_dest.y = -map_dest.height;

    player_vert_progress = 0.f;

    // Set first chekpoint and reset maps now
    object_checkp_map = object_map;
    object_reset_map = object_map;

    player_orient = Direction_Down;
    player_checkp_orient = player_orient;
    player_reset_map_orient = player_orient;

    // Load game textures
    //--------------------------------------------------------------------------------------

    // Balatro Shader
    // https://godotshaders.com/shader/balatro-paint-mix/

    bal_shader = LoadShader(0, "shaders/balatro.fs");
    bal_texture = LoadRenderTexture(1280, 720);

    bal_shader_uni["spin_rotation"] = GetShaderLocation(bal_shader, "spin_rotation");
    bal_shader_uni["spin_speed"] = GetShaderLocation(bal_shader, "spin_speed");
    bal_shader_uni["colour_1"] = GetShaderLocation(bal_shader, "colour_1");
    bal_shader_uni["colour_2"] = GetShaderLocation(bal_shader, "colour_2");
    bal_shader_uni["colour_3"] = GetShaderLocation(bal_shader, "colour_3");
    bal_shader_uni["contrast"] = GetShaderLocation(bal_shader, "contrast");
    bal_shader_uni["spin_amount"] = GetShaderLocation(bal_shader, "spin_amount");
    bal_shader_uni["pixel_filter"] = GetShaderLocation(bal_shader, "pixel_filter");
    bal_shader_uni["delta_time"] = GetShaderLocation(bal_shader, "delta_time");

    float spin_rot = 10;
    float spin_speed = 5;
    float col_1[4] = {0.3, 0.3, 0.3, 1};
    float col_2[4] = {0.9, 0.9, 0.9, 1};
    float col_3[4] = {0.1, 0.1, 0.1, 1};
    float contrast = 2;
    float spin_amount = 0.36;
    float pix_filt = 7000;
    delta_t_bal = 0;

    SetShaderValue(bal_shader, bal_shader_uni["spin_rotation"], &spin_rot, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bal_shader, bal_shader_uni["spin_speed"], &spin_speed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bal_shader, bal_shader_uni["colour_1"], &col_1, SHADER_UNIFORM_VEC4);
    SetShaderValue(bal_shader, bal_shader_uni["colour_2"], &col_2, SHADER_UNIFORM_VEC4);
    SetShaderValue(bal_shader, bal_shader_uni["colour_3"], &col_3, SHADER_UNIFORM_VEC4);
    SetShaderValue(bal_shader, bal_shader_uni["contrast"], &contrast, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bal_shader, bal_shader_uni["spin_amount"], &spin_amount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bal_shader, bal_shader_uni["pixel_filter"], &pix_filt, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bal_shader, bal_shader_uni["delta_time"], &delta_t_bal, SHADER_UNIFORM_FLOAT);

    // Load the default texture
    loadTexFromImg("[v1.3] tranquil_tunnels_transparent.png", &ttt_tex);
    loadTexFromImg("cat.png", &cat_tex);
}

// Destructor
App::~App()
{
    // Shaders
    UnloadShader(bal_shader);

    // Fonts
    UnloadFont(fear_font);
    UnloadFont(lookout_font);
    UnloadFont(absolute_font);

    // Audio
    if (is_audio_initialized)
    {
        UnloadSound(jump_sound);
    }
}

// Load texture from an image
void App::loadTexFromImg(std::string img_file, Texture2D *tex)
{
    Image img = LoadImage(img_file.c_str());
    *tex = LoadTextureFromImage(img);
    UnloadImage(img);
}

// Initialize all Flecs systems
void App::initFlecsSystems()
{
    flecs::system player_system = ecs_world->system<plt::Player>()
                                      .kind(flecs::PreUpdate)
                                      .each([&](flecs::entity e, plt::Player &player)
                                            {
                                                PlayerSystem(e, player); //
                                            });

    flecs::system map_system = ecs_world->system()
                                   .kind(flecs::PostUpdate)
                                   .run([&](flecs::iter &it)
                                        {
                                            // Update the map
                                            map->update(player_orient, cat_tex); //
                                        });

    flecs::system map_pos_system = ecs_world->system()
                                       .kind(flecs::PostUpdate)
                                       .run([&](flecs::iter &it)
                                            {
                                                // Update where the map should be drawn
                                                MapPosSystem(); //
                                            });

    // flecs::system part_system = ecs_world->system()
    //                                 .kind(flecs::PostUpdate)
    //                                 .run([&](flecs::iter &it)
    //                                      {
    //                                          // Update where the map should be drawn
    //                                          ParticleSystem(); //
    //                                      });

    flecs::system render_system = ecs_world->system()
                                      .kind(flecs::PostUpdate)
                                      .run([&](flecs::iter &it)
                                           {
                                               RenderSystem(); //
                                           });
}

// Reset the game
void App::gameReset()
{
    // Reset map and checkpoint
    object_map = object_reset_map;
    object_checkp_map = object_reset_map;

    player_orient = player_reset_map_orient;
    player_checkp_orient = player_reset_map_orient;

    particle_vec.clear();

    // Reset timer
    time_counter = 0.0;
}

// App update
// ======================================================================================

void App::update()
{
    // Only progress world 60 times per sec (60 fps)
    std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> work_time = time_now - last_frame;

    if (work_time.count() >= 16.67)
    {
        // Update last frame time to now
        last_frame = std::chrono::system_clock::now();

        // Progress FLECS world
        ecs_world->progress();
    }

    // Handle game music as often as possible to avoid audio clipping
    handleGameMusic();
}

// Game Audio
// ======================================================================================

// Handles switching between game music based on game state
void App::handleGameMusic()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !is_audio_initialized)
    {
        InitAudioDevice();
        is_audio_initialized = true;

        jump_sound = LoadSound("Jump 1.wav");
        SetSoundVolume(jump_sound, 0.4);

        cat_sound = LoadSound("Cat 1.wav");
        SetSoundVolume(cat_sound, 0.4);

        game_over_sound = LoadSound("Game Over II ~ v1.wav");
        SetSoundVolume(game_over_sound, 0.4);

        // Add the music in order with plt::GameMusic enum
        game_music[plt::GameMusic_MainMenu] = LoadMusicStream("music/racing_game_menu_bpm165.mp3");
        SetMusicVolume(game_music[plt::GameMusic_MainMenu], 0.4);

        game_music[plt::GameMusic_Playing] = LoadMusicStream("music/fever_stadium_bpm165.mp3");
        SetMusicVolume(game_music[plt::GameMusic_Playing], 0.4);

        game_music[plt::GameMusic_Climax] = LoadMusicStream("music/fever_stadium_climax_bpm180.mp3");
        SetMusicVolume(game_music[plt::GameMusic_Climax], 0.4);

        game_music[plt::GameMusic_Win] = LoadMusicStream("music/short_IMP.mp3");
        SetMusicVolume(game_music[plt::GameMusic_Win], 0.4);
    }

    switch (game_state)
    {
    case plt::GameState_MainMenu:
    {
        playGameMusic(game_music[plt::GameMusic_MainMenu]);
    }
    break;

    case plt::GameState_Playing:
    {
        if (player_vert_progress > 30)
        {
            playGameMusic(game_music[plt::GameMusic_Climax]);
        }
        else
        {
            playGameMusic(game_music[plt::GameMusic_Playing]);
        }
    }
    break;

    case plt::GameState_Win:
    {
        playGameMusic(game_music[plt::GameMusic_Win]);
    }
    break;

    default:
        break;
    }
}

// Plays a specific music
void App::playGameMusic(Music &mus)
{
    // Return if the audio device is not ready
    if (!IsAudioDeviceReady())
        return;

    if (!IsMusicStreamPlaying(mus))
    {
        // Pause the playing music track
        for (auto &track : game_music)
            if (IsMusicStreamPlaying(track.second))
                StopMusicStream(track.second);

        // Start playing the new song
        PlayMusicStream(mus);
    }
    else
    {
        UpdateMusicStream(mus);
    }
}

// Grid Handling
// ======================================================================================

// Moves an entity at pos by mov, then returns if the movement was successful (ie. not blocked)
bool App::gridMove(Vector2i pos, Vector2i mov)
{
    // Return false if value is out of range
    if (pos.x + mov.x < 0 || pos.x + mov.x >= object_map.size() || pos.y + mov.y < 0 || pos.y + mov.y >= object_map.back().size())
    {
        return false;
    }
    // Move from current position to destination if destination is empty, and return true
    else if (object_map[pos.x + mov.x][pos.y + mov.y] == GridVal_Empty)
    {
        object_map[pos.x + mov.x][pos.y + mov.y] = object_map[pos.x][pos.y];
        object_map[pos.x][pos.y] = GridVal_Empty;
        return true;
    }
    // If destination is occupied, return false
    else
    {
        return false;
    }
}

// Move in a specified direction infinitely until blocked, returning the info of where it stopped and what it was blocked by
MoveInfo App::infGridMove(Vector2i pos, Direction dir)
{
    // Determine which direction to move in
    Vector2i mov = {0, 0};
    switch (dir)
    {
    case Direction_Up:
        mov = {0, -1};
        break;
    case Direction_Down:
        mov = {0, 1};
        break;
    case Direction_Left:
        mov = {-1, 0};
        break;
    case Direction_Right:
        mov = {1, 0};
        break;
    default:
        break;
    }

    // Move infinitely until blocked
    while (gridMove(pos, mov))
    {
        pos.x += mov.x;
        pos.y += mov.y;
    }

    // Return the final position of the moving block and what it hit
    return MoveInfo{{pos.x, pos.y}, (GridVal)gridCheck({pos.x + mov.x, pos.y + mov.y})};
}

// Check what type of entity is at the specified location
GridVal App::gridCheck(Vector2i pos)
{
    // Return GridVal_SolidBlock if the position is out of range
    if (pos.x < 0 || pos.x >= object_map.size() || pos.y < 0 || pos.y >= object_map.back().size())
    {
        return GridVal_SolidBlock;
    }
    return (GridVal)object_map[pos.x][pos.y];
}

// Returns which cell the player is in
Vector2i App::getPlayerPos()
{
    for (int i = 0; i < object_map.size(); i++)
        for (int j = 0; j < object_map[i].size(); j++)
            if (object_map[i][j] == GridVal_Player)
                return {i, j};

    return {-1, -1};
}

// FLECS Systems
// ======================================================================================

// Handle the player
void App::PlayerSystem(flecs::entity e, plt::Player &player)
{
    // Get player's position on the grid once player wants to move
    Vector2i pos = getPlayerPos();

    // Calculate current player progress
    player_vert_progress = (float)pos.y / (float)object_map.back().size();

    // Don't try to move if not currently playing the game
    if (game_state != plt::GameState_Playing)
        return;

    // If the time runs out
    if (time_counter >= time_limit)
    {
        PlaySound(game_over_sound);
        gameReset();
        game_state = plt::GameState_Lose;
    }

    // Player Input
    if (player.move_state == plt::PlayerMvnmtState_Idle)
    {
        if (IsKeyDown(KEY_W))
            player.move_state = plt::PlayerMvnmtState_Up;
        if (IsKeyDown(KEY_S))
            player.move_state = plt::PlayerMvnmtState_Down;
        if (IsKeyDown(KEY_A))
            player.move_state = plt::PlayerMvnmtState_Left;
        if (IsKeyDown(KEY_D))
            player.move_state = plt::PlayerMvnmtState_Right;
        if (IsKeyDown(KEY_R))
        {
            object_map = object_checkp_map;
            player_orient = player_checkp_orient;
            return;
        }
    }

    // If the player isn't moving, we're done
    if (player.move_state == plt::PlayerMvnmtState_Idle)
        return;

    // Set the desired movement vector
    Direction desired_dir;
    switch (player.move_state)
    {
    case plt::PlayerMvnmtState_Up:
        desired_dir = Direction_Up;
        player_orient = Direction_Up;
        break;
    case plt::PlayerMvnmtState_Down:
        desired_dir = Direction_Down;
        player_orient = Direction_Down;
        break;
    case plt::PlayerMvnmtState_Left:
        desired_dir = Direction_Left;
        player_orient = Direction_Right;
        break;
    case plt::PlayerMvnmtState_Right:
        desired_dir = Direction_Right;
        player_orient = Direction_Left;
        break;
    default:
        break;
    }

    // Move
    MoveInfo mov_info;
    mov_info = infGridMove({pos.x, pos.y}, desired_dir);

    // After moving, player is back to idle
    player.move_state = plt::PlayerMvnmtState_Idle;

    // Handle what you were hit by
    switch (mov_info.blocked_by)
    {
        // Hit a damage block
    case GridVal_Damage:
    {
        // Create blood
        // createParticlesInCell({mov_info.final_pos.x, mov_info.final_pos.y}, 0.3, RED, 250.5);
        object_map = object_checkp_map;
        player_orient = player_checkp_orient;

        // Play jumping sound
        if (is_audio_initialized)
            PlaySound(cat_sound);

        return;
    }
    break;

        // Hit a checkpoint
    case GridVal_CheckP:
    {
        object_checkp_map = object_map;
        player_checkp_orient = player_orient;
    }
    break;

        // Hit the finish
    case GridVal_Finish:
    {
        game_state = plt::GameState_Win;
    }
    break;

    default:
        break;
    }

    // Return if we didn't actually move anywhere
    if (pos.x == mov_info.final_pos.x && pos.y == mov_info.final_pos.y)
        return;

    // Play jumping sound
    if (is_audio_initialized)
        PlaySound(jump_sound);

    // Set new position
    pos.x = mov_info.final_pos.x;
    pos.y = mov_info.final_pos.y;

    // Recalculate player progress after move
    player_vert_progress = (float)pos.y / (float)object_map.back().size();
}

// Handle the map's position on the screen
void App::MapPosSystem()
{
    Vector2 ideal_map_pos = {0, 0};
    RenderTexture2D map_tex = map->getRenderTexture();

    // Destination w and h stay the same
    map_dest.width = map_tex.texture.width * 2.5f;
    map_dest.height = map_tex.texture.height * 2.5f;

    // It will always be ideal to have the map horz. centered on the screen
    ideal_map_pos.x = screen_w / 2 - map_dest.width / 2;

    // Determine ideal vertical map position on the screen
    // --------------------------------------------------------------------------------------
    switch (game_state)
    {
    // In main menu, let just a bit of the map peak out of the top
    case plt::GameState_Lose:
    case plt::GameState_MainMenu:
    {
        ideal_map_pos.y = (map_dest.height + 20) * -1.f;
    }
    break;

    // While playing, try to vertically center on the player
    case plt::GameState_Win:
    case plt::GameState_Playing:
    {
        ideal_map_pos.y = screen_h / 2.0 - map_dest.height * player_vert_progress;
    }
    break;
    default:
        break;
    }

    // Move current_pos towards ideal_map_pos
    // --------------------------------------------------------------------------------------

    float dist_to_ideal = abs(Vector2Distance({map_dest.x, map_dest.y}, ideal_map_pos));

    map_dest.x += (ideal_map_pos.x - map_dest.x) * 0.01 * dist_to_ideal * ecs_world->delta_time();
    map_dest.y += (ideal_map_pos.y - map_dest.y) * 0.01 * dist_to_ideal * ecs_world->delta_time();
}

// Update all particles and delete ones that are done
// void App::ParticleSystem()
// {
//     for (int i = 0; i < particle_vec.size();)
//     {
//         // Make particle fall
//         particle_vec[i].pos.y += particle_vec[i].fall_speed * ecs_world->delta_time();

//         printf("%lf\n", particle_vec[i].fall_speed * ecs_world->delta_time());

//         // Determine if the particle should be erased
//         if (particle_vec[i].pos.y >= 1.2 * screen_h)
//             particle_vec.erase(particle_vec.begin() + i);
//         else
//             ++i;
//     }
// }

// void App::createParticlesInCell(Vector2i cell, float fall_speed, Color col, float density)
// {
//     // Determine how many particles should be made
//     int part_count = (int)(density * 100.0);

//     for (int i = 0; i < part_count; i++)
//     {
//         plt::ParticleBit new_particle;
//         new_particle.col = col;
//         new_particle.fall_speed = 0.1;

//         new_particle.pos.x = map_dest.x + ((float)cell.x / (float)object_map.size()) * map_dest.width;
//         new_particle.pos.y = map_dest.y + ((float)cell.y / (float)object_map.back().size()) * map_dest.height;

//         printf("%lf %lf\n", new_particle.pos.x, new_particle.pos.y);

//         particle_vec.push_back(new_particle);
//     }
// }

// void App::createParticlesOnCellEdge(Vector2i cell, Direction edge, float fall_speed, Color col, float density)
// {
// }

// Render system (onto render texture)
void App::RenderSystem()
{
    // Pre-draw
    // -------------------------------------------------------------------------------------

    // Calculate map render texture destination
    // --------------------------------------------------------------------------------------
    RenderTexture2D map_tex = map->getRenderTexture();
    Rectangle map_src = {0, 0, (float)map_tex.texture.width, (float)-map_tex.texture.height};

    // Begin rendering to the application texture
    BeginTextureMode(target);
    ClearBackground(RAYWHITE);

    // Balatro Shader
    delta_t_bal += ecs_world->delta_time();
    SetShaderValue(bal_shader, bal_shader_uni["delta_time"], &delta_t_bal, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(bal_shader);
    DrawTexture(bal_texture.texture, 0, 0, ColorAlpha(WHITE, 0.1));
    EndShaderMode();

    // Draw map shadow and map
    // --------------------------------------------------------------------------------------

    // Draw map shadow
    DrawRectangleRec(Rectangle{map_dest.x + 5, map_dest.y + 5, map_dest.width, map_dest.height}, BLACK);

    // Draw map
    DrawTexturePro(map_tex.texture,
                   map_src,
                   map_dest,
                   Vector2{0, 0},
                   0.0,
                   WHITE);

    // Draw GUI
    // --------------------------------------------------------------------------------------

    switch (game_state)
    {
    case plt::GameState_MainMenu:
    {
        // Title
        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 150, 17);
        GuiLabel(Rectangle{40 + 5, 30 + 5, screen_w - 80, 150}, "Cat Tower");
        setGuiTextStyle(absolute_font, ColorToInt(WHITE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 150, 17);
        GuiLabel(Rectangle{40, 30, screen_w - 80, 150}, "Cat Tower");

        // Title
        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{40 + 5, 170 + 5, screen_w - 80, 50}, "Do you have what it takes...");
        setGuiTextStyle(absolute_font, ColorToInt(WHITE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{40, 170, screen_w - 80, 50}, "Do you have what it takes...");

        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 60, 17);
        GuiLabel(Rectangle{40 + 5, 230 + 5, screen_w - 80, 50}, "TO CLIMB THE CAT TOWER?");
        setGuiTextStyle(absolute_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 60, 17);
        GuiLabel(Rectangle{40, 230, screen_w - 80, 50}, "TO CLIMB THE CAT TOWER?");

        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 0.f) + 5, 310 + 5, screen_w * 0.3f, 50}, "Use WASD to");
        setGuiTextStyle(absolute_font, ColorToInt(BLUE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 0.f), 310, screen_w * 0.3f, 50}, "Use WASD to");
        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 0.f) + 5, 350 + 5, screen_w * 0.3f, 50}, "slide");
        setGuiTextStyle(absolute_font, ColorToInt(BLUE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 0.f), 350, screen_w * 0.3f, 50}, "slide");

        // Draw Cat
        DrawTexturePro(cat_tex,
                       Rectangle{0, 0, 8, 8},
                       Rectangle{(screen_w * 0.23f * 0.f) + 125 + 5,
                                 420 + 5,
                                 screen_w * 0.1f,
                                 screen_w * 0.1f},
                       {0, 0},
                       0.0,
                       BLACK);
        DrawTexturePro(cat_tex,
                       Rectangle{0, 0, 8, 8},
                       Rectangle{(screen_w * 0.23f * 0.f) + 125,
                                 420,
                                 screen_w * 0.1f,
                                 screen_w * 0.1f},
                       {0, 0},
                       0.0,
                       WHITE);

        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 1.f) + 5, 310 + 5, screen_w * 0.3f, 50}, "Avoid Spikes");
        setGuiTextStyle(absolute_font, ColorToInt(ORANGE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 1.f), 310, screen_w * 0.3f, 50}, "Avoid Spikes");

        // Draw Spikes
        DrawTexturePro(ttt_tex,
                       Rectangle{1000, 688, 8, 8},
                       Rectangle{(screen_w * 0.23f * 1.f) + 100 + 5,
                                 350 + 5,
                                 screen_w * 0.15f,
                                 screen_w * 0.15f},
                       {0, 0},
                       0.0,
                       BLACK);
        DrawTexturePro(ttt_tex,
                       Rectangle{1000, 688, 8, 8},
                       Rectangle{(screen_w * 0.23f * 1.f) + 100,
                                 350,
                                 screen_w * 0.15f,
                                 screen_w * 0.15f},
                       {0, 0},
                       0.0,
                       WHITE);

        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 2.f) + 5, 310 + 5, screen_w * 0.3f, 50}, "Checkpoints");
        setGuiTextStyle(absolute_font, ColorToInt(GREEN), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 2.f), 310, screen_w * 0.3f, 50}, "Checkpoints");
        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 2.f) + 5, 350 + 5, screen_w * 0.3f, 50}, "save progress");
        setGuiTextStyle(absolute_font, ColorToInt(GREEN), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 2.f), 350, screen_w * 0.3f, 50}, "save progress");

        // Draw Checkpoint
        DrawTexturePro(ttt_tex,
                       Rectangle{760, 16, 8, 8},
                       Rectangle{(screen_w * 0.23f * 2.f) + 125 + 5,
                                 420 + 5,
                                 screen_w * 0.1f,
                                 screen_w * 0.1f},
                       {0, 0},
                       0.0,
                       BLACK);
        DrawTexturePro(ttt_tex,
                       Rectangle{760, 16, 8, 8},
                       Rectangle{(screen_w * 0.23f * 2.f) + 125,
                                 420,
                                 screen_w * 0.1f,
                                 screen_w * 0.1f},
                       {0, 0},
                       0.0,
                       WHITE);

        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 3.f) + 5, 310 + 5, screen_w * 0.3f, 50}, "Beat this");
        setGuiTextStyle(absolute_font, ColorToInt(PURPLE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 3.f), 310, screen_w * 0.3f, 50}, "Beat this");
        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 3.f) + 5, 350 + 5, screen_w * 0.3f, 50}, "TIME");
        setGuiTextStyle(absolute_font, ColorToInt(PURPLE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 3.f), 350, screen_w * 0.3f, 50}, "TIME");

        // Draw Checkpoint
        std::stringstream speedrun_limit_stream;
        speedrun_limit_stream << std::fixed << std::setprecision(2) << time_limit;

        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 3.f) + 5, 460 + 5, screen_w * 0.3f, 50}, (speedrun_limit_stream.str() + "s").c_str());
        setGuiTextStyle(absolute_font, ColorToInt(WHITE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
        GuiLabel(Rectangle{(screen_w * 0.23f * 3.f), 460, screen_w * 0.3f, 50}, (speedrun_limit_stream.str() + "s").c_str());

        // Play Button
        setGuiTextStyle(absolute_font, ColorToInt(Color{0x2B, 0x26, 0x27, 0xFF}), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
        if (GuiButton(Rectangle{screen_w * 0.23f, 580, screen_w - (screen_w * 0.5f), 100}, "PLAY"))
            game_state = plt::GameState_Playing;
    }
    break;
    case plt::GameState_Playing:
    {
        // Speedrun time counter
        // --------------------------------------------------------------------------------------
        time_counter += ecs_world->delta_time();

        std::stringstream speedrun_stream;
        speedrun_stream << std::fixed << std::setprecision(2) << time_counter;

        std::stringstream speedrun_limit_stream;
        speedrun_limit_stream << std::fixed << std::setprecision(2) << time_limit;

        // Time
        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel({50 + 1, screen_h - 200.f + 1, 200, 40}, (speedrun_stream.str() + "s").c_str());
        setGuiTextStyle(absolute_font, ColorToInt(WHITE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel({50, screen_h - 200.f, 200, 40}, (speedrun_stream.str() + "s").c_str());

        // Of
        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel({50 + 1, screen_h - 150.f + 1, 200, 40}, "OF");
        setGuiTextStyle(absolute_font, ColorToInt(YELLOW), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel({50, screen_h - 150.f, 200, 40}, "OF");

        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel({50 + 1, screen_h - 100.f + 1, 200, 40}, (speedrun_limit_stream.str() + "s").c_str());
        setGuiTextStyle(absolute_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 50, 17);
        GuiLabel({50, screen_h - 100.f, 200, 40}, (speedrun_limit_stream.str() + "s").c_str());

        // Menu Button
        setGuiTextStyle(absolute_font, ColorToInt(Color{0x2B, 0x26, 0x27, 0xFF}), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
        if (GuiButton(Rectangle{100, 100, 120, 80}, "Menu"))
        {
            game_state = plt::GameState_MainMenu;
            gameReset();
        }
    }
    break;
    case plt::GameState_Win:
    {
        DrawRectangleRec({screen_w / 2.f - screen_w * 0.3f, 0, screen_w * 0.6f, screen_h}, ColorAlpha(BLACK, 0.8f));

        // You WIN
        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
        GuiLabel(Rectangle{40 + 5, 40 + 5, screen_w - 80, 200}, "You WIN");
        setGuiTextStyle(absolute_font, ColorToInt(WHITE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
        GuiLabel(Rectangle{40, 40, screen_w - 80, 200}, "You WIN");

        // Win Time
        std::stringstream speedrun_stream;
        speedrun_stream << std::fixed << std::setprecision(2) << time_counter;

        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
        GuiLabel(Rectangle{40 + 5, 120 + 5, screen_w - 80, 200}, (speedrun_stream.str() + "s").c_str());
        setGuiTextStyle(absolute_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
        GuiLabel(Rectangle{40, 120, screen_w - 80, 200}, (speedrun_stream.str() + "s").c_str());

        // Restart Button
        setGuiTextStyle(absolute_font, ColorToInt(Color{0x2B, 0x26, 0x27, 0xFF}), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);

        if (GuiButton(Rectangle{screen_w * 0.23f, 400, screen_w - (screen_w * 0.5f), 100}, "RESTART"))
        {
            game_state = plt::GameState_Playing;
            gameReset();
        }

        // Menu Button
        setGuiTextStyle(absolute_font, ColorToInt(Color{0x2B, 0x26, 0x27, 0xFF}), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
        if (GuiButton(Rectangle{100, 100, 120, 80}, "Menu"))
        {
            game_state = plt::GameState_MainMenu;
            gameReset();
        }
    }
    break;
    case plt::GameState_Lose:
    {
        DrawRectangleRec({screen_w / 2.f - screen_w * 0.3f, 0, screen_w * 0.6f, screen_h}, ColorAlpha(BLACK, 0.8f));

        // You LOSE
        setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
        GuiLabel(Rectangle{40 + 5, 40 + 5, screen_w - 80, 200}, "You LOSE");
        setGuiTextStyle(absolute_font, ColorToInt(RED), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
        GuiLabel(Rectangle{40, 40, screen_w - 80, 200}, "You LOSE");

        // Restart Button
        setGuiTextStyle(absolute_font, ColorToInt(Color{0x2B, 0x26, 0x27, 0xFF}), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);

        if (GuiButton(Rectangle{screen_w * 0.23f, 400, screen_w - (screen_w * 0.5f), 100}, "RESTART"))
        {
            game_state = plt::GameState_Playing;
            StopSound(game_over_sound);
            gameReset();
        }

        // Menu Button
        setGuiTextStyle(absolute_font, ColorToInt(Color{0x2B, 0x26, 0x27, 0xFF}), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, lookout_font.baseSize / 3, 30);
        if (GuiButton(Rectangle{100, 100, 120, 80}, "Menu"))
        {
            game_state = plt::GameState_MainMenu;
            StopSound(game_over_sound);
            gameReset();
        }
    }
    break;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 m_pos = (Vector2){(float)GetRandomValue((GetMousePosition().x - 20) * 10, (GetMousePosition().x + 20) * 10) * .1f, (float)GetMousePosition().y};
        p_systems.emplace_back(m_pos);
    }

    p_systems.erase(
        std::remove_if(p_systems.begin(), p_systems.end(), [](ParticleSystem &sys)
                       { 
                   sys.update();
				   return sys.draw(); }),
        p_systems.end());

    EndTextureMode();
}