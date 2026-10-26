#include "plugin_api.h"
#include <cmath>
#include <stdexcept>

static int CosFunc(double x, double* result) {
    if (!result) return -1;
    try {
        if (!std::isfinite(x)) {
            throw std::invalid_argument("Invalid argument");
        }
        const double pi = std::acos(-1.0);
        double rad = x * pi / 180.0;
        double val = std::cos(rad);
        
        if (std::abs(val) < 1e-15) {
            val = 0.0;
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
    "cos",
    &CosFunc
};

PLUGIN_EXPORT const Plugin* GetPlugin() {
    return &kPluginInstance;
}

