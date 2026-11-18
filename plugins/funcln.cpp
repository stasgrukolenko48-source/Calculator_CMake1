#include "plugin_api.h"
#include <cmath>
#include <stdexcept>

IMPLEMENT_PLUGIN("ln",
    if (x <= 0.0) throw std::domain_error("Logarithm of non-positive number");
    *result = std::log(x);
)
