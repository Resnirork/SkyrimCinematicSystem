#pragma once
#include "pch.h"
#include <limits>

namespace CinematicSystem {

enum class ActionType { Move, Rotate, FOV, Time };

enum class ValueMode { Relative, Absolute };

struct CameraAction {
    ActionType type{};
    ValueMode mode{};
    float start{0.0f};
    float duration{0.0f};
    RE::NiPoint3 vector{};
    std::vector<RE::NiPoint3> points;
    float scalar{0.0f};
    std::size_t line{0};
};

struct CameraScript {
    std::string filename;
    std::vector<CameraAction> actions;
    float duration{0.0f};
};

class Parser {
public:
    static std::optional<CameraScript> Parse(const std::filesystem::path& path, std::string& error);

private:
    static std::string Trim(std::string value);
    static bool ParseFloat(std::string_view text, float& out);
    static std::vector<std::string> SplitTopLevel(std::string_view text);
    static bool ParsePoint(std::string_view text, ValueMode& mode, RE::NiPoint3& out);
};

}  // namespace CinematicSystem
