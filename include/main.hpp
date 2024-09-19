#pragma once

// Host (select the new HTML5 file):
//  >>  python -m http.server 8888 --bind 0.0.0.0
//
//      Self:               http://localhost:8888/
//      Others (Hamachi):   http://25.39.101.225:8888/
//
// Navigate to the HTML file and it will display the application

// Standard Library
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>
#include <cstdio>
#include <map>
#include <filesystem>
#include <random>
#include <sstream>
#include <queue>

// Raylib Graphics
#include "raylib.h"
#include "raymath.h"
#include "raygui.h"

#define GLSL_VERSION 100

// Flecs (Fast Entity Component System)
#include "flecs.h"

// Emscripten
#include <emscripten.h>
#include <emscripten/html5.h>

// 'Tiled'-generated Map loader (ensure map assets are in assets folder)
#include "cute/cute_tiled.h"

// Collision Detection and Resolution
#include "cute/cute_c2.hpp"

// CUSTOM CLASSES/TYPES HERE
// ===================================================================

class Map;

class App;

class Particle;

class ParticleSystem;

// CUSTOM FILES HERE
// ===================================================================

enum GridVal : uint8_t
{
    GridVal_Empty,
    GridVal_Player,
    GridVal_SolidBlock,
    GridVal_Damage,
    GridVal_CheckP,
    GridVal_Finish
};

enum Direction : uint8_t
{
    Direction_Left,
    Direction_Right,
    Direction_Up,
    Direction_Down,
};

struct Vector2i
{
    int x, y;
};

struct MoveInfo
{
    Vector2i final_pos;
    GridVal blocked_by;
};

// Raylib QOL extension  
#include "raylib_extension.hpp"

// Custom Flecs components
#include "components.hpp"

// Tiled map class
#include "Map.hpp"

// Particle System
#include "ParticleSystem.hpp"

// Main application
#include "App.hpp"