#pragma once
#include "main.hpp"

namespace plt
{
    //--------------------------------------------------------------------------------------
    // Entity position
    //--------------------------------------------------------------------------------------
    struct Position
    {
        int x, y;
    };

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
        GameState_MainMenu
    };

    enum GameMusic : uint8_t
    {
        GameMusic_MainMenu
    };

    struct LoopingEase
    {
        float val;

        float set_time;
        float cur_time_left;

        bool increasing;

        float min_val;
        float max_val;

        easing_functions func;
    };
}