#include "doctest.h"

#include "Render/Renderer/SdlGpuVolumetricFog.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
constexpr std::uint32_t SpirvMagic = 0x07230203u;
constexpr std::uint32_t OpTypeImage = 25u;
constexpr std::uint32_t StorageImage = 2u; // OpTypeImage's Sampled operand
constexpr std::uint32_t FormatRgba16f = 2u;

std::vector<std::uint32_t> ReadWords(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<std::uint32_t> words(bytes.size() / 4);
    std::memcpy(words.data(), bytes.data(), words.size() * 4);
    return words;
}

// The image format of every storage image the module declares.
std::vector<std::uint32_t> StorageImageFormats(const std::vector<std::uint32_t>& words)
{
    std::vector<std::uint32_t> formats;
    for (std::size_t i = 5; i < words.size();)
    {
        const std::uint32_t count = words[i] >> 16;
        const std::uint32_t opcode = words[i] & 0xFFFFu;
        if (count == 0)
        {
            break;
        }
        if (opcode == OpTypeImage && count >= 9 && words[i + 7] == StorageImage)
        {
            formats.push_back(words[i + 8]);
        }
        i += count;
    }
    return formats;
}
} // namespace

TEST_CASE("the fog's storage images are declared in the format the renderer allocates [render][fog]")
{
    // Vulkan needs a storage image's declared format to match its texture's.
    REQUIRE(mu::SdlGpuVolumetricFog::Format == SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT);
    const std::filesystem::path compiled =
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "src" / "shaders" / "compiled";
    for (const char* name : {"fog_volume.comp.spv", "fog_apply.comp.spv"})
    {
        CAPTURE(name);
        const std::vector<std::uint32_t> words = ReadWords(compiled / name);
        REQUIRE(words.size() > 5);
        REQUIRE(words[0] == SpirvMagic);
        const std::vector<std::uint32_t> formats = StorageImageFormats(words);
        REQUIRE(formats.size() == 1);
        CHECK(formats[0] == FormatRgba16f);
    }
}
