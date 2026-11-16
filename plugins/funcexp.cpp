#include "plugin_api.h"
#include <cmath>
#include <stdexcept>

IMPLEMENT_PLUGIN("exp",
    double val = std::exp(x);
    if (!std::isfinite(val)) throw std::overflow_error("Exponential overflow");
    *result = val;
)
