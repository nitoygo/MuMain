#pragma once

#include <cstdint>

namespace mu
{

class IPlatformWindow;

class MuPlatform
{
public:
    [[nodiscard]] static bool Initialize();
    static void Shutdown();

    // Named CreatePlatformWindow, not CreateWindow -- <windows.h> #defines CreateWindow to
    // CreateWindowA/CreateWindowW, so the obvious name is a hard syntax error on MSVC (fine on
    // macOS/Linux, which have no such macro -- this only surfaces on the Windows build).
    [[nodiscard]] static bool CreatePlatformWindow(const char* title, int width, int height, uint32_t flags);
    [[nodiscard]] static IPlatformWindow* GetWindow();
    [[nodiscard]] static bool PollEvents();

    static void SetFullscreen(bool fullscreen);
    static void SetMouseGrab(bool grab);
    [[nodiscard]] static bool GetDisplaySize(int& outWidth, int& outHeight);

private:
    MuPlatform() = delete;
};

} // namespace mu
