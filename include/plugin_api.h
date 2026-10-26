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