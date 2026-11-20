#include "Specs.h"

namespace ut {

constexpr const char *divisionNames =
    "1/64|1/32|1/32 triplet|1/16 triplet|1/32 dotted|1/16|1/8 triplet|1/16 "
    "dotted|1/8|1/4 triplet|1/8 dotted|1/4|1/2 triplet|1/4 dotted|1/2|1 bar "
    "triplet|1/2 dotted|1 bar|2 bars|4 bars|8 bars";
const std::array<Spec, count> specs{{
#include "ParameterSpecs.inc"
}};

} // namespace ut
