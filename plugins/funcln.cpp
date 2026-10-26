#include "plugin_api.h"
#include <cmath>
#include <stdexcept>

static int LnFunc(double x, double* result) {
    if (!result) return -1;
    try {
        if (x <= 0.0) {
            // исключение
            throw std::domain_error("Logarithm of non-positive number");
        }
        *result = std::log(x);
        return 0;
    }
    catch (...) {
        return -1;
    }
}

static const Plugin kPluginInstance = {
    1,
    "ln",
    &LnFunc
};

PLUGIN_EXPORT const Plugin* GetPlugin() {
    return &kPluginInstance;
}
