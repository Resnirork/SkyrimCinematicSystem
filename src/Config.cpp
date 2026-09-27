#include "Config.h"

#include "pch.h"
#include <REX/W32/KERNEL32.h>
#include <spdlog/spdlog.h>
#include <charconv>

namespace CinematicSystem::Config {
namespace {
const std::string kConfigPath = "Data/SKSE/Plugins/SkyrimCinematicSystem.ini";
}

void Load()
{
    std::array<char, 16> valueBuffer{};
    REX::W32::GetPrivateProfileStringA("Debug", "Loglevel", "5", valueBuffer.data(),
                                       static_cast<std::uint32_t>(valueBuffer.size()), kConfigPath.c_str());
    const std::string_view valueText(valueBuffer.data());
    std::int32_t configuredLevel = 5;
    const auto [parseEnd, parseError] = std::from_chars(valueText.data(), valueText.data() + valueText.size(), configuredLevel);
    if (parseError != std::errc{} || parseEnd != valueText.data() + valueText.size()) {
        spdlog::warn("invalid [Debug] Loglevel '{}'; using debug (5).", valueText);
        configuredLevel = 5;
    }

    constexpr std::array levels{
        spdlog::level::off,
        spdlog::level::critical,
        spdlog::level::err,
        spdlog::level::warn,
        spdlog::level::info,
        spdlog::level::debug,
        spdlog::level::trace
    };
    if (configuredLevel < 0 || configuredLevel >= static_cast<std::int32_t>(levels.size())) {
        spdlog::warn("invalid [Debug] Loglevel {}; using debug (5).", configuredLevel);
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::debug);
        return;
    }

    const auto level = levels[static_cast<std::size_t>(configuredLevel)];
    spdlog::set_level(level);
    spdlog::flush_on(level);
    spdlog::info("configured log level {}.", configuredLevel);
}

}  // namespace CinematicSystem::Config