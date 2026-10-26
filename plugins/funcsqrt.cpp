#include "plugin_api.h"
#include <cmath>
#include <stdexcept>

static int SqrtFunc(double x, double* result) {
    if (!result) return -1;
    try {
        if (x < 0.0) {
            throw std::domain_error("Square root of negative number");
        }
        *result = std::sqrt(x);
        return 0;
    }
    catch (...) {
        return -1;
    }
}

static const Plugin kPluginInstance = {
    1,
    "sqrt",
    &SqrtFunc
};

PLUGIN_EXPORT const Plugin* GetPlugin() {
    return &kPluginInstance;
}
