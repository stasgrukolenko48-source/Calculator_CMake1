#include "plugin_manager.h"
#include "plugin_api.h"
#include <iostream>

namespace fs = std::filesystem;

DynamicLibrary::DynamicLibrary(const fs::path& path) : filename_(path.filename().string()) {
    UINT oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    handle_ = LoadLibraryW(path.wstring().c_str());
    SetErrorMode(oldMode);
    if (!handle_) throw std::runtime_error("Не удалось загрузить библиотеку: " + filename_);
}

DynamicLibrary::~DynamicLibrary() { unload(); }

DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
    : handle_(other.handle_), filename_(std::move(other.filename_)) {
    other.handle_ = nullptr;
}

DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
    if (this != &other) {
        unload();
        handle_ = other.handle_;
        filename_ = std::move(other.filename_);
        other.handle_ = nullptr;
    }
    return *this;
}

void DynamicLibrary::unload() noexcept {
    if (handle_) {
        FreeLibrary(handle_);
        handle_ = nullptr;
    }
}

const std::string& DynamicLibrary::filename() const noexcept { return filename_; }

fs::path PluginManager::resolvePluginDirectory() {
    fs::path localPlugins = fs::current_path() / "plugins";
    if (fs::exists(localPlugins) && fs::is_directory(localPlugins)) return localPlugins;

    wchar_t exePathBuf[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH) > 0) {
        fs::path exePlugins = fs::path(exePathBuf).parent_path() / "plugins";
        if (fs::exists(exePlugins) && fs::is_directory(exePlugins)) return exePlugins;
    }
    return localPlugins;
}

bool PluginManager::loadSinglePlugin(const fs::path& path, Calculator& calculator) {
    try {
        DynamicLibrary lib(path);
        auto getPlugin = lib.getSymbol<GetPluginFunc>("GetPlugin");
        if (!getPlugin) return false;

        const Plugin* plugin = getPlugin();
        if (!plugin || plugin->version != 1 || !plugin->name || !plugin->func) return false;

        std::string funcName(plugin->name);
        MathFunc rawFunc = plugin->func;

        calculator.registerFunction(funcName, [rawFunc, funcName](double x) -> double {
            double result = 0.0;
            if (rawFunc(x, &result) != 0) throw std::domain_error("Ошибка вычисления в функции: " + funcName);
            if (!std::isfinite(result)) throw std::overflow_error("Переполнение в функции: " + funcName);
            return result;
        });

        std::cout << "[Плагин загружен] " << funcName << " (" << lib.filename() << ")\n";
        loadedLibs_.push_back(std::move(lib));
        return true;
    } catch (...) {
        return false;
    }
}

void PluginManager::loadAll(Calculator& calculator) {
    fs::path pluginDir = resolvePluginDirectory();
    std::error_code ec;
    
    if (!fs::exists(pluginDir, ec) || !fs::is_directory(pluginDir, ec)) {
        fs::create_directory("plugins", ec);
        return;
    }

    for (const auto& entry : fs::directory_iterator(pluginDir, ec)) {
        if (ec || !entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ext == ".dll") loadSinglePlugin(entry.path(), calculator);
    }
}
