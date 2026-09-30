#include "CameraScript.h"

namespace CinematicSystem {

std::string Parser::Trim(std::string value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool Parser::ParseFloat(std::string_view text, float& out)
{
    try {
        std::size_t used = 0;
        const std::string s(text);
        out = std::stof(s, &used);
        return used == s.size() && std::isfinite(out);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> Parser::SplitTopLevel(std::string_view text)
{
    std::vector<std::string> result;
    std::string current;
    int depth = 0;
    for (const char ch : text) {
        if (ch == '[' || ch == '{') ++depth;
        else if (ch == ']' || ch == '}') --depth;

        if (ch == ',' && depth == 0) {
            result.push_back(Trim(std::move(current)));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    result.push_back(Trim(std::move(current)));
    return result;
}

bool Parser::ParsePoint(std::string_view text, ValueMode& mode, RE::NiPoint3& out)
{
    const auto value = Trim(std::string(text));
    if (value.size() < 5) return false;

    if (value.front() == '[' && value.back() == ']') mode = ValueMode::Relative;
    else if (value.front() == '{' && value.back() == '}') mode = ValueMode::Absolute;
    else return false;

    const auto nums = SplitTopLevel(std::string_view(value).substr(1, value.size() - 2));
    if (nums.size() != 3) return false;

    return ParseFloat(nums[0], out.x) && ParseFloat(nums[1], out.y) && ParseFloat(nums[2], out.z);
}

std::optional<CameraScript> Parser::Parse(const std::filesystem::path& path, std::string& error)
{
    std::ifstream in(path);
    if (!in) {
        logs::error("cannot open script '{}'.", path.string());
        error = "cannot open script";
        return std::nullopt;
    }

    CameraScript script;
    script.filename = path.filename().string();
    std::string line;
    std::size_t lineNo = 0;

    while (std::getline(in, line)) {
        ++lineNo;
        const auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        line = Trim(std::move(line));
        if (line.empty()) continue;

        const auto fields = SplitTopLevel(line);
        if (fields.empty()) continue;
        const auto& cmd = fields[0];

        if (cmd == "rst") {
            if (fields.size() != 2) {
                logs::error("{} line {}: rst requires start.", path.filename().string(), lineNo);
                error = "rst requires start";
                return std::nullopt;
            }
            float start;
            if (!ParseFloat(fields[1], start) || start < 0.0f) {
                logs::error("{} line {}: invalid rst start '{}'.", path.filename().string(), lineNo, fields[1]);
                error = "invalid rst start";
                return std::nullopt;
            }
            CameraAction a;
            a.type = ActionType::Move;
            a.mode = ValueMode::Absolute;
            a.start = start;
            a.duration = 0.0f;
            a.line = lineNo;
            a.scalar = std::numeric_limits<float>::quiet_NaN();
            script.actions.push_back(std::move(a));
            script.duration = script.duration < start ? start : script.duration;
            continue;
        }

        if (fields.size() < 4) {
            logs::error("{} line {}: too few comma-separated fields in '{}'.", path.filename().string(), lineNo, line);
            error = "too few comma-separated fields";
            return std::nullopt;
        }

        float start, duration;
        if (!ParseFloat(fields[1], start) || !ParseFloat(fields[2], duration) || start < 0.0f || duration < 0.0f) {
            logs::error("{} line {}: invalid start '{}' or duration '{}' (both must be finite and non-negative).",
                        path.filename().string(), lineNo, fields[1], fields[2]);
            error = "invalid start/duration";
            return std::nullopt;
        }

        CameraAction a;
        a.start = start;
        a.duration = duration;
        a.line = lineNo;

        if (cmd == "spl") {
            if (fields.size() < 5) {
                logs::error("{} line {}: spl requires at least 2 points.", path.filename().string(), lineNo);
                error = "spl requires at least 2 points";
                return std::nullopt;
            }
            if (fields.size() - 3 > 255) {
                logs::error("{} line {}: spl supports at most 255 points.", path.filename().string(), lineNo);
                error = "spl supports at most 255 points";
                return std::nullopt;
            }

            ValueMode mode{};
            bool modeSet = false;
            a.type = ActionType::Move;
            a.points.reserve(fields.size() - 3);
            for (std::size_t i = 3; i < fields.size(); ++i) {
                RE::NiPoint3 point{};
                ValueMode pointMode{};
                if (!ParsePoint(fields[i], pointMode, point)) {
                    logs::error("{} line {}: invalid spline point {} '{}'.", path.filename().string(), lineNo, i - 2, fields[i]);
                    error = "invalid spline point";
                    return std::nullopt;
                }
                if (!modeSet) {
                    mode = pointMode;
                    modeSet = true;
                } else if (pointMode != mode) {
                    logs::error("{} line {}: spline point '{}' mixes [] and {{}} modes; all points must use the same mode.",
                                path.filename().string(), lineNo, fields[i]);
                    error = "inconsistent spline point mode";
                    return std::nullopt;
                }
                a.points.push_back(point);
            }
            a.mode = mode;
        } else {
            if (fields.size() != 4) {
                logs::error("{} line {}: expected 4 comma-separated fields.", path.filename().string(), lineNo);
                error = "expected 4 comma-separated fields";
                return std::nullopt;
            }

            const auto& value = fields[3];
            if (value.size() < 3) {
                logs::error("{} line {}: invalid value '{}'.", path.filename().string(), lineNo, value);
                error = "invalid value";
                return std::nullopt;
            }
            if (value.front() == '[' && value.back() == ']') a.mode = ValueMode::Relative;
            else if (value.front() == '{' && value.back() == '}') a.mode = ValueMode::Absolute;
            else {
                logs::error("{} line {}: value must use [] or {{}}.", path.filename().string(), lineNo);
                error = "value must use [] or {}";
                return std::nullopt;
            }

            const auto nums = SplitTopLevel(std::string_view(value).substr(1, value.size() - 2));

            if (cmd == "mov" || cmd == "rot") {
                if (nums.size() != 3 && !(cmd == "rot" && nums.size() == 2)) {
                    logs::error("{} line {}: invalid {} vector '{}'.", path.filename().string(), lineNo, cmd, value);
                    error = "invalid vector";
                    return std::nullopt;
                }
                float x, y = 0.0f, z;
                if (!ParseFloat(nums[0], x) || !ParseFloat(nums.back(), z)) {
                    logs::error("{} line {}: invalid numeric value in {} vector '{}'.", path.filename().string(), lineNo, cmd, value);
                    error = "invalid numeric value";
                    return std::nullopt;
                }
                if (cmd == "mov") {
                    if (!ParseFloat(nums[1], y)) {
                        logs::error("{} line {}: invalid Y value '{}' in mov vector.", path.filename().string(), lineNo, nums[1]);
                        error = "invalid Y value";
                        return std::nullopt;
                    }
                    a.type = ActionType::Move;
                    a.vector = { x, y, z };
                } else {
                    a.type = ActionType::Rotate;
                    a.vector = { x, z, 0.0f };
                }
            } else if (cmd == "fov" || cmd == "tim") {
                if (nums.size() != 1 || !ParseFloat(nums[0], a.scalar)) {
                    logs::error("{} line {}: invalid {} scalar '{}'.", path.filename().string(), lineNo, cmd, value);
                    error = "invalid scalar";
                    return std::nullopt;
                }
                a.type = cmd == "fov" ? ActionType::FOV : ActionType::Time;
            } else {
                logs::error("{} line {}: unknown command '{}'.", path.filename().string(), lineNo, cmd);
                error = "unknown command";
                return std::nullopt;
            }
        }

        script.duration = script.duration < start + duration ? start + duration : script.duration;
        script.actions.push_back(std::move(a));
    }

    std::sort(script.actions.begin(), script.actions.end(), [](const auto& a, const auto& b) {
        if (a.start != b.start) return a.start < b.start;
        return a.line < b.line;
    });
    return script;
}

}  // namespace CinematicSystem
