#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
#include <cstdio>
#include <vector>
#include <winver.h>

namespace
{
HMODULE g_bridge = nullptr;
HMODULE g_anti_player_mosaic = nullptr;

void append_log(const wchar_t *message)
{
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (!length || length >= MAX_PATH)
        return;
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!slash)
        return;
    slash[1] = L'\0';
    wcscat_s(path, L"Dx11FsrBridgeLoader.log");

    FILE *file = nullptr;
    if (_wfopen_s(&file, path, L"ab") != 0 || file == nullptr)
        return;
    fwprintf(file, L"%s", message);
    fclose(file);
}

void log_load_success(const wchar_t *name, const wchar_t *path, HMODULE module)
{
    DWORD entry_rva = 0;
    DWORD image_size = 0;
    if (module != nullptr)
    {
        auto *base = static_cast<const BYTE *>(static_cast<void *>(module));
        const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        if (dos->e_magic == IMAGE_DOS_SIGNATURE)
        {
            const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
            if (nt->Signature == IMAGE_NT_SIGNATURE)
            {
                entry_rva = nt->OptionalHeader.AddressOfEntryPoint;
                image_size = nt->OptionalHeader.SizeOfImage;
            }
        }
    }

    DWORD version_size = GetFileVersionInfoSizeW(path, nullptr);
    wchar_t version[64] = L"(unknown)";
    if (version_size != 0)
    {
        std::vector<BYTE> data(version_size);
        if (GetFileVersionInfoW(path, 0, version_size, data.data()))
        {
            VS_FIXEDFILEINFO *fixed = nullptr;
            UINT fixed_size = 0;
            if (VerQueryValueW(data.data(), L"\\", reinterpret_cast<void **>(&fixed), &fixed_size) &&
                fixed != nullptr)
            {
                swprintf_s(version, sizeof(version) / sizeof(version[0]), L"%u.%u.%u.%u",
                           HIWORD(fixed->dwFileVersionMS), LOWORD(fixed->dwFileVersionMS),
                           HIWORD(fixed->dwFileVersionLS), LOWORD(fixed->dwFileVersionLS));
            }
        }
    }

    wchar_t line[1024] = {};
    swprintf_s(line, sizeof(line) / sizeof(line[0]),
               L"Dx11FsrBridgeLoader: loaded %s path=%s module=0x%p "
               L"entry_rva=0x%lX image_size=0x%lX version=%s\n",
               name, path, static_cast<void *>(module),
               static_cast<unsigned long>(entry_rva),
               static_cast<unsigned long>(image_size), version);
    append_log(line);
}

void log_load_error(const wchar_t *name, const wchar_t *path)
{
    const DWORD error = GetLastError();
    wchar_t message[512] = {};
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, 0, message,
        static_cast<DWORD>(sizeof(message) / sizeof(message[0])), nullptr);
    wchar_t line[1024] = {};
    swprintf_s(line, sizeof(line) / sizeof(line[0]),
               L"Dx11FsrBridgeLoader: failed to load %s path=%s error=%lu message=%s\n",
               name, path, static_cast<unsigned long>(error),
               length != 0 ? message : L"(unknown)");
    append_log(line);
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
    else
        log_load_success(name, path, module);
    return module;
}

void load_plugins()
{
    g_bridge = load_sibling(L"Dx11FsrBridge.dll");
    append_log(g_bridge
        ? L"Dx11FsrBridgeLoader: loaded Dx11FsrBridge.dll\n"
        : L"Dx11FsrBridgeLoader: failed to load Dx11FsrBridge.dll\n");

    g_anti_player_mosaic = load_sibling(L"AntiPlayerMosaic.dll");
    append_log(g_anti_player_mosaic
        ? L"Dx11FsrBridgeLoader: loaded AntiPlayerMosaic.dll\n"
        : L"Dx11FsrBridgeLoader: failed to load AntiPlayerMosaic.dll\n");
}
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    load_plugins();
}
