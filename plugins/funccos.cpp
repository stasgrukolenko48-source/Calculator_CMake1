#include "plugin_api.h"
#include <cmath>
#include <stdexcept>

IMPLEMENT_PLUGIN("cos",
    if (!std::isfinite(x)) throw std::invalid_argument("Invalid argument");
    const double pi = std::acos(-1.0);
    double val = std::cos(x * pi / 180.0);
    if (std::abs(val) < 1e-15) val = 0.0;
    *result = val;
)
