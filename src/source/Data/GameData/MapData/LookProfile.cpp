#include "stdafx.h"

#include "LookProfile.h"

#include "Data/GameData/ItemData/ItemJsonCommon.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <system_error>

namespace Data::Maps
{
namespace
{
using Items::ItemDataIssue;
using Items::ItemDataIssueSeverity;
using Items::Json::OrderedJson;

constexpr const char* DefaultKey = "default";
constexpr const char* MapsKey = "maps";
constexpr const char* WorldKey = "world";
constexpr const char* NameKey = "name";
constexpr const char* EnvironmentsKey = "environments";
constexpr const char* EnvironmentKey = "environment";
constexpr std::array<const char*, TimeStateCount> StateKeys = {"dawn", "day", "dusk", "night"};

// The built-in values are the renderer's behaviour before the profile existed; a doctest checks
// each against the constant it replaces.
constexpr std::array<LookValueInfo, LookValueCount> Infos = {{
    {LookValue::DiffuseScale, "lighting.diffuse.scale", 0.8, 0.0, 2.0},
    {LookValue::DiffuseAmbient, "lighting.diffuse.ambient", 0.4, 0.0, 1.0},
    {LookValue::DiffuseFloor, "lighting.diffuse.floor", 0.2, 0.0, 1.0},
    {LookValue::HighlightKnee, "lighting.highlightKnee", 0.8, 0.5, 0.9},
    {LookValue::RimStrength, "lighting.rim.strength", 0.35, 0.0, 1.0},
    {LookValue::RimPower, "lighting.rim.power", 3.0, 1.0, 8.0},
    {LookValue::SpecularScale, "lighting.specular.scale", 1.0, 0.0, 2.0},
    {LookValue::SpecularPower, "lighting.specular.power", 32.0, 4.0, 128.0},
    {LookValue::DynamicWrap, "dynamic.wrap", 0.5, 0.0, 0.5},
    {LookValue::DynamicMapSmoothing, "dynamic.mapSmoothing", 0.0, 0.0, 2.0},
    {LookValue::DynamicEffectGain, "dynamic.effectGain", 1.0, 0.0, 4.0},
    {LookValue::DynamicEffectRange, "dynamic.effectRange", 1.0, 0.5, 4.0},
    {LookValue::DynamicEffectRangeMax, "dynamic.effectRangeMax", 0.0, 0.0, 16.0},
    {LookValue::DynamicEffectFade, "dynamic.effectFade", 0.0, 0.0, 2.0},
    {LookValue::DynamicEffectCap, "dynamic.effectCap", 0.0, 0.0, 8.0},
    {LookValue::GroundShadowOpacity, "dynamic.shadowOpacity", 0.6, 0.0, 1.0},
    {LookValue::GroundShadowAngle, "dynamic.shadowAngle", 180.0, 0.0, 360.0},
    {LookValue::GroundShadowLights, "dynamic.shadowLights", 0.0, 0.0, 4.0},
    {LookValue::BloomStrength, "bloom.strength", 0.6, 0.0, 2.0},
    {LookValue::BloomLevelFalloff, "bloom.levelFalloff", 0.5, 0.25, 1.0},
    {LookValue::GradingStrength, "grading.strength", 1.0, 0.0, 1.0},
    {LookValue::AmbientOcclusionStrength, "ao.strength", 0.0, 0.0, 2.0},
    {LookValue::AmbientOcclusionRadius, "ao.radius", 60.0, 10.0, 300.0},
    {LookValue::LightShaftStrength, "shafts.strength", 0.0, 0.0, 2.0},
    {LookValue::LightShaftSkills, "shafts.skills", 1.0, 0.0, 2.0},
    {LookValue::LightShaftMap, "shafts.map", 1.0, 0.0, 2.0},
    {LookValue::SunShadowStrength, "sun.shadow", 0.0, 0.0, 1.0},
    {LookValue::SunShadowSoftness, "sun.softness", 1.0, 0.0, 2.0},
    {LookValue::SunShadowResolution, "sun.resolution", 2048.0, 1024.0, 4096.0},
    {LookValue::DynamicPhysicalFalloff, "dynamic.physicalFalloff", 0.0, 0.0, 1.0},
    {LookValue::FlatSunShadow, "dynamic.flatSunShadow", 1.0, 0.0, 1.0},
    {LookValue::DayMinutes, "time.dayMinutes", 0.0, 0.0, 240.0},
    {LookValue::Daylight, "time.daylight", 1.0, 0.0, 1.0},
    {LookValue::SunHeight, "sun.height", 63.0, 5.0, 90.0},
    {LookValue::SunStrength, "sun.strength", 1.0, 0.0, 1.0},
    {LookValue::SunWarmth, "sun.warmth", 0.0, -1.0, 1.0},
    {LookValue::SkyBrightness, "sky.brightness", 1.0, 0.0, 1.0},
    {LookValue::SkyFarLight, "sky.farLight", 0.0, 0.0, 1.0},
    {LookValue::SkyGlow, "sky.glow", 0.0, 0.0, 1.0},
    {LookValue::SkyNight, "sky.night", 0.0, 0.0, 1.0},
    {LookValue::FogDensity, "fog.density", 0.0, 0.0, 2.0},
    {LookValue::FogHeight, "fog.height", 300.0, 50.0, 2000.0},
    {LookValue::FogAnisotropy, "fog.anisotropy", 0.3, 0.0, 0.9},
    {LookValue::FogSunScatter, "fog.sunScatter", 1.0, 0.0, 4.0},
    {LookValue::FogLightScatter, "fog.lightScatter", 1.0, 0.0, 4.0},
    {LookValue::CloudStrength, "clouds.strength", 0.0, 0.0, 1.0},
    {LookValue::CloudScale, "clouds.scale", 3000.0, 500.0, 10000.0},
    {LookValue::CloudSpeed, "clouds.speed", 40.0, 0.0, 300.0},
    {LookValue::CloudCover, "clouds.cover", 0.45, 0.0, 1.0},
    {LookValue::SkyFill, "sky.fill", 0.0, 0.0, 1.0},
    {LookValue::RimSunFollow, "lighting.rim.sunFollow", 0.0, 0.0, 1.0},
    {LookValue::LinearLight, "lighting.linear", 0.0, 0.0, 1.0},
    {LookValue::QualityTier, "quality.tier", 2.0, 0.0, 2.0},
}};

constexpr bool InfosFollowLookValueOrder()
{
    for (std::size_t index = 0; index < Infos.size(); ++index)
    {
        if (static_cast<std::size_t>(Infos[index].value) != index)
        {
            return false;
        }
    }
    return true;
}
static_assert(InfosFollowLookValueOrder());

class IssueList
{
public:
    IssueList(const std::string& source, std::vector<ItemDataIssue>& issues) : m_source(source), m_issues(issues) {}

    void Error(const std::string& field, const std::string& message)
    {
        Items::Json::AddFileIssue(m_issues, m_source, ItemDataIssue::NoItem, field, message);
    }

    void Warning(const std::string& field, const std::string& message)
    {
        m_issues.push_back(
            {ItemDataIssueSeverity::Warning, m_source, ItemDataIssue::NoItem, ItemDataIssue::NoItem, field, message});
    }

private:
    const std::string& m_source;
    std::vector<ItemDataIssue>& m_issues;
};

std::string FormatNumber(double value)
{
    std::ostringstream text;
    text << value;
    return text.str();
}

std::string Field(const std::string& object, const std::string& key)
{
    return object.empty() ? key : object + "." + key;
}

bool ReadValue(const LookValueInfo& info, const OrderedJson& json, const std::string& field, LookOverrides& overrides,
               IssueList& issues)
{
    if (!json.is_number() || !std::isfinite(json.get<double>()))
    {
        issues.Error(field, "must be a number");
        return false;
    }
    const double value = json.get<double>();
    if (!IsInRange(info, value))
    {
        issues.Error(field, "is " + FormatNumber(value) + ", must be from " + FormatNumber(info.min) + " to " +
                                FormatNumber(info.max));
        return false;
    }
    overrides[info.value] = value;
    return true;
}

// Reads every key of an entry: a look value, or one of `allowed` (read by the caller). Every
// problem is reported, not only the first.
bool ReadOverrides(const OrderedJson& json, const std::string& object, std::initializer_list<const char*> allowed,
                   LookOverrides& overrides, IssueList& issues)
{
    bool valid = true;
    for (const auto& [key, value] : json.items())
    {
        if (std::any_of(allowed.begin(), allowed.end(), [&key](const char* name) { return key == name; }))
        {
            continue;
        }
        const std::optional<LookValue> lookValue = FindLookValue(key);
        if (!lookValue)
        {
            issues.Error(Field(object, key), "is not a look value");
            valid = false;
            continue;
        }
        valid = ReadValue(Info(*lookValue), value, Field(object, key), overrides, issues) && valid;
    }
    return valid;
}

// Reads a layer: its plain values and its time-of-day objects; `allowed` keys are the caller's.
bool ReadLayer(const OrderedJson& json, const std::string& object, std::initializer_list<const char*> allowed,
               LayerOverrides& layer, IssueList& issues)
{
    std::vector<const char*> keys(allowed);
    keys.insert(keys.end(), StateKeys.begin(), StateKeys.end());
    bool valid = true;
    for (const auto& [key, value] : json.items())
    {
        if (std::find_if(keys.begin(), keys.end(), [&key](const char* name) { return key == name; }) != keys.end())
        {
            continue;
        }
        const std::optional<LookValue> lookValue = FindLookValue(key);
        if (!lookValue)
        {
            issues.Error(Field(object, key), "is not a look value");
            valid = false;
            continue;
        }
        valid = ReadValue(Info(*lookValue), value, Field(object, key), layer.overrides, issues) && valid;
    }
    for (std::size_t index = 0; index < TimeStateCount; ++index)
    {
        const auto state = json.find(StateKeys[index]);
        if (state == json.end())
        {
            continue;
        }
        const std::string field = Field(object, StateKeys[index]);
        if (!state->is_object())
        {
            issues.Error(field, "must be an object of look values");
            valid = false;
            continue;
        }
        valid = ReadOverrides(*state, field, {}, layer.states[index], issues) && valid;
    }
    return valid;
}

bool CheckRootKeys(const OrderedJson& root, IssueList& issues)
{
    bool valid = true;
    for (const auto& [key, value] : root.items())
    {
        if (key != Items::Json::Keys::FormatVersion && key != DefaultKey && key != MapsKey && key != EnvironmentsKey)
        {
            issues.Error(key, "unknown field");
            valid = false;
        }
    }
    return valid;
}

bool ReadDefault(const OrderedJson& root, LayerOverrides& layer, IssueList& issues)
{
    const auto json = root.find(DefaultKey);
    if (json == root.end())
    {
        return true;
    }
    if (!json->is_object())
    {
        issues.Error(DefaultKey, "must be an object of look values");
        return false;
    }
    return ReadLayer(*json, DefaultKey, {}, layer, issues);
}

bool ReadEnvironments(const OrderedJson& root, LookProfileData& data, IssueList& issues)
{
    const auto json = root.find(EnvironmentsKey);
    if (json == root.end())
    {
        return true;
    }
    if (!json->is_object())
    {
        issues.Error(EnvironmentsKey, "must be an object of named environments");
        return false;
    }
    bool valid = true;
    for (const auto& [name, values] : json->items())
    {
        const std::string object = Field(EnvironmentsKey, name);
        if (!values.is_object())
        {
            issues.Error(object, "must be an object of look values");
            valid = false;
            continue;
        }
        EnvironmentLookOverrides environment{name, {}};
        valid = ReadLayer(values, object, {}, environment.layer, issues) && valid;
        data.environments.push_back(std::move(environment));
    }
    return valid;
}

bool ReadWorld(const OrderedJson& json, const std::string& object, int& world, IssueList& issues)
{
    const auto field = json.find(WorldKey);
    long long number = 0;
    if (field == json.end() || !field->is_number_integer() || !Items::Json::ReadWholeNumber(*field, number) ||
        number < 0 || number > MaxLookProfileWorld)
    {
        issues.Error(Field(object, WorldKey),
                     "missing or not a whole number from 0 to " + std::to_string(MaxLookProfileWorld));
        return false;
    }
    world = static_cast<int>(number);
    return true;
}

bool ReadMap(const OrderedJson& json, size_t index, std::set<int>& worlds, LookProfileData& data, IssueList& issues)
{
    const std::string entry = std::string(MapsKey) + "[" + std::to_string(index) + "]";
    if (!json.is_object())
    {
        issues.Error(entry, "must be an object with a world and its values");
        return false;
    }
    MapLookOverrides map;
    if (!ReadWorld(json, entry, map.world, issues))
    {
        return false;
    }
    const std::string object = std::string(MapsKey) + "[world " + std::to_string(map.world) + "]";
    if (!worlds.insert(map.world).second)
    {
        issues.Error(object, "is in the list more than once");
        return false;
    }
    const auto name = json.find(NameKey);
    if (name != json.end() && !name->is_string())
    {
        issues.Error(Field(object, NameKey), "must be text");
        return false;
    }
    if (const auto environment = json.find(EnvironmentKey); environment != json.end())
    {
        const auto known = environment->is_string()
                               ? std::find_if(data.environments.begin(), data.environments.end(),
                                              [&environment](const EnvironmentLookOverrides& entry)
                                              { return entry.name == environment->get<std::string>(); })
                               : data.environments.end();
        if (known == data.environments.end())
        {
            issues.Error(Field(object, EnvironmentKey), "must name one of \"environments\"");
            return false;
        }
        map.environment = static_cast<std::size_t>(known - data.environments.begin());
    }
    if (!ReadLayer(json, object, {WorldKey, NameKey, EnvironmentKey}, map.layer, issues))
    {
        return false;
    }
    data.maps.push_back(map);
    return true;
}

bool ReadMaps(const OrderedJson& root, LookProfileData& data, IssueList& issues)
{
    const auto list = root.find(MapsKey);
    if (list == root.end())
    {
        return true;
    }
    if (!list->is_array())
    {
        issues.Error(MapsKey, "must be a list of map entries");
        return false;
    }
    bool valid = true;
    std::set<int> worlds;
    for (size_t index = 0; index < list->size(); ++index)
    {
        valid = ReadMap((*list)[index], index, worlds, data, issues) && valid;
    }
    return valid;
}
} // namespace

const std::array<LookValueInfo, LookValueCount>& LookValueInfos()
{
    return Infos;
}

const LookValueInfo& Info(LookValue value)
{
    return Infos[static_cast<std::size_t>(value)];
}

std::optional<LookValue> FindLookValue(std::string_view name)
{
    const auto info =
        std::find_if(Infos.begin(), Infos.end(), [name](const LookValueInfo& entry) { return name == entry.name; });
    if (info == Infos.end())
    {
        return std::nullopt;
    }
    return info->value;
}

void LookOverrides::ApplyTo(LookValues& target) const
{
    for (std::size_t index = 0; index < LookValueCount; ++index)
    {
        if (values[index])
        {
            target.values[index] = *values[index];
        }
    }
}

LookValues BuiltInLookValues()
{
    LookValues builtIn;
    for (const LookValueInfo& info : Infos)
    {
        builtIn[info.value] = info.builtIn;
    }
    return builtIn;
}

const char* TimeStateName(TimeState state)
{
    return StateKeys[static_cast<std::size_t>(state)];
}

TimeBlend TimeStateAt(double hour)
{
    constexpr double DawnStart = 5.0;
    constexpr double DayStart = 6.0;
    constexpr double DuskStart = 20.0;
    constexpr double NightStart = 21.0;
    const double h = std::fmod(std::fmod(hour, 24.0) + 24.0, 24.0);
    // Half a transition's hour each way, eased so a state settles in smoothly.
    const auto eased = [](double from, double at)
    {
        const double t = std::clamp((at - from) * 2.0, 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    };
    if (h >= DayStart && h < DuskStart)
        return {TimeState::Day, TimeState::Day, 0.0};
    if (h >= DuskStart && h < DuskStart + 0.5)
        return {TimeState::Day, TimeState::Dusk, eased(DuskStart, h)};
    if (h >= DuskStart + 0.5 && h < NightStart)
        return {TimeState::Dusk, TimeState::Night, eased(DuskStart + 0.5, h)};
    if (h >= DawnStart && h < DawnStart + 0.5)
        return {TimeState::Night, TimeState::Dawn, eased(DawnStart, h)};
    if (h >= DawnStart + 0.5 && h < DayStart)
        return {TimeState::Dawn, TimeState::Day, eased(DawnStart + 0.5, h)};
    return {TimeState::Night, TimeState::Night, 0.0};
}

double TimeStateHour(TimeState state)
{
    switch (state)
    {
    case TimeState::Dawn:
        return 5.5;
    case TimeState::Dusk:
        return 20.5;
    case TimeState::Night:
        return 1.0;
    default:
        return 13.0;
    }
}

void ApplyQualityTier(LookValues& values)
{
    const double tier = std::round(values[LookValue::QualityTier]);
    if (tier >= 2.0)
    {
        return;
    }
    values[LookValue::LightShaftStrength] = 0.0;
    values[LookValue::SunShadowResolution] = std::min(values[LookValue::SunShadowResolution], 1024.0);
    if (tier >= 1.0)
    {
        return;
    }
    values[LookValue::SunShadowStrength] = 0.0;
    values[LookValue::FogDensity] = 0.0;
    values[LookValue::AmbientOcclusionStrength] = 0.0;
}

LookValues BlendLookValues(const LookValues& from, const LookValues& to, double amount)
{
    LookValues blended;
    for (std::size_t index = 0; index < LookValueCount; ++index)
    {
        double difference = to.values[index] - from.values[index];
        if (static_cast<LookValue>(index) == LookValue::GroundShadowAngle)
        {
            difference = std::remainder(difference, 360.0);
        }
        blended.values[index] = from.values[index] + difference * amount;
        if (static_cast<LookValue>(index) == LookValue::GroundShadowAngle)
        {
            blended.values[index] = std::fmod(blended.values[index] + 360.0, 360.0);
        }
    }
    return blended;
}

void LayerOverrides::ApplyTo(LookValues& target, std::optional<TimeState> state) const
{
    overrides.ApplyTo(target);
    if (state)
    {
        states[static_cast<std::size_t>(*state)].ApplyTo(target);
    }
}

const MapLookOverrides* LookProfileData::FindMap(int world) const
{
    const auto map =
        std::find_if(maps.begin(), maps.end(), [world](const MapLookOverrides& entry) { return entry.world == world; });
    return map != maps.end() ? &*map : nullptr;
}

LookValues LookProfileData::ForWorld(int world, std::optional<TimeState> state) const
{
    LookValues values = BuiltInLookValues();
    defaults.ApplyTo(values, state);
    if (const MapLookOverrides* map = FindMap(world))
    {
        if (map->environment && *map->environment < environments.size())
        {
            environments[*map->environment].layer.ApplyTo(values, state);
        }
        map->layer.ApplyTo(values, state);
    }
    return values;
}

void ReadLookProfileJson(std::string_view text, const std::string& source, LookProfileData& data,
                         std::vector<ItemDataIssue>& issues)
{
    data = {};
    IssueList list(source, issues);
    OrderedJson root;
    if (!Items::Json::ReadFileVersion(text, source, LookProfileFormatVersion, root, issues))
    {
        return;
    }

    LookProfileData read;
    // Non-short-circuit: every problem of the file is reported.
    const bool keys = CheckRootKeys(root, list);
    const bool defaults = ReadDefault(root, read.defaults, list);
    // Before the maps, which name them.
    const bool environments = ReadEnvironments(root, read, list);
    const bool maps = ReadMaps(root, read, list);
    if (keys && defaults && environments && maps)
    {
        data = std::move(read);
    }
}

LookProfileLoadResult LoadLookProfileFile(const std::filesystem::path& file)
{
    LookProfileLoadResult result;
    const std::string source = file.generic_string();
    // Only a file that is certainly absent selects the built-in values; one whose existence cannot be
    // checked (an inaccessible folder) is read, and the read reports the failure.
    std::error_code error;
    if (!std::filesystem::exists(file, error) && !error)
    {
        IssueList(source, result.issues).Warning("", "not found, the built-in values apply");
        return result;
    }
    const std::optional<std::string> text = Items::Json::ReadTextFile(file);
    if (!text)
    {
        IssueList(source, result.issues).Error("", "could not be read");
        return result;
    }
    ReadLookProfileJson(*text, source, result.data, result.issues);
    return result;
}
} // namespace Data::Maps
