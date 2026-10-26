#include "plugin_api.h"
#include <cmath>
#include <stdexcept>

static int ExpFunc(double x, double* result) {
    if (!result) return -1;
    try {
        double val = std::exp(x);
        if (!std::isfinite(val)) {
            throw std::overflow_error("Exponential overflow");
        }
        *result = val;
        return 0;
    }
    catch (...) {
        return -1;
    }
}

static const Plugin kPluginInstance = {
    1,
    "exp",
    &ExpFunc
};

PLUGIN_EXPORT const Plugin* GetPlugin() {
    return &kPluginInstance;
}

