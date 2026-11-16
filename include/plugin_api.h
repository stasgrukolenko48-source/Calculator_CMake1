#pragma once

#include <cstdint>

using MathFunc = int (*)(double, double*);

struct Plugin {
    std::uint32_t version;
    const char* name;
    MathFunc func;
};

using GetPluginFunc = const Plugin* (*)();

#ifdef _WIN32
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C"
#endif

#define IMPLEMENT_PLUGIN(PluginNameStr, FuncLogic) \
    static int PluginFunc(double x, double* result) { \
        if (!result) return -1; \
        try { \
            FuncLogic; \
            return 0; \
        } catch (...) { \
            return -1; \
        } \
    } \
    static const Plugin kPluginInstance = { 1, PluginNameStr, &PluginFunc }; \
    PLUGIN_EXPORT const Plugin* GetPlugin() { return &kPluginInstance; }
