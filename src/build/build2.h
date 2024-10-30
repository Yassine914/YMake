#pragma once

#include "../core/defines.h"
#include "../core/logger.h"
#include "../core/error.h"

#include "../toml/parser.h"
#include "../cache/cache.h"

#include "mt.h"

#include <filesystem>

using namespace Y::Cache;

namespace Y::Build {

enum class BuildMode
{
    DEBUG = 0,
    RELEASE,
};

// returns a compiler enum value from a string

void BuildProject(Project projects, BuildMode mode, bool cleanBuild);

} // namespace Y::Build