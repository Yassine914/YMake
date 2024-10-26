#pragma once

#include "../core/defines.h"
#include "../core/logger.h"
#include "../core/error.h"

#include "../toml/parser.h"
#include "../cache/cache.h"

#include "mt.h"

#include <filesystem>

namespace Y::Build {

enum class BuildMode
{
    DEBUG = 0,
    RELEASE,
};

enum class Compiler
{
    NONE = 0,
    UNKOWN,
    GCC,
    CLANG,
    ICC,
    MSVC,
};

enum class FileType
{
    NONE = 0,
    UNKOWN,
    C,
    CPP,
    HEADER,
    INTERMEDIARY,
    OBJ,
};

FileType GetFileType(const char *filepath);

// returns a compiler enum value from a string
Compiler WhatCompiler(const std::string &compiler);

void BuildProject(Project projects, BuildMode mode, bool cleanBuild);

} // namespace Y::Build