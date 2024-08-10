#include <App.hpp>

void processLoopingEase(plt::LoopingEase &le, float dt)
{
    le.cur_time_left -= dt;

    if (le.cur_time_left < 0)
    {
        le.cur_time_left = le.set_time;
        le.increasing = !le.increasing;
    }

    easingFunction ease_func = getEasingFunction(le.func);
    float percentage = 1.0 - (le.cur_time_left / le.set_time);
    float progress = ease_func(percentage);

    if (le.increasing)
        le.val = Lerp(le.min_val, le.max_val, progress);
    else
        le.val = Lerp(le.max_val, le.min_val, progress);
}

// App Initialization & Destruction
// ==================================================
App::App(RenderTexture2D target)
{
    // Set screen w and h
    this->target = target;
    this->screen_w = target.texture.width;
    this->screen_h = target.texture.height;

    map_pos_offset = {0.0, 0.0};

    // Timing initialization
    //--------------------------------------------------------------------------------------

    // Set gameplay timer
    time_counter = 0;

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

    // Easings
    //--------------------------------------------------------------------------------------
    map_x_ease = {0.0, 20, 20, true, -1, 1, EaseInOutCubic};
    map_y_ease = {0.0, 10, 10, true, -1, 1, EaseInOutCubic};

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
    // loadTexFromImg("def.png", &def_tex);
}

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

void App::loadTexFromImg(std::string img_file, Texture2D *tex)
{
    Image img = LoadImage(img_file.c_str());
    *tex = LoadTextureFromImage(img);
    UnloadImage(img);
}

void App::update()
{
    // Handle game music as often as possible to avoid audio clipping
    handleGameMusic();

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
}

void App::initFlecsSystems()
{
    flecs::system player_system = ecs_world->system<plt::Position, plt::Player>()
                                      .kind(flecs::PreUpdate)
                                      .each([&](flecs::entity e, plt::Position &pos, plt::Player &player)
                                            {
                                                PlayerSystem(e, pos, player); //
                                            });

    flecs::system map_system = ecs_world->system()
                                   .kind(flecs::PostUpdate)
                                   .run([&](flecs::iter &it)
                                        {
                                            // Update the map
                                            map->update(); //
                                        });
    flecs::system render_system = ecs_world->system()
                                      .kind(flecs::PostUpdate)
                                      .run([&](flecs::iter &it)
                                           {
                                               RenderSystem(); //
                                           });
}

// Game Audio
// ======================================================================================

// Handles switching between game music based on game state
// --------------------------------------------------------------------------------------
void App::handleGameMusic()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !is_audio_initialized)
    {
        InitAudioDevice();
        is_audio_initialized = true;

        jump_sound = LoadSound("Jump 1.wav");

        // // Add the music
        // game_music.push_back(LoadMusicStream("music/jazzfunk.mp3"));
        // game_music.push_back(LoadMusicStream("music/nokia.mp3"));
        // game_music.push_back(LoadMusicStream("music/dance1.mp3"));
        // game_music.push_back(LoadMusicStream("music/churchcombat.mp3"));

        // game_music.push_back(LoadMusicStream("music/devil.mp3"));
        // SetMusicVolume(game_music.back(), 0.7);

        // game_music.push_back(LoadMusicStream("music/New Sunrise.mp3"));
    }

    switch (game_state)
    {
    case plt::GameState_MainMenu:
        // playGameMusic(game_music[plt::GameMusic_MainMenu]);
        break;

    default:
        break;
    }
}

// Plays a specific music
// --------------------------------------------------------------------------------------
void App::playGameMusic(Music &mus)
{
    // Return if the audio device is not ready
    if (!IsAudioDeviceReady())
        return;

    if (!IsMusicStreamPlaying(mus))
    {
        // Pause the playing music track
        for (auto &track : game_music)
            if (IsMusicStreamPlaying(track))
                StopMusicStream(track);

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

void App::PlayerSystem(flecs::entity e, plt::Position &pos, plt::Player &player)
{
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
        break;
    case plt::PlayerMvnmtState_Down:
        desired_dir = Direction_Down;
        break;
    case plt::PlayerMvnmtState_Left:
        desired_dir = Direction_Left;
        break;
    case plt::PlayerMvnmtState_Right:
        desired_dir = Direction_Right;
        break;
    default:
        break;
    }

    MoveInfo mov_info;
    mov_info = infGridMove({pos.x, pos.y}, desired_dir);

    player.move_state = plt::PlayerMvnmtState_Idle;

    // Return if we didn't actually move anywhere
    if (pos.x == mov_info.final_pos.x && pos.y == mov_info.final_pos.y)
        return;

    // Play jumping sound
    if (is_audio_initialized)
        PlaySound(jump_sound);

    pos.x = mov_info.final_pos.x;
    pos.y = mov_info.final_pos.y;
}

// Render system (onto render texture)
// --------------------------------------------------------------------------------------
void App::RenderSystem()
{
    // Pre-draw
    // --------------------------------------------------------------------------------------

    processLoopingEase(map_x_ease, ecs_world->delta_time());
    processLoopingEase(map_y_ease, ecs_world->delta_time());

    map_pos_offset.x += map_x_ease.val * 0.15;
    map_pos_offset.y += map_y_ease.val * 0.15;

    // Begin rendering to the application texture
    BeginTextureMode(target);

    ClearBackground(RAYWHITE);

    // Balatro Shader
    delta_t_bal += ecs_world->delta_time();
    SetShaderValue(bal_shader, bal_shader_uni["delta_time"], &delta_t_bal, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(bal_shader);
    DrawTexture(bal_texture.texture, 0, 0, ColorAlpha(WHITE, 0.1));
    EndShaderMode();

    // Speedrun time counter
    // --------------------------------------------------------------------------------------

    time_counter += ecs_world->delta_time();

    std::stringstream speedrun_stream;
    speedrun_stream << std::fixed << std::setprecision(2) << time_counter;

    setGuiTextStyle(lookout_font, ColorToInt(BLACK), TEXT_ALIGN_LEFT, TEXT_ALIGN_BOTTOM, 28, 30);
    GuiLabel({10 + 1, screen_h - 40.f + 1, 200, 40}, speedrun_stream.str().c_str());
    setGuiTextStyle(lookout_font, ColorToInt(WHITE), TEXT_ALIGN_LEFT, TEXT_ALIGN_BOTTOM, 28, 30);
    GuiLabel({10, screen_h - 40.f, 200, 40}, speedrun_stream.str().c_str());

    // Render the map to the screen
    RenderTexture2D map_tex = map->getRenderTexture();

    Rectangle map_src = {0, 0, (float)map_tex.texture.width, (float)-map_tex.texture.height};

    Rectangle map_dest;
    map_dest.width = map_tex.texture.width * 4;
    map_dest.height = map_tex.texture.width * 4;

    // Determine x,y based on the calculated w,h
    map_dest.x = floor(screen_w / 2 - map_dest.width / 2 + map_pos_offset.x);
    map_dest.y = floor(screen_h / 2 - map_dest.height / 2 + map_pos_offset.y);

    DrawRectangleRec(Rectangle{map_dest.x + 5, map_dest.y + 5, map_dest.width, map_dest.height}, BLACK);

    DrawTexturePro(map_tex.texture,
                   map_src,
                   map_dest,
                   Vector2{0, 0},
                   0.0,
                   WHITE);

    setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
    GuiLabel(Rectangle{40 + 5, 40 + 5, screen_w - 80, 200}, "AARON SMELLS");
    setGuiTextStyle(absolute_font, ColorToInt(WHITE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
    GuiLabel(Rectangle{40, 40, screen_w - 80, 200}, "AARON SMELLS");

    setGuiTextStyle(absolute_font, ColorToInt(BLACK), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
    GuiLabel(Rectangle{40 + 5, 450 + 5, screen_w - 80, 200}, "LIKE FLOWERS");
    setGuiTextStyle(absolute_font, ColorToInt(WHITE), TEXT_ALIGN_CENTER, TEXT_ALIGN_MIDDLE, 100, 17);
    GuiLabel(Rectangle{40, 450, screen_w - 80, 200}, "LIKE FLOWERS");

    EndTextureMode();
}