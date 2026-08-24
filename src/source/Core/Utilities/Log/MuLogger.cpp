#include "stdafx.h"

#include "MuLogger.h"

#include <array>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace mu::log
{
namespace
{
constexpr std::array<const char*, 11> kLoggerNames = {
    "core", "network", "render", "data", "gameplay", "ui",
    "audio", "platform", "dotnet", "gameshop", "scenes"
};

struct LoggerState
{
    std::mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers;
    std::vector<spdlog::sink_ptr> sinks;
    bool initialized = false;
};

// Function-local static, not a namespace-scope global: C++ guarantees this constructs
// exactly once, thread-safely, on first actual call -- regardless of static-initialization
// order across translation units. Real bug this fixes: PacketBindings_ClientToServer.h
// (auto-generated) initializes inline globals via mu::platform::GetSymbol() at dynamic
// static-init time; if that runs before this TU's own statics would have, it can reach
// mu::log::Get() (via GetSymbol's null-handle error path) while the plain-global version of
// this state was still raw, uninitialized memory -- a real crash inside <xhash>/
// unordered_map internals, not hypothetical (reproduced and stack-walked).
LoggerState& State()
{
    static LoggerState state;
    return state;
}

std::shared_ptr<spdlog::logger> CreateLogger(LoggerState& state, const std::string& name)
{
    auto logger = std::make_shared<spdlog::logger>(name, state.sinks.begin(), state.sinks.end());
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::err);
    spdlog::register_logger(logger);
    state.loggers.emplace(name, logger);
    return logger;
}
} // namespace

void Init(const std::filesystem::path& logDirectory)
{
    LoggerState& state = State();
    std::lock_guard lock(state.mutex);
    if (state.initialized)
        return;

    const std::filesystem::path directory = logDirectory.empty() ? std::filesystem::current_path() : logDirectory;
    std::error_code error;
    std::filesystem::create_directories(directory, error);

    state.sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    state.sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>((directory / "MuError.log").string(), 5 * 1024 * 1024, 3));

    for (const char* name : kLoggerNames)
        CreateLogger(state, name);

    state.initialized = true;
}

void Shutdown()
{
    LoggerState& state = State();
    std::lock_guard lock(state.mutex);
    for (auto& [name, logger] : state.loggers)
        logger->flush();
    state.loggers.clear();
    state.sinks.clear();
    spdlog::shutdown();
    state.initialized = false;
}

std::shared_ptr<spdlog::logger> Get(const std::string& name)
{
    LoggerState& state = State();
    std::lock_guard lock(state.mutex);
    if (!state.initialized)
    {
        const std::filesystem::path directory = std::filesystem::current_path();
        state.sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        state.sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>((directory / "MuError.log").string(), 5 * 1024 * 1024, 3));
        state.initialized = true;
    }

    const auto found = state.loggers.find(name);
    return found != state.loggers.end() ? found->second : CreateLogger(state, name);
}
} // namespace mu::log
