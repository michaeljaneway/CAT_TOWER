#pragma once
#include "main.hpp"

namespace plt
{
    //--------------------------------------------------------------------------------------
    // Player
    //--------------------------------------------------------------------------------------
    enum PlayerMvnmtState : uint8_t
    {
        PlayerMvnmtState_Left,
        PlayerMvnmtState_Right,
        PlayerMvnmtState_Up,
        PlayerMvnmtState_Down,
        PlayerMvnmtState_Idle
    };

    struct Player
    {
        // Movement state
        PlayerMvnmtState move_state;
    };

    //--------------------------------------------------------------------------------------
    // Game State
    //--------------------------------------------------------------------------------------

    enum GameState
    {
        GameState_MainMenu,
        GameState_Playing,
        GameState_Win,
    };

    enum GameMusic : uint8_t
    {
        GameMusic_MainMenu,
        GameMusic_Playing,
        GameMusic_Climax,
        GameMusic_Win
    };

    // Particle effect
    struct ParticleBit
    {
        Vector2 pos;
        Color col;
        float fall_speed;
    };
}