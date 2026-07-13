#include "game_config.hpp"
#include "toml.hpp"
#include "file.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

static void build_config_path(char* buf, size_t sz)
{
    snprintf(buf, sz, "%sconfig.toml", get_pref_dir());
}

GameConfig LoadGameConfig()
{
    GameConfig cfg;
    char path[512];
    build_config_path(path, sizeof(path));

    try {
        auto tbl = toml::parse_file(path);
        cfg.digi_volume = (int16)std::clamp(
            tbl["audio"]["digi_volume"].value_or((int64_t)127),
            (int64_t)0, (int64_t)127);
        cfg.midi_volume = (int16)std::clamp(
            tbl["audio"]["midi_volume"].value_or((int64_t)127),
            (int64_t)0, (int64_t)127);
        auto spd = tbl["game"]["gamespeed"].value_or(std::string_view("normal"));
        if      (spd == "fast")    cfg.game_speed = GameSpeed::Fast;
        else if (spd == "fastest") cfg.game_speed = GameSpeed::Fastest;
        cfg.health_bars  = tbl["game"]["health_bars"].value_or(false);
        cfg.fast_collect = tbl["game"]["fast_collect"].value_or(false);
    } catch (...) {}

    return cfg;
}

void SaveGameConfig(const GameConfig& cfg)
{
    char path[512];
    build_config_path(path, sizeof(path));

    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (...) {}

    tbl.insert_or_assign("audio", toml::table{
        {"digi_volume", (int64_t)cfg.digi_volume},
        {"midi_volume", (int64_t)cfg.midi_volume},
    });

    const char* spd = "normal";
    if      (cfg.game_speed == GameSpeed::Fast)    spd = "fast";
    else if (cfg.game_speed == GameSpeed::Fastest) spd = "fastest";
    tbl.insert_or_assign("game", toml::table{{"gamespeed", spd}, {"health_bars", cfg.health_bars}, {"fast_collect", cfg.fast_collect}});

    std::ofstream out(path);
    out << tbl << "\n";
}
