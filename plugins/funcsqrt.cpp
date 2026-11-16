#include "plugin_api.h"
#include <cmath>
#include <stdexcept>

IMPLEMENT_PLUGIN("sqrt",
    if (x < 0.0) throw std::domain_error("Invalid arg");
    *result = std::sqrt(x);
)
