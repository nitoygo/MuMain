#pragma once

#include "Data/GameData/ItemData/ItemDataIssue.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The look profile of the Enhanced rendering features (Data/Render/Look.json):
//
//   { "formatVersion": 1,
//     "default": { "lighting.rim.strength": 0.35, "night": { "sun.strength": 0.3 } },
//     "environments": { "enclosed": { "sun.shadow": 0 } },
//     "maps": [ { "world": 1, "name": "Dungeon", "environment": "enclosed", "grading.strength": 0.8 } ] }
//
// Every value has a built-in value (the renderer's behaviour without the file)
// and a range. The values apply in the order built-in, "default", the map's
// environment, the map's entry; each may name any subset of them. "world" is
// the map number as in LightMood.json, "name" is for the reader only.
// "environment" names one of "environments": the kind of place a map is.
//
// The default, an environment and a map entry may each hold "dawn", "day",
// "dusk" and "night" objects: the values of that time of day (TimeState). In
// that state, each layer's own object applies right after the layer, so a map's
// plain value still wins over the default's night value. While the clock runs
// the values blend between the two states around the hour (TimeStateAt).
//
// A file with an unknown name, a value of the wrong type or out of range, a
// missing or unsupported formatVersion, a duplicate world, a world outside 0 to
// 255 or an unknown environment is unusable as a whole: the built-in values
// apply everywhere.
namespace Data::Maps
{
enum class LookValue : std::uint8_t
{
    DiffuseScale,
    DiffuseAmbient,
    DiffuseFloor,
    HighlightKnee,
    RimStrength,
    RimPower,
    SpecularScale,
    SpecularPower,
    DynamicWrap,
    DynamicMapSmoothing,
    DynamicEffectGain,
    DynamicEffectRange,
    DynamicEffectRangeMax,
    DynamicEffectFade,
    DynamicEffectCap,
    GroundShadowOpacity,
    GroundShadowAngle,
    GroundShadowLights,
    BloomStrength,
    BloomLevelFalloff,
    GradingStrength,
    AmbientOcclusionStrength,
    AmbientOcclusionRadius,
    LightShaftStrength,
    LightShaftSkills,
    LightShaftMap,
    SunShadowStrength,
    SunShadowSoftness,
    SunShadowResolution,
    DynamicPhysicalFalloff,
    FlatSunShadow,
    DayMinutes,
    Daylight,
    SunHeight,
    SunStrength,
    SunWarmth,
    SkyBrightness,
    SkyFarLight,
    SkyGlow,
    SkyNight,
    FogDensity,
    FogHeight,
    FogAnisotropy,
    FogSunScatter,
    FogLightScatter,
    CloudStrength,
    CloudScale,
    CloudSpeed,
    CloudCover,
    SkyFill,
    RimSunFollow,
    LinearLight,
    QualityTier,
    Count,
};

inline constexpr std::size_t LookValueCount = static_cast<std::size_t>(LookValue::Count);

struct LookValueInfo
{
    LookValue value;
    const char* name;
    double builtIn;
    double min;
    double max;
};

[[nodiscard]] constexpr bool IsInRange(const LookValueInfo& info, double value)
{
    return value >= info.min && value <= info.max;
}

// Every value with its name in the file, its built-in value and its range, in LookValue order.
[[nodiscard]] const std::array<LookValueInfo, LookValueCount>& LookValueInfos();
[[nodiscard]] const LookValueInfo& Info(LookValue value);
// The value with that name in the file, if there is one.
[[nodiscard]] std::optional<LookValue> FindLookValue(std::string_view name);

// A full set of values, indexed by LookValue.
struct LookValues
{
    std::array<double, LookValueCount> values{};

    [[nodiscard]] double operator[](LookValue value) const
    {
        return values[static_cast<std::size_t>(value)];
    }
    double& operator[](LookValue value)
    {
        return values[static_cast<std::size_t>(value)];
    }
    bool operator==(const LookValues&) const = default;
};

// The values an entry of the file names; the others are left to the layer below.
struct LookOverrides
{
    std::array<std::optional<double>, LookValueCount> values{};

    [[nodiscard]] const std::optional<double>& operator[](LookValue value) const
    {
        return values[static_cast<std::size_t>(value)];
    }
    std::optional<double>& operator[](LookValue value)
    {
        return values[static_cast<std::size_t>(value)];
    }
    // Replaces the values of `target` this entry names.
    void ApplyTo(LookValues& target) const;
    bool operator==(const LookOverrides&) const = default;
};

// The times of day, each with its own values.
enum class TimeState : std::uint8_t
{
    Dawn,
    Day,
    Dusk,
    Night,
    Count,
};
inline constexpr std::size_t TimeStateCount = static_cast<std::size_t>(TimeState::Count);
// The key of the state's object in the file.
[[nodiscard]] const char* TimeStateName(TimeState state);

// Where the hour (0 to 24) stands between two states: `from` blended `amount` (0 to 1) towards `to`.
// Day 06 to 20 and night 21 to 05 hold; dawn (05 to 06) and dusk (20 to 21) peak at their middle and
// blend into their neighbours on either side, in the proportions of Guild Wars 2's day.
struct TimeBlend
{
    TimeState from = TimeState::Day;
    TimeState to = TimeState::Day;
    double amount = 0.0;

    bool operator==(const TimeBlend&) const = default;
};
[[nodiscard]] TimeBlend TimeStateAt(double hour);
// The hour in the middle of the state: what `$env` shows.
[[nodiscard]] double TimeStateHour(TimeState state);

// quality.tier's limits over `values`: 0 (Low) turns off sun shadows (characters cast their flat shadow),
// the volumetric fog, ambient occlusion and light shafts, and takes the smallest shadow map; 1 (Medium)
// turns off light shafts and takes at most a 1024 shadow map; 2 (High) leaves everything.
void ApplyQualityTier(LookValues& values);

// `from` moved `amount` (0 to 1) towards `to`, value by value; dynamic.shadowAngle the short way round.
[[nodiscard]] LookValues BlendLookValues(const LookValues& from, const LookValues& to, double amount);

// A layer of the file: its plain values and those of each time of day.
struct LayerOverrides
{
    LookOverrides overrides;
    std::array<LookOverrides, TimeStateCount> states{};

    // The plain values, then the state's (none: the plain values only).
    void ApplyTo(LookValues& target, std::optional<TimeState> state) const;
    bool operator==(const LayerOverrides&) const = default;
};

struct EnvironmentLookOverrides
{
    std::string name;
    LayerOverrides layer;

    bool operator==(const EnvironmentLookOverrides&) const = default;
};

struct MapLookOverrides
{
    int world = 0;
    // Index into LookProfileData::environments.
    std::optional<std::size_t> environment;
    LayerOverrides layer;

    bool operator==(const MapLookOverrides&) const = default;
};

struct LookProfileData
{
    LayerOverrides defaults;
    std::vector<EnvironmentLookOverrides> environments;
    std::vector<MapLookOverrides> maps;

    // Built-in, then the default, the world's environment and the world's entry: their plain
    // values, or with `state` each layer's values of that time of day too.
    [[nodiscard]] LookValues ForWorld(int world, std::optional<TimeState> state = std::nullopt) const;
    [[nodiscard]] const MapLookOverrides* FindMap(int world) const;
    bool operator==(const LookProfileData&) const = default;
};

[[nodiscard]] LookValues BuiltInLookValues();

constexpr int LookProfileFormatVersion = 1;
// The server sends the map number as one byte.
constexpr int MaxLookProfileWorld = 255;

// The file, relative to the client folder.
constexpr const char* LookProfileFile = "Data/Render/Look.json";

// Reads the profile of the file into `data`. One error makes the whole file
// unusable: `data` is then empty (every value built-in) and `issues` has
// errors (Items::HasErrors).
void ReadLookProfileJson(std::string_view text, const std::string& source, LookProfileData& data,
                         std::vector<Items::ItemDataIssue>& issues);

struct LookProfileLoadResult
{
    LookProfileData data;
    std::vector<Items::ItemDataIssue> issues;
};

// Reads the file. A missing file gives the built-in values with a warning; an
// unreadable or invalid one gives them with errors.
LookProfileLoadResult LoadLookProfileFile(const std::filesystem::path& file);
} // namespace Data::Maps
