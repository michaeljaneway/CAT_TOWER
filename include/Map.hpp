#pragma once
#include "main.hpp"

struct TilesetInfo
{
    cute_tiled_tileset_t info;
    Texture2D tex;
};

struct TileLayerInfo
{
    RenderTexture2D tex;
    float opacity;
};

class Map
{
private:
    // Map
    //--------------------------------------------------------------------------------------
    cute_tiled_map_t *map;
    std::vector<std::vector<uint8_t>> *object_map;
    flecs::world *ecs_world;

    // Map Info
    //--------------------------------------------------------------------------------------
    int map_w;
    int map_h;

    int tile_w;
    int tile_h;

    // Map render target(s)
    //--------------------------------------------------------------------------------------

    // Map portion drawn behind everything else
    RenderTexture2D map_target;

    // Map Tilesets
    //--------------------------------------------------------------------------------------
    std::vector<TilesetInfo> tilesets_info;
    
    // Map Tile Layers
    //--------------------------------------------------------------------------------------
    std::vector<TileLayerInfo> tilelayers_info;

    // Methods
    //--------------------------------------------------------------------------------------

    // Load tileset textures
    void loadTilesets();

    // Load map dimensions
    void loadMapDimensions();

    // Parse through all map layers
    void parseMapLayers();

    // Parse a single tile layer
    void parseTileLayer(cute_tiled_layer_t *layer);

    // Parse a single object layer
    void parseObjLayer(cute_tiled_layer_t *layer);

public:
    // Constructor
    Map(flecs::world *ecs_world, std::vector<std::vector<uint8_t>> *object_map);

    // Destructor
    ~Map();

    // Draw the map texture to the screen
    void update();

    RenderTexture2D getRenderTexture();
};