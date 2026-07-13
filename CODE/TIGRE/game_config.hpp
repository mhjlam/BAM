#ifndef game_config_hpp
#define game_config_hpp

#include "types.hpp"

enum class GameSpeed { Normal, Fast, Fastest };

struct GameConfig {
    int16     digi_volume = 127;
    int16     midi_volume = 127;
    GameSpeed game_speed  = GameSpeed::Normal;
    bool      health_bars  = false;
    bool      fast_collect = false;
};

GameConfig LoadGameConfig();
void       SaveGameConfig(const GameConfig& cfg);

#endif
