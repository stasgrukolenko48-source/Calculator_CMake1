#pragma once
#include "calculator.h"
#include <vector>
#include <string>
#include <filesystem>
#include <windows.h>

class DynamicLibrary {
private:
    HMODULE handle_ = nullptr;
    std::string filename_;

public:
    DynamicLibrary() = default;
    explicit DynamicLibrary(const std::filesystem::path& path);
    ~DynamicLibrary();

    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&& other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

    void unload() noexcept;
    const std::string& filename() const noexcept;

    template <typename FuncPtr>
    FuncPtr getSymbol(const char* symbolName) const {
        if (!handle_) return nullptr;
        return reinterpret_cast<FuncPtr>(GetProcAddress(handle_, symbolName));
    }
};

class PluginManager {
private:
    std::vector<DynamicLibrary> loadedLibs_;
    
    static std::filesystem::path resolvePluginDirectory();
    // Этот метод решает проблему высокой цикломатической сложности!
    bool loadSinglePlugin(const std::filesystem::path& path, Calculator& calculator);

public:
    PluginManager() = default;
    ~PluginManager() = default;
    void loadAll(Calculator& calculator);
};
