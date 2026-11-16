#include "plugin_api.h"
#include <cmath>

IMPLEMENT_PLUGIN("exp",
    *result = std::exp(x);
)
