#include "plugin_api.h"
#include <cmath>

IMPLEMENT_PLUGIN("sin",
    const double pi = std::acos(-1.0);
    double rad = x * pi / 180.0;
    double val = std::sin(rad);
    if (std::abs(val) < 1e-15) val = 0.0;
    *result = val;
)
