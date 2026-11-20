#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>

namespace
{
HMODULE g_bridge = nullptr;
HMODULE g_anti_player_mosaic = nullptr;

void log_load_error(const wchar_t *name, const wchar_t *path)
{
    const DWORD error = GetLastError();
    wchar_t message[512] = {};
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, 0, message, static_cast<DWORD>(std::size(message)), nullptr);
    wchar_t line[1024] = {};
    swprintf_s(line, std::size(line),
               L"Dx11FsrBridgeLoader: failed to load %s path=%s error=%lu message=%s\n",
               name, path, static_cast<unsigned long>(error),
               length != 0 ? message : L"(unknown)");
    OutputDebugStringW(line);
}

HMODULE load_sibling(const wchar_t *name)
{
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (!length || length >= MAX_PATH)
    {
        log_load_error(name, L"(module path unavailable)");
        return nullptr;
    }

    wchar_t *slash = wcsrchr(path, L'\\');
    if (!slash)
    {
        log_load_error(name, L"(module path has no directory)");
        return nullptr;
    }
    slash[1] = L'\0';
    wcscat_s(path, name);
    HMODULE module = LoadLibraryW(path);
    if (!module)
        log_load_error(name, path);
    return module;
}

void load_plugins()
{
    g_bridge = load_sibling(L"Dx11FsrBridge.dll");
    OutputDebugStringW(g_bridge
        ? L"Dx11FsrBridgeLoader: loaded Dx11FsrBridge.dll\n"
        : L"Dx11FsrBridgeLoader: failed to load Dx11FsrBridge.dll (details above)\n");

    g_anti_player_mosaic = load_sibling(L"AntiPlayerMosaic.dll");
    OutputDebugStringW(g_anti_player_mosaic
        ? L"Dx11FsrBridgeLoader: loaded AntiPlayerMosaic.dll\n"
        : L"Dx11FsrBridgeLoader: failed to load AntiPlayerMosaic.dll (details above)\n");
}
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    load_plugins();
}
