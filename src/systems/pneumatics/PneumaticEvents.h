#pragma once
#include "core/EventBus.h"

namespace a320 {

struct BleedFaultEvent  { int  sourceIndex; bool fault; };  // 0=ENG1, 1=ENG2, 2=APU
struct PackFaultEvent   { int  packIndex;   bool fault; };
struct CabinAltWarnEvent { bool excessAlt; bool excessDiff; };

} // namespace a320
