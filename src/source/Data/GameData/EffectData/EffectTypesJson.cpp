#include "stdafx.h"

#include "EffectTypesJson.h"

#include "Data/GameData/EffectData/EffectCreateParamsJson.h"
#include "Data/GameData/EffectData/EffectTypeSymbols.h"
#include "Data/GameData/ItemData/ItemJsonCommon.h"
#include "Data/GameData/ItemData/ItemModelJsonValues.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <map>
#include <set>
#include <tuple>

namespace Data::Effects
{
namespace
{
using Items::ItemDataIssue;
using Items::ItemDataIssueSeverity;
using Items::Json::OrderedJson;

namespace Keys
{
constexpr const char* Kind = "kind";
constexpr const char* Types = "types";
constexpr const char* Name = "name";
constexpr const char* Code = "code";
constexpr const char* DynamicLight = "dynamicLight";
constexpr const char* Intensity = "intensity";
constexpr const char* Radius = "radius";
constexpr const char* Color = "color";
constexpr const char* Shadows = "shadows";
constexpr const char* Shaft = "shaft";
} // namespace Keys

constexpr const char* UnknownField = "unknown field, ignored";

void AddIssue(std::vector<ItemDataIssue>& issues, ItemDataIssueSeverity severity, const std::string& source,
              const std::string& field, const std::string& message)
{
    issues.push_back({severity, source, ItemDataIssue::NoItem, ItemDataIssue::NoItem, field, message});
}

void AddError(std::vector<ItemDataIssue>& issues, const std::string& source, const std::string& field,
              const std::string& message)
{
    AddIssue(issues, ItemDataIssueSeverity::Error, source, field, message);
}

bool IsTypeName(std::string_view name)
{
    return Items::Json::IsName(name) && std::isalpha(static_cast<unsigned char>(name.front())) != 0;
}

bool ReadText(const OrderedJson& json, const char* key, std::string& text)
{
    const auto field = json.find(key);
    if (field == json.end() || !field->is_string())
    {
        return false;
    }
    text = field->get<std::string>();
    return true;
}

// "dark" is a flag of the model renderer and "alphaBlendMinus" a render type
// of the textures; the game keeps both in the same number, which means
// something else for the other kind of effect. Textures are the BITMAP_ types.
void WarnAboutRenderTypeOfOtherKind(const EffectCreateParams& params, const std::string& code, const std::string& field,
                                    const std::string& source, std::vector<ItemDataIssue>& issues)
{
    if (!params.renderType)
    {
        return;
    }
    const bool texture = code.rfind("BITMAP_", 0) == 0;
    if (*params.renderType == EffectRenderType::Dark && texture)
    {
        AddIssue(issues, ItemDataIssueSeverity::Warning, source, field + ".renderType",
                 "\"dark\" is for models; this type is a texture");
    }
    if (*params.renderType == EffectRenderType::AlphaBlendMinus && !texture)
    {
        AddIssue(issues, ItemDataIssueSeverity::Warning, source, field + ".renderType",
                 "\"alphaBlendMinus\" is for textures; this type is a model");
    }
}

// Reads the creation values of an effect entry, if it has them.
void ReadCreateParams(const OrderedJson& json, const std::string& field, const std::string& source,
                      EffectTypeEntry& entry, std::vector<ItemDataIssue>& issues)
{
    const auto create = json.find(CreateKey);
    if (create == json.end())
    {
        return;
    }
    const std::string createField = field + "." + CreateKey;
    if (!create->is_object())
    {
        AddError(issues, source, createField, "must be an object with the creation values");
        return;
    }
    const Items::ModelJson::ReportIssue report =
        [&](ItemDataIssueSeverity severity, const std::string& issueField, const std::string& message)
    { AddIssue(issues, severity, source, issueField, message); };
    entry.create = ReadEffectCreateParams(*create, createField, report);
    if (*entry.create == EffectCreateParams{})
    {
        AddIssue(issues, ItemDataIssueSeverity::Warning, source, createField,
                 "sets no value; the creation code of the type is skipped all the same");
    }
    WarnAboutRenderTypeOfOtherKind(*entry.create, entry.code, createField, source, issues);
    for (size_t i = 0; i < entry.create->variants.size(); ++i)
    {
        WarnAboutRenderTypeOfOtherKind(entry.create->variants[i].params, entry.code,
                                       createField + ".variants[" + std::to_string(i) + "]", source, issues);
    }
}

// A number of "dynamicLight" between `low` and `high`, or the default with an issue.
void ReadLightNumber(const OrderedJson& light, const char* key, float low, float high, const char* range,
                     const std::string& field, const std::string& source, float& value,
                     std::vector<ItemDataIssue>& issues)
{
    const auto found = light.find(key);
    if (found == light.end())
    {
        return;
    }
    if (!found->is_number() || found->get<double>() < low || found->get<double>() > high)
    {
        AddIssue(issues, ItemDataIssueSeverity::Warning, source, field + "." + key,
                 std::string("must be a number from ") + range + "; the effect's own value is kept");
        return;
    }
    value = found->get<float>();
}

// Reads how an effect entry's point lights shine, if it says.
void ReadDynamicLight(const OrderedJson& json, const std::string& entryField, const std::string& source,
                      EffectTypeEntry& entry, std::vector<ItemDataIssue>& issues)
{
    const auto light = json.find(Keys::DynamicLight);
    if (light == json.end())
    {
        return;
    }
    const std::string field = entryField + "." + Keys::DynamicLight;
    if (!light->is_object())
    {
        AddError(issues, source, field, "must be an object with the light's values");
        return;
    }
    EffectDynamicLight values;
    ReadLightNumber(*light, Keys::Intensity, MinEffectLightIntensity, MaxEffectLightIntensity, "0 to 4", field, source,
                    values.intensity, issues);
    ReadLightNumber(*light, Keys::Radius, MinEffectLightRadius, MaxEffectLightRadius, "0.25 to 4", field, source,
                    values.radius, issues);
    ReadLightNumber(*light, Keys::Shaft, MinEffectLightShaft, MaxEffectLightShaft, "0 to 4", field, source,
                    values.shaft, issues);
    if (const auto color = light->find(Keys::Color); color != light->end())
    {
        const bool valid = color->is_array() && color->size() == 3 &&
                           std::all_of(color->begin(), color->end(), [](const OrderedJson& channel)
                                       { return channel.is_number() && channel.get<double>() >= 0.0; }) &&
                           std::any_of(color->begin(), color->end(),
                                       [](const OrderedJson& channel) { return channel.get<double>() > 0.0; });
        if (valid)
        {
            values.color =
                std::array<float, 3>{(*color)[0].get<float>(), (*color)[1].get<float>(), (*color)[2].get<float>()};
        }
        else
        {
            AddIssue(issues, ItemDataIssueSeverity::Warning, source, field + "." + Keys::Color,
                     "must be [red, green, blue], none below 0 and not all 0; the effect's own colour is kept");
        }
    }
    if (const auto shadows = light->find(Keys::Shadows); shadows != light->end())
    {
        if (shadows->is_boolean())
        {
            values.shadows = shadows->get<bool>();
        }
        else
        {
            AddIssue(issues, ItemDataIssueSeverity::Warning, source, field + "." + Keys::Shadows,
                     "must be true or false; the light casts shadows");
        }
    }
    for (const auto& [key, value] : light->items())
    {
        if (key != Keys::Intensity && key != Keys::Radius && key != Keys::Color && key != Keys::Shadows &&
            key != Keys::Shaft)
        {
            AddIssue(issues, ItemDataIssueSeverity::Warning, source, field + "." + key, UnknownField);
        }
    }
    entry.dynamicLight = values;
}

// A light value as written: 4 decimals, so a float read from the file writes back as it was.
double Rounded(float value)
{
    return std::round(static_cast<double>(value) * 1e4) / 1e4;
}

OrderedJson WriteDynamicLight(const EffectDynamicLight& light)
{
    OrderedJson json = OrderedJson::object();
    const EffectDynamicLight defaults;
    if (light.intensity != defaults.intensity)
    {
        json[Keys::Intensity] = Items::ModelJson::WriteNumber(Rounded(light.intensity));
    }
    if (light.radius != defaults.radius)
    {
        json[Keys::Radius] = Items::ModelJson::WriteNumber(Rounded(light.radius));
    }
    if (light.color)
    {
        const std::array<double, 3> color{Rounded((*light.color)[0]), Rounded((*light.color)[1]),
                                          Rounded((*light.color)[2])};
        json[Keys::Color] = Items::ModelJson::WriteNumbers(color);
    }
    if (light.shadows != defaults.shadows)
    {
        json[Keys::Shadows] = light.shadows;
    }
    if (light.shaft != defaults.shaft)
    {
        json[Keys::Shaft] = Items::ModelJson::WriteNumber(Rounded(light.shaft));
    }
    return json;
}

// Reads entry `index` of "types"; false when it has no name or code. Only
// effects have creation values and dynamic lights.
bool ReadEntry(const OrderedJson& json, size_t index, const std::string& source, EffectKind kind,
               EffectTypeEntry& entry, std::vector<ItemDataIssue>& issues)
{
    const std::string field = std::string(Keys::Types) + "[" + std::to_string(index) + "]";
    if (!json.is_object())
    {
        AddError(issues, source, field, "must be an object with a name and a code");
        return false;
    }
    const auto readText = [&](const char* key, std::string& text)
    {
        if (ReadText(json, key, text))
        {
            return true;
        }
        AddError(issues, source, field + "." + key, "missing or not text");
        return false;
    };
    const bool hasName = readText(Keys::Name, entry.name);
    const bool hasCode = readText(Keys::Code, entry.code);
    const bool kindHasCreateParams = kind == EffectKind::Effect;
    if (kindHasCreateParams)
    {
        ReadCreateParams(json, field, source, entry, issues);
        ReadDynamicLight(json, field, source, entry, issues);
    }
    for (const auto& [key, value] : json.items())
    {
        if (key != Keys::Name && key != Keys::Code &&
            !(kindHasCreateParams && (key == CreateKey || key == Keys::DynamicLight)))
        {
            AddIssue(issues, ItemDataIssueSeverity::Warning, source, field + "." + key, UnknownField);
        }
    }
    return hasName && hasCode;
}

bool HasKind(const OrderedJson& root, EffectKind kind)
{
    const auto field = root.find(Keys::Kind);
    return field != root.end() && field->is_string() && field->get<std::string>() == GetEffectKindName(kind);
}

void WarnUnknownRootFields(const OrderedJson& root, const std::string& source, std::vector<ItemDataIssue>& issues)
{
    for (const auto& [key, value] : root.items())
    {
        if (key != Items::Json::Keys::FormatVersion && key != Keys::Kind && key != Keys::Types)
        {
            AddIssue(issues, ItemDataIssueSeverity::Warning, source, key, UnknownField);
        }
    }
}
} // namespace

std::string_view GetEffectTypesFileName(EffectKind kind)
{
    constexpr std::array<std::string_view, EffectKindCount> FileNames = {"EffectTypes.json", "ParticleTypes.json",
                                                                         "JointTypes.json", "SpriteTypes.json"};
    return FileNames[ToIndex(kind)];
}

void ReadEffectTypesJson(std::string_view text, const std::string& source, EffectKind kind,
                         std::vector<EffectTypeEntry>& types, std::vector<ItemDataIssue>& issues)
{
    OrderedJson root;
    if (!Items::Json::ReadFileVersion(text, source, EffectTypesFormatVersion, root, issues))
    {
        return;
    }
    if (!HasKind(root, kind))
    {
        AddError(issues, source, Keys::Kind, "missing or not \"" + std::string(GetEffectKindName(kind)) + "\"");
        return;
    }

    const auto list = root.find(Keys::Types);
    if (list == root.end() || !list->is_array())
    {
        AddError(issues, source, Keys::Types, "missing or not a list of types");
        return;
    }
    for (size_t index = 0; index < list->size(); ++index)
    {
        EffectTypeEntry entry;
        if (ReadEntry((*list)[index], index, source, kind, entry, issues))
        {
            types.push_back(std::move(entry));
        }
    }
    WarnUnknownRootFields(root, source, issues);
}

void ValidateEffectTypes(EffectKind kind, std::span<const EffectTypeEntry> types, const std::string& source,
                         std::vector<ItemDataIssue>& issues)
{
    std::set<std::string_view> codes;
    for (const EffectTypeSymbol& symbol : GetEffectTypeSymbols(kind))
    {
        codes.insert(symbol.code);
    }

    std::set<std::string_view> names;
    std::map<std::string_view, std::string_view> nameOfCode;
    for (const EffectTypeEntry& entry : types)
    {
        if (!IsTypeName(entry.name))
        {
            AddError(issues, source, entry.name, "is not a name of letters and digits that starts with a letter");
        }
        else if (!names.insert(entry.name).second)
        {
            AddError(issues, source, entry.name, "is the name of more than one type");
        }

        if (!codes.contains(entry.code))
        {
            AddError(issues, source, entry.name, "the code " + entry.code + " is no type of this kind");
            continue;
        }
        const auto [named, inserted] = nameOfCode.emplace(entry.code, entry.name);
        if (!inserted)
        {
            AddError(issues, source, entry.name, entry.code + " is already named " + std::string(named->second));
        }
    }

    for (const std::string_view code : codes)
    {
        if (!nameOfCode.contains(code))
        {
            AddError(issues, source, std::string(code), "has no name; every type the code uses needs one");
        }
    }
}

std::string WriteEffectTypesJson(EffectKind kind, std::span<const EffectTypeEntry> types)
{
    std::vector<const EffectTypeEntry*> sorted;
    for (const EffectTypeEntry& entry : types)
    {
        sorted.push_back(&entry);
    }
    std::sort(sorted.begin(), sorted.end(), [](const EffectTypeEntry* left, const EffectTypeEntry* right)
              { return std::tie(left->name, left->code) < std::tie(right->name, right->code); });

    OrderedJson root;
    root[Items::Json::Keys::FormatVersion] = EffectTypesFormatVersion;
    root[Keys::Kind] = std::string(GetEffectKindName(kind));
    root[Keys::Types] = OrderedJson::array();
    for (const EffectTypeEntry* entry : sorted)
    {
        OrderedJson json;
        json[Keys::Name] = entry->name;
        json[Keys::Code] = entry->code;
        if (entry->create && kind == EffectKind::Effect)
        {
            json[CreateKey] = WriteEffectCreateParams(*entry->create);
        }
        if (entry->dynamicLight && kind == EffectKind::Effect)
        {
            json[Keys::DynamicLight] = WriteDynamicLight(*entry->dynamicLight);
        }
        root[Keys::Types].push_back(std::move(json));
    }
    std::string text = root.dump(Items::Json::Indent, ' ', false, OrderedJson::error_handler_t::replace);
    text = Items::Json::PutListsOnOneLine(Items::Json::PutListsOnOneLine(text, CreateLightKey), CreateSubTypesKey);
    text = Items::Json::PutListsOnOneLine(text, Keys::Color);
    for (const char* key : CreateVectorKeys)
    {
        text = Items::Json::PutObjectsOnOneLine(Items::Json::PutListsOnOneLine(text, key), key);
    }
    return text + "\n";
}
} // namespace Data::Effects
