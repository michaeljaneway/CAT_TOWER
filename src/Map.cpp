#include "Map.hpp"

// Constructor
Map::Map(flecs::world *ecs_world, std::vector<std::vector<uint8_t>> *object_map)
{
    this->object_map = object_map;
    this->ecs_world = ecs_world;

    // Parse Map
    map = cute_tiled_load_map_from_file("testmap2.json", NULL);

    loadTilesets();

    loadMapDimensions();

    // Initialize map rendertarget
    map_target = LoadRenderTexture(map_w * tile_w, map_h * tile_h);

    // Initialize object_map size
    *object_map = std::vector<std::vector<uint8_t>>(map_w,
                                                    std::vector<uint8_t>(map_h));

    parseMapLayers();
}

// Map destructor
Map::~Map()
{
    UnloadRenderTexture(map_target);

    for (auto &ts_info : tilesets_info)
        UnloadTexture(ts_info.tex);

    for (auto &tl_info : tilelayers_info)
        UnloadRenderTexture(tl_info.tex);

    cute_tiled_free_map(map);
}

// Load Tileset Textures
void Map::loadTilesets()
{
    cute_tiled_tileset_t *ts_ptr = map->tilesets;
    while (ts_ptr)
    {
        // Get the tileset image's path
        std::filesystem::path ts_path(ts_ptr->image.ptr);

        // Load tileset's image
        Image ts_img = LoadImage(ts_path.filename().string().c_str());
        TilesetInfo ts_info;
        ts_info.info = *ts_ptr;

        // Load texture
        ts_info.tex = LoadTextureFromImage(ts_img);
        UnloadImage(ts_img);

        // Add to tilesets
        tilesets_info.push_back(ts_info);

        // Go onto next tileset
        ts_ptr = ts_ptr->next;
    }
}

// Load map dimensions
void Map::loadMapDimensions()
{
    map_w = map->width;
    map_h = map->height;

    tile_w = map->tilewidth;
    tile_h = map->tileheight;
}

// Parse through all map layers
void Map::parseMapLayers()
{
    // Render map layers to RenderTexture
    cute_tiled_layer_t *layer = map->layers;

    while (layer)
    {
        if (std::string("tilelayer") == layer->type.ptr)
            parseTileLayer(layer);
        else if (std::string("objectgroup") == layer->type.ptr)
            parseObjLayer(layer);
        else
            printf("Unknown layer type %s", layer->type.ptr);

        layer = layer->next;
    }
}

// Parse a single tile layer
void Map::parseTileLayer(cute_tiled_layer_t *layer)
{
    int *data = layer->data;
    int data_count = layer->data_count;

    // Create the RenderTexture2D for this layer
    TileLayerInfo layer_info;
    layer_info.tex = LoadRenderTexture(map_w * tile_w, map_h * tile_h);
    layer_info.opacity = layer->opacity;

    for (int column = 0; column < map_w; column++)
    {
        for (int row = 0; row < map_h; row++)
        {
            // Get the tile num for the tile on this layer
            int tile_data = data[map_w * row + column];

            // Rotation flags
            const int FlippedHorizontallyFlag = 0x8000'0000;
            const int FlippedVerticallyFlag = 0x4000'0000;
            const int FlippedAntiDiagonallyFlag = 0x2000'0000;

            // Get the flags
            int tile_flags = tile_data & 0xF000'0000;

            // Get the tile data without the flags
            tile_data = tile_data & 0x0FFF'FFFF;

            // We're done if the tile is empty
            if (tile_data == 0)
                continue;

            // Determine the tile's tileset
            TilesetInfo *this_tile_info;
            for (auto &tile_info : tilesets_info)
            {
                if (tile_info.info.firstgid <= tile_data && tile_data <= tile_info.info.firstgid + tile_info.info.tilecount - 1)
                {
                    this_tile_info = &tile_info;
                    break;
                }
            }

            // Determine src and destination Rectangles
            Rectangle src_rect = {(float)tile_w * ((tile_data - this_tile_info->info.firstgid) % this_tile_info->info.columns),
                                  (float)tile_h * ((tile_data - this_tile_info->info.firstgid) / this_tile_info->info.columns),
                                  (float)tile_w,
                                  (float)tile_h};

            if (FlippedHorizontallyFlag & tile_flags)
                src_rect.width *= -1;

            if (FlippedVerticallyFlag & tile_flags)
                src_rect.height *= -1;

            // if (FlippedAntiDiagonallyFlag & tile_flags)
            // {
            //     src_rect.width *= -1;
            //     src_rect.height *= -1;
            // }

            Rectangle dest_rect = {(float)column * tile_w, (float)row * tile_h, (float)tile_w, (float)tile_h};

            // Draw to the rendertexture
            BeginTextureMode(layer_info.tex);
            DrawTexturePro(this_tile_info->tex, src_rect, dest_rect, {0.f, 0.f}, 0.0, WHITE);
            EndTextureMode();
        }
    }

    // Add layer info to tilelayers_info vector
    tilelayers_info.push_back(layer_info);
}

// Parse a single object layer
void Map::parseObjLayer(cute_tiled_layer_t *layer)
{
    // Add Solid Bodies
    // --------------------------------------------------------------------------------------
    if (std::string("Collision") == layer->name.ptr)
    {
        cute_tiled_object_t *layer_obj = layer->objects;

        // Iterate over each collider object
        while (layer_obj)
        {
            // Set all of the grid spaces within the collider to GridVal_SolidBlock
            for (float temp_w = 0; temp_w < layer_obj->width; temp_w += tile_w)
                for (float temp_h = 0; temp_h < layer_obj->height; temp_h += tile_h)
                    (*object_map)[int((layer_obj->x + temp_w) / 8)][int((layer_obj->y + temp_h) / 8)] = GridVal_SolidBlock;

            layer_obj = layer_obj->next;
        }
    }

    // Add objects
    // --------------------------------------------------------------------------------------
    else if (std::string("Objects") == layer->name.ptr)
    {
        cute_tiled_object_t *layer_obj = layer->objects;

        while (layer_obj)
        {
            // Add the player at spawn
            // ==================================================
            if (std::string("Spawn") == layer_obj->name.ptr)
            {
                (*object_map)[int(layer_obj->x / 8)][int(layer_obj->y / 8)] = GridVal_Player;

                flecs::entity player_e = ecs_world->entity("Player");
                player_e.set<plt::Player>({plt::PlayerMvnmtState_Idle});
            }

            layer_obj = layer_obj->next;
        }
    }

    // Add damage
    // --------------------------------------------------------------------------------------
    else if (std::string("Damage") == layer->name.ptr)
    {
        cute_tiled_object_t *layer_obj = layer->objects;

        // Iterate over each collider object
        while (layer_obj)
        {
            // Set all of the grid spaces within the collider to damage cell
            for (float temp_w = 0; temp_w < layer_obj->width; temp_w += tile_w)
                for (float temp_h = 0; temp_h < layer_obj->height; temp_h += tile_h)
                    (*object_map)[int((layer_obj->x + temp_w) / 8)][int((layer_obj->y + temp_h) / 8)] = GridVal_Damage;

            layer_obj = layer_obj->next;
        }
    }

    // Add Checkpoints
    // --------------------------------------------------------------------------------------
    else if (std::string("Checkpoints") == layer->name.ptr)
    {
        cute_tiled_object_t *layer_obj = layer->objects;

        // Iterate over each collider object
        while (layer_obj)
        {
            // Set all of the grid spaces within the collider to checkpoint
            for (float temp_w = 0; temp_w < layer_obj->width; temp_w += tile_w)
                for (float temp_h = 0; temp_h < layer_obj->height; temp_h += tile_h)
                    (*object_map)[int((layer_obj->x + temp_w) / 8)][int((layer_obj->y + temp_h) / 8)] = GridVal_CheckP;

            layer_obj = layer_obj->next;
        }
    }

    // Add Finish
    // --------------------------------------------------------------------------------------
    else if (std::string("Finish") == layer->name.ptr)
    {
        cute_tiled_object_t *layer_obj = layer->objects;

        // Iterate over each collider object
        while (layer_obj)
        {
            // Set all of the grid spaces within the collider to finish cell
            for (float temp_w = 0; temp_w < layer_obj->width; temp_w += tile_w)
                for (float temp_h = 0; temp_h < layer_obj->height; temp_h += tile_h)
                    (*object_map)[int((layer_obj->x + temp_w) / 8)][int((layer_obj->y + temp_h) / 8)] = GridVal_Finish;

            layer_obj = layer_obj->next;
        }
    }
}

// Draw map background to the map render texture
void Map::update(Direction player_o, Texture2D player_tex)
{
    // Begin rendering to the map texture
    BeginTextureMode(map_target);
    ClearBackground(GRAY);

    // Draw map layers first
    for (int i = 0; i < tilelayers_info.size(); i++)
    {
        DrawTextureRec(tilelayers_info[i].tex.texture,
                       Rectangle{0, 0, (float)tilelayers_info[i].tex.texture.width, (float)-tilelayers_info[i].tex.texture.height},
                       Vector2{0, 0},
                       ColorAlpha(WHITE, tilelayers_info[i].opacity));
    }

    // Draw colliders and player
    for (int i = 0; i < (*object_map).size(); i++)
    {
        for (int j = 0; j < (*object_map)[i].size(); j++)
        {
            switch ((*object_map)[i][j])
            {
            case GridVal_Player:
            {
                float player_rot = 0;

                switch (player_o)
                {
                case Direction_Down:
                    player_rot = 0.f;
                    break;
                case Direction_Up:
                    player_rot = 180.f;
                    break;
                case Direction_Left:
                    player_rot = 270.f;
                    break;
                case Direction_Right:
                    player_rot = 90.f;
                    break;

                default:
                    break;
                }

                DrawTexturePro(player_tex,
                               Rectangle{0, 0, 8, 8},
                               Rectangle{i * 8.f + tile_w / 2.f, j * 8.f + tile_h / 2.f, 8, 8},
                               {tile_w / 2.f, tile_h / 2.f}, player_rot, WHITE);
            }
            break;

                // Draw colliders
            case GridVal_SolidBlock:
            {
                // DrawRectangleLines(i * 8, j * 8, 8, 8, ColorAlpha(BLUE, 0.8));
            }
            break;

            case GridVal_Empty:
            default:
                break;
            }
        }
    }

    EndTextureMode();
}

// Returns the map's render texture
RenderTexture2D Map::getRenderTexture()
{
    return map_target;
}
