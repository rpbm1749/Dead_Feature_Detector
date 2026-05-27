#pragma once

#include "../../include/types.hpp"

// Takes raw parsed data from ProjectData and produces ResolvedData
// Expands variable references, builds lookup maps, flags unresolvable chains
ResolvedData resolve(const ProjectData& data);