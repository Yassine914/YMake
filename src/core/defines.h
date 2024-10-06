#ifndef DEFINES_H
#define DEFINES_H

#include <corecrt.h>

// project specific
#define YMAKE_VERSION_MAJOR 0
#define YMAKE_VERSION_MINOR 1
#define YMAKE_VERSION_PATCH 1

#define YMAKE_DEFAULT_FILE "YMake.toml"
#define YMAKE_CACHE_DIR    "./YMakeCache"

// macros for toml file
#define YMAKE_MACRO_PROJECT_NAME "YM_PROJECT_NAME"
#define YMAKE_MACRO_CURRENT_DIR  "YM_CURRENT_DIR"
#define YMAKE_MACRO_SRC_DIR      "YM_SRC_DIR"
#define YMAKE_MACRO_BUILD_DIR    "YM_BUILD_DIR"

// toml file param names.
#define YMAKE_TOML_PROJECT_VERSION "version"
#define YMAKE_TOML_LANG            "lang"
#define YMAKE_TOML_CPP             "cpp"
#define YMAKE_TOML_C               "c"
#define YMAKE_TOML_STD             "std"
#define YMAKE_TOML_COMPILER        "compiler"
#define YMAKE_TOML_BUILD           "build"
#define YMAKE_TOML_TYPE            "type"
#define YMAKE_TOML_DIR             "dir"
#define YMAKE_TOML_SRC             "src"
#define YMAKE_TOML_ENV             "env"
#define YMAKE_TOML_INCLUDES        "includes"
#define YMAKE_TOML_LIBS            "libs"
#define YMAKE_TOML_BUILT           "built"
#define YMAKE_TOML_SYS             "sys"
#define YMAKE_TOML_DEBUG           "debug"
#define YMAKE_TOML_RELEASE         "release"
#define YMAKE_TOML_OPTIMIZATION    "optimization"
#define YMAKE_TOML_DEFINES         "defines"
#define YMAKE_TOML_FLAGS           "flags"
#define YMAKE_TOML_LIB_NAME        "name"
#define YMAKE_TOML_LIB_PATH        "path"
#define YMAKE_TOML_LIB_INCLUDE     "include"
#define YMAKE_TOML_LIB_TYPE        "type"

#define YMAKE_CONFIG_METADATA_CACHE_FILENAME "config.cache"
#define YMAKE_TIMESTAMP_CACHE_FILENAME       "timestamp.cache"
#define YMAKE_PROJECTS_CACHE_FILENAME        "projects.cache"
#define YMAKE_CONFIG_PATH_CACHE_FILENAME     "path.cache"
#define YMAKE_METADATA_CACHE_FILENAME        "metadata.cache"
#define YMAKE_PREPROCESS_CACHE_FILENAME      "preprocessed_metadata.cache"

// 24 hrs
#define YMAKE_TIMESTAMP_THRESHHOLD_SEC 86400

// compiler specific flags
// gcc, icc, clang => COMP_{FLAG}
// msvc => COMP_MSVC_{FLAG}
// added trailing spaces. (no space in front.)
#define COMP_COMPILE_ONLY      "-c "
#define COMP_MSVC_COMPILE_ONLY "/c "

#define COMP_OUTPUT_FILE(x)      std::string("-o ") + x + " "
#define COMP_MSVC_OUTPUT_FILE(x) std::string("/Fo") + x + " "

#define COMP_INCLUDE_DIR(x)      std::string("-I") + x + " "
#define COMP_MSVC_INCLUDE_DIR(x) std::string("/I") + x + " "

#define COMP_LIBRARY_DIR(x)      std::string("-L") + x + " "
#define COMP_MSVC_LIBRARY_DIR(x) std::string("/LIBPATH:") + x + " "

#define COMP_LINK_LIBRARY(x)      std::string("-l") + x + " "
#define COMP_MSVC_LINK_LIBRARY(x) std::string("") + x + " "

#define COMP_PREPROCESS_ONLY      "-E "
#define COMP_MSVC_PREPROCESS_ONLY "/P "

#define COMP_GENERATE_ASSEMBLY      "-S "
#define COMP_MSVC_GENERATE_ASSEMBLY "/FA "

#define COMP_OPTIMIZATION_LEVEL_0      "-O0 "
#define COMP_OPTIMIZATION_LEVEL_1      "-O1 "
#define COMP_OPTIMIZATION_LEVEL_2      "-O2 "
#define COMP_OPTIMIZATION_LEVEL_3      "-O3 "
#define COMP_MSVC_OPTIMIZATION_LEVEL_0 "/Od "
#define COMP_MSVC_OPTIMIZATION_LEVEL_1 "/O1 "
#define COMP_MSVC_OPTIMIZATION_LEVEL_2 "/O2 "
#define COMP_MSVC_OPTIMIZATION_LEVEL_3 "/Ox "

#define COMP_DEBUG_INFORMATION      "-g "
#define COMP_MSVC_DEBUG_INFORMATION "/Zi "

#define COMP_DEFINE_MACRO(x)      std::string("-D") + x + " "
#define COMP_MSVC_DEFINE_MACRO(x) std::string("/D") + x + " "

#define COMP_WARNING_LEVEL_ALL    "-Wall "
#define COMP_WARNING_LEVEL_EXTRA  "-Wextra "
#define COMP_MSVC_WARNING_LEVEL_1 "/W1 "
#define COMP_MSVC_WARNING_LEVEL_2 "/W2 "
#define COMP_MSVC_WARNING_LEVEL_3 "/W3 "
#define COMP_MSVC_WARNING_LEVEL_4 "/W4 "

#define COMP_SUPPRESS_WARNINGS      "-w "
#define COMP_MSVC_SUPPRESS_WARNINGS "/w "

#define COMP_STANDARD_VERSION_C(x)      std::string("-std=c") + std::to_string(x) + " "
#define COMP_STANDARD_VERSION_CPP(x)    std::string("-std=c++") + std::to_string(x) + " "
#define COMP_MSVC_STANDARD_VERSION_C(x) std::string("/std:c++") + std::to_string(x) + " "

#define COMP_POSITION_INDEPENDENT_CODE      "-fPIC "
#define COMP_MSVC_POSITION_INDEPENDENT_CODE ""

#define COMP_STATIC_LINKING      "-static "
#define COMP_MSVC_STATIC_LINKING "/MT "

#define COMP_SHARED_LIBRARY      "-shared "
#define COMP_MSVC_SHARED_LIBRARY "/DLL "

#define COMP_MULTITHREADING         "-pthread "
#define COMP_MSVC_MULTITHREADING_MT "/MT "
#define COMP_MSVC_MULTITHREADING_MD "/MD "

#define COMP_LINK_TIME_OPTIMIZATION      "-flto "
#define COMP_MSVC_LINK_TIME_OPTIMIZATION "/GL "

#define COMP_VERBOSE_OUTPUT      "-v "
#define COMP_MSVC_VERBOSE_OUTPUT "/VERBOSE "

#define COMP_SHOW_INCLUDES      "-H "
#define COMP_MSVC_SHOW_INCLUDES "/showIncludes "

#define COMP_PROFILE_GUIDED_OPTIMIZATION_GENERATE "-fprofile-generate "
#define COMP_PROFILE_GUIDED_OPTIMIZATION_USE      "-fprofile-use "
#define COMP_MSVC_PROFILE_GUIDED_OPTIMIZATION     "/LTCG "

// unsigned int types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef size_t usize;

// signed int types
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// floating point types
typedef float f32;
typedef double f64;

// for bit fields (accessing bit no. x)
#define BIT(x) (1 << x)

// define static assertions
#if defined(__clang__) || defined(__gcc__) || defined(__GNUC__)
    #define ST_ASSERT _Static_assert
#else
    #define ST_ASSERT static_assert
#endif

// static assertions for type sizes
ST_ASSERT(sizeof(u8) == 1, "expected u8 to be 1 byte.");
ST_ASSERT(sizeof(u16) == 2, "expected u16 to be 2 bytes.");
ST_ASSERT(sizeof(u32) == 4, "expected u32 to be 4 bytes.");
ST_ASSERT(sizeof(u64) == 8, "expected u64 to be 8 bytes.");

ST_ASSERT(sizeof(i8) == 1, "expected i8 to be 1 byte.");
ST_ASSERT(sizeof(i16) == 2, "expected i16 to be 2 bytes.");
ST_ASSERT(sizeof(i32) == 4, "expected i32 to be 4 bytes.");
ST_ASSERT(sizeof(i64) == 8, "expected i64 to be 8 bytes.");

ST_ASSERT(sizeof(f32) == 4, "expected f32 to be 4 bytes.");
ST_ASSERT(sizeof(f64) == 8, "expected f64 to be 8 bytes.");

// platform detection

// windows
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define IPLATFORM_WINDOWS 1
    #ifndef _WIN64
        #error "64-bit is required on windows."
    #endif
// linux
#elif defined(__linux__) || defined(__gnu_linux__)
    #define IPLATFORM_LINUX 1
#elif defined(__unix__)
    #define IPLATFORM_UNIX 1
#elif defined(__POSIX__)
    #define IPLATFORM_POSIX 1
// catch any other unsupported OS
#else
    #error "platform is not supported."
#endif

#ifndef IPLATFORM_WINDOWS
    #define COMP_SUPPRESS_OUTPUT " > /dev/null 2>&1"
#else
    #define COMP_SUPPRESS_OUTPUT " > NUL 2>&1"
#endif

#define COMP_MSVC_SUPPRESS_OUTPUT " /nologo > NUL 2>&1"

#endif // DEFINES_H