#include "build2.h"

using std::string;
using std::vector;
namespace fs = std::filesystem;

namespace Y::Build {

// path/to/filename.c -> filename
string Basename(string fullpath)
{
    size_t pos = fullpath.find_last_of("/\\");
    if(pos != string::npos)
    {
        string filename = fullpath.substr(pos + 1);
        size_t dotPos   = filename.find_last_of('.');
        if(dotPos != string::npos)
        {
            return filename.substr(0, dotPos);
        }
        return filename;
    }
    return fullpath;
}

std::string ToLower(const std::string &str)
{
    std::string lower_str = "";
    for(auto const &ch : str)
        lower_str += std::tolower(ch);

    return lower_str;
}

Compiler WhatCompiler(const string &compiler)
{
    std::string comp = ToLower(compiler);
    if(comp == "clang" || comp == "clang++")
        return Compiler::CLANG;

    if(comp == "icc" || comp == "intel c++")
        return Compiler::ICC;

    if(comp == "gcc" || comp == "gnu" || comp == "g++" || comp == "gnu gcc")
        return Compiler::GCC;

    if(comp == "cl" || comp == "msvc" || comp == "cl++")
        return Compiler::MSVC;

    if(comp == "" || comp.empty())
        return Compiler::NONE;

    return Compiler::UNKOWN;
}

string GetHashedFileNameFromPath(const string &file)
{
    std::hash<std::string> hasher;
    usize hashValue = hasher(file);
    return Basename(file) + "_" + std::to_string(hashValue);
}

FileType GetFileType(const string &filepath)
{
    std::string path(filepath);
    std::string extension = ToLower(fs::path(path).extension().string());

    if(extension == ".c")
        return FileType::C;

    if(extension == ".cpp" || extension == ".cxx" || extension == ".cc" || extension == ".c++" || extension == ".cp" ||
       extension == ".cxx" || extension == ".tpp")
        return FileType::CPP;

    if(extension == ".h" || extension == ".hpp" || extension == ".hxx" || extension == ".h++" || extension == ".hh")
        return FileType::HEADER;

    if(extension == ".o" || extension == ".obj")
        return FileType::OBJ;

    if(extension == ".i")
        return FileType::INTERMEDIARY;

    return FileType::UNKOWN;
}

// path/to/file.c -> outDir/file_HASH.o
string CompileFile(Project proj, const string &file, const string &outDir, BuildMode mode, BuildType type)
{
    // ex: clang -c file.c [flags] -o Concat(outDir, file.o)
    // flags: linking, optimization, include dirs, defines, etc.

    // compiler.
    FileType fileType = GetFileType(file);
    LTRACE(true, "compiling file: ", file, "\n");

    Compiler compiler = Compiler::NONE;
    string command    = "";

    if(fileType == FileType::C)
    {
        compiler = WhatCompiler(proj.cCompiler);
        if(compiler == Compiler::UNKOWN)
            throw Y::Error("unknown compiler specified in the project config file.");
        else if(compiler == Compiler::NONE)
            throw Y::Error("no c compiler specified in the project config file.");

        command += proj.cCompiler;
        command += " ";
    }
    else if(fileType == FileType::CPP)
    {
        compiler = WhatCompiler(proj.cppCompiler);
        if(compiler == Compiler::UNKOWN)
            throw Y::Error("unknown compiler specified in the project config file.");
        else if(compiler == Compiler::NONE)
            throw Y::Error("no cpp compiler specified in the project config file.");

        command += proj.cppCompiler;
        command += " ";
    }

    if(type == BuildType::SHARED_LIB)
        command += (compiler == Compiler::MSVC) ? "" : COMP_POSITION_INDEPENDENT_CODE;

    // add -c flag.
    command += (compiler == Compiler::MSVC) ? COMP_MSVC_COMPILE_ONLY : COMP_COMPILE_ONLY;
    command += string(file) + " ";

    // add standard.
    if(fileType == FileType::C)
    {
        command += (compiler == Compiler::MSVC) ? "" : COMP_STANDARD_VERSION_C(proj.cStd);
    }
    else
    {
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_STANDARD_VERSION_C(proj.cppStd)
                                                : COMP_STANDARD_VERSION_CPP(proj.cppStd);
    }

    // add includes.
    for(auto include : proj.includeDirs)
        command +=
            (compiler == Compiler::MSVC) ? COMP_MSVC_INCLUDE_DIR(include.c_str()) : COMP_INCLUDE_DIR(include.c_str());

    // add defines.
    if(mode == BuildMode::RELEASE)
    {
        for(auto macro : proj.definesRelease)
            command +=
                (compiler == Compiler::MSVC) ? COMP_MSVC_DEFINE_MACRO(macro.c_str()) : COMP_DEFINE_MACRO(macro.c_str());
    }
    else if(mode == BuildMode::DEBUG)
    {
        for(auto macro : proj.definesDebug)
            command +=
                (compiler == Compiler::MSVC) ? COMP_MSVC_DEFINE_MACRO(macro.c_str()) : COMP_DEFINE_MACRO(macro.c_str());
    }

    // add compiler flags.
    for(auto flag : proj.flagsDebug)
        command += flag + " ";

    // add optimization level.
    if(mode == BuildMode::RELEASE)
    {
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_OPTIMIZATION_LEVEL(proj.optimizationRelease)
                                                : COMP_OPTIMIZATION_LEVEL(proj.optimizationRelease);
    }
    else if(mode == BuildMode::DEBUG)
    {
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_OPTIMIZATION_LEVEL(proj.optimizationDebug)
                                                : COMP_OPTIMIZATION_LEVEL(proj.optimizationDebug);
    }

    // output.
    string outFilepath = GetHashedFileNameFromPath(file) + ((compiler == Compiler::MSVC) ? ".obj" : ".o");
    string outPath     = string(outDir) + "/" + outFilepath;

    command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outPath) : COMP_OUTPUT_FILE(outPath);

    // suppress output. NOTE: don't suppress output for compiler errors.
    // command += (compiler == Compiler::MSVC) ? COMP_MSVC_SUPPRESS_OUTPUT : COMP_SUPPRESS_OUTPUT;

    LTRACE(true, "COMMAND TO COMPILE: \n\t", command.c_str(), "\n");

    // compile the file.
    i32 result = std::system(command.c_str());
    LASSERT(result == 0, RED_TEXT("[YMAKE COMPILE ERROR]: "), "failed to compile source file: ", file, "\n\t",
            "exit code: ", result, "\n");

    LTRACE(true, "compiled file at: ", outPath, "\n");

    return Cache::ToAbsolutePath(outPath);
}

bool IsToolAvailable(const string &tool)
{
    string command = tool + " --version ";
    command += COMP_SUPPRESS_OUTPUT;
    int result = std::system(command.c_str());
    return !(result == 0); // 0 means success.
}

string LinkStaticLibrary(Project &proj, Library &lib, vector<string> compiledFiles, const char *buildDir)
{
    LTRACE(true, "linking/packaging static library: ", lib.name, "...\n");

#if defined(IPLATFORM_WINDOWS)
    std::string libStaticExt = ".lib";
#elif defined(IPLATFORM_LINUX) || defined(IPLATFORM_FREEBSD) || defined(IPLATFORM_MACOS)
    std::string libStaticExt = ".a";
#elif defined(IPLATFORM_UNIX) || defined(IPLATFORM_POSIX)
    std::string libStaticExt = ".a";
#else
    LLOG(RED_TEXT("[YMAKE LINKER ERROR]: "), "unsupported platform.\n");
    #error "[YMAKE LINKER ERROR]: unsupported platform"
#endif

    // ex: ar rcs libname.a file1.o file2.o file3.o
    //      ex msvc: lib /OUT:libname.lib file1.obj file2.obj file3.obj

    // TODO: add support for other compilers.
    // TODO: make this function more robust.
    // TODO: make vals like ar, llvm-ar, etc... configurable.
    // TODO: make constants defined in defines.h

    if(IsToolAvailable("ar"))
    {
        string command = "ar rcs ";

        for(auto file : compiledFiles)
            command += file + " ";

        string outname = string(buildDir) + "/" + lib.name + libStaticExt;
        command += outname + " ";

        // suppress output.
        command += COMP_SUPPRESS_OUTPUT;

        LTRACE(true, "COMMAND TO LINK STATIC LIB: \n\t", command.c_str(), "\n");

        // link the library.
        i32 result = std::system(command.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to link library: ", lib.name, "\n\t",
                "exit code: ", result, "\n");

        LTRACE(true, "linked static library at: ", outname, "\n");

        return outname;
    }
    else if(WhatCompiler(proj.cppCompiler) == Compiler::MSVC || WhatCompiler(proj.cCompiler) == Compiler::MSVC)
    {
        // do msvc stuff.
        string command = "lib /OUT:";

        string outname = string(buildDir) + "/" + lib.name + libStaticExt;
        command += outname + " ";

        for(auto file : compiledFiles)
            command += file + " ";

        // suppress output.
        command += COMP_MSVC_SUPPRESS_OUTPUT;

        LTRACE(true, "COMMAND TO LINK STATIC LIB: \n\t", command.c_str(), "\n");

        // link the library.
        i32 result = std::system(command.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to link library: ", lib.name, "\n\t",
                "exit code: ", result, "\n");

        LTRACE(true, "linked static library at: ", outname, "\n");

        return outname;
    }
    else if((WhatCompiler(proj.cppCompiler) == Compiler::CLANG || WhatCompiler(proj.cCompiler) == Compiler::CLANG) &&
            IsToolAvailable("llvm-ar"))
    {
        string command = "llvm-ar rcs ";

        string outname = string(buildDir) + "/" + lib.name + libStaticExt;
        command += outname;

        for(auto file : compiledFiles)
            command += file + " ";

        // suppress output.
        command += COMP_SUPPRESS_OUTPUT;

        LTRACE(true, "COMMAND TO LINK STATIC LIB: \n\t", command.c_str(), "\n");

        // link the library.
        i32 result = std::system(command.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to link library: ", lib.name, "\n\t",
                "exit code: ", result, "\n");

        LTRACE(true, "linked static library at: ", outname, "\n");

        return outname;
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "could not package static library: ", CYAN_TEXT(lib.name), "\n\t",
             "ar command not found.\n");
        throw Y::Error("ar command not found.");
    }
}

// string builtLib = LinkLibrary(Project, lib, compiledFiles, buildDir);
string LinkDynamicLibrary(Project &proj, Library &lib, vector<string> compiledFiles, const char *buildDir)
{
    LTRACE(true, "linking shared library: ", lib.name, "...\n");

#if defined(IPLATFORM_WINDOWS)
    std::string libDynamicExt = ".dll";
#elif defined(IPLATFORM_MACOS)
    std::string libDynamicExt = ".dylib";
#elif defined(IPLATFORM_LINUX) || defined(IPLATFORM_FREEBSD)
    std::string libDynamicExt = ".so";
#elif defined(IPLATFORM_UNIX) || defined(IPLATFORM_POSIX)
    std::string libDynamicExt = ".so";
#else
    LLOG(RED_TEXT("[YMAKE ERROR]: "), "unsupported platform.\n");
    #error "[YMAKE ERROR]: unsupported platform"
#endif

    // ex: clang++ -shared main.o something.o -o project.exe -L./libpath -llibname
    //      ex msvc: cl /DLL /OUT:libname.dll file1.obj file2.obj file3.obj

    // compiler.
    Compiler compiler = Compiler::NONE;
    string command    = "";

    if(lib.type == BuildType::EXECUTABLE)
        throw Y::Error("cannot build a library as an executable.");

    if((compiler = WhatCompiler(proj.cppCompiler)) != Compiler::NONE && compiler != Compiler::UNKOWN)
    {
        command += proj.cppCompiler;
        command += " ";
    }
    else if((compiler = WhatCompiler(proj.cCompiler)) != Compiler::NONE && compiler != Compiler::UNKOWN)
    {
        compiler = WhatCompiler(proj.cCompiler);
        command += proj.cCompiler;
        command += " ";
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "no (supported) compiler specified in the project: ", CYAN_TEXT(proj.name),
             "\'s config file.\n");
        throw Y::Error("no compiler specified in the project config file.");
    }

    // add -shared flag.
    command += (compiler == Compiler::MSVC) ? COMP_MSVC_BUILD_SHARED_LIBRARY : COMP_BUILD_SHARED_LIBRARY;

    // add files to link.
    for(auto file : compiledFiles)
        command += file + " ";

    // add linker flags. NOTE: always use release flags for libraries.
    for(auto flag : proj.flagsRelease)
        command += flag + " ";

    // add include dirs.
    for(auto inclDir : proj.includeDirs)
        command +=
            (compiler == Compiler::MSVC) ? COMP_MSVC_INCLUDE_DIR(inclDir.c_str()) : COMP_INCLUDE_DIR(inclDir.c_str());

    // add libraries.
    for(auto lib : proj.libs)
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(lib.name) : COMP_LINK_LIBRARY(lib.name);

    // add system libraries.
    for(auto sysLib : proj.sysLibs)
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(sysLib) : COMP_LINK_LIBRARY(sysLib);

    // add prebuilt libraries.
    for(auto prebuiltLib : proj.preBuiltLibs)
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(prebuiltLib) : COMP_LINK_LIBRARY(prebuiltLib);

    // add output file.
    string outname = string(buildDir) + "/" + lib.name + libDynamicExt;
    command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outname) : COMP_OUTPUT_FILE(outname);

    LTRACE(true, "COMMAND TO LINK SHARED LIB: \n\t", command.c_str(), "\n");

    // link the library.
    i32 result = std::system(command.c_str());
    LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to link library: ", lib.name, "\n\t",
            "exit code: ", result, "\n");

    LTRACE(true, "built dynamic library at: ")

    return outname;
}

Library BuildLibrary(Project &proj, Library &lib, const char *buildDir, f32 libPercent, f32 &currentPercent)
{
    LLOG(BLUE_TEXT("[YMAKE BUILD]: "), "building library: ", CYAN_TEXT(lib.name), "...\n");

    if(lib.path.empty())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "library path is empty.\n");
        throw Y::Error("library path is empty.");
    }

    vector<string> files = Cache::GetSrcFilesRecursive(lib.path);

    // get directory for .o files.
    string projCacheDir = string(YMAKE_CACHE_DIR) + "/" + proj.name;
    if(!Cache::DirExists(projCacheDir.c_str()))
        Cache::CreateDir(projCacheDir.c_str());

    string cacheDir = string(projCacheDir) + "/" + lib.name + "";
    Cache::CreateDir(cacheDir.c_str());

    // percentage calculation.
    f32 filePercent_f = 100.0f / files.size();
    filePercent_f *= (libPercent / 100.0f);
    i32 filePercent = static_cast<i32>(filePercent_f);

    i32 percent = currentPercent;

    // create a thread pool...
    ThreadPool threadPool;
    vector<string> compiledFiles;
    for(auto file : files)
    {
        threadPool.AddTask([&proj, file, cacheDir, &compiledFiles, &percent, filePercent, &threadPool, &lib] {
            try
            {
                // always compile library files in release mode.
                string compiledFile = CompileFile(proj, file.c_str(), cacheDir.c_str(), BuildMode::RELEASE, lib.type);
                LTRACE(true, "compiled file at: ", compiledFile, "\n");

                compiledFiles.push_back(compiledFile);
            }
            catch(Y::Error &err)
            {
                LLOG(RED_TEXT("[YMAKE BUILD]: "), "error building file: ", CYAN_TEXT(file), "\n\t", err.what(), "\n");
            }

            threadPool.Lock();
            percent += filePercent;
            if(percent == 99)
                percent = 100;

            // TODO: see if the message should be (building file) and at the top of the lambda or same as it is now.
            LLOG(GREEN_TEXT("[YMAKE BUILD]: "), BLUE_TEXT("[", percent, "%] "), "built file: ", CYAN_TEXT(file), "\n");
            threadPool.Unlock();
        });
    }

    threadPool.JoinAll();

    // link everything.
    string builtLib;
    try
    {
        if(lib.type == BuildType::STATIC_LIB)
            builtLib = LinkStaticLibrary(proj, lib, compiledFiles, buildDir);
        else if(lib.type == BuildType::SHARED_LIB)
            builtLib = LinkDynamicLibrary(proj, lib, compiledFiles, buildDir);
    }
    catch(Y::Error &err)
    {
        LLOG(RED_TEXT("[YMAKE BUILD]: "), "error linking library: ", CYAN_TEXT(lib.name), "\n\t", err.what(), "\n");
    }

    // building library is done!
    LLOG(GREEN_TEXT("[YMAKE BUILD]: "), BLUE_TEXT("[", percent, "%] "), "built library: ", CYAN_TEXT(lib.name), "\n");

    currentPercent += libPercent;

    Library compLib;
    compLib.name = lib.name;
    compLib.path = builtLib;
    compLib.type = lib.type;

    LTRACE(true, "library built at: ", builtLib, "\n");

    return compLib;
}

// LinkEverything(proj, compiledFiles, compiledLibs);
string LinkEverything(Project &proj, vector<string> compiledFiles, vector<Library> compiledLibs, BuildMode mode)
{
    LLOG(BLUE_TEXT("[YMAKE BUILD]: "), "linking project: ", CYAN_TEXT(proj.name), "...\n");

    // get compiler.
    Compiler compiler = Compiler::NONE;
    string command    = "";

    if((compiler = WhatCompiler(proj.cppCompiler)) != Compiler::NONE && compiler != Compiler::UNKOWN)
    {
        command += proj.cppCompiler;
        command += " ";
    }
    else if((compiler = WhatCompiler(proj.cCompiler)) != Compiler::NONE && compiler != Compiler::UNKOWN)
    {
        compiler = WhatCompiler(proj.cCompiler);
        command += proj.cCompiler;
        command += " ";
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "no (supported) compiler specified in the project: ", CYAN_TEXT(proj.name),
             "\'s config file.\n");
        throw Y::Error("no compiler specified in the project config file.");
    }

    // add files to link.
    for(auto file : compiledFiles)
        command += file + " ";

    // add linker flags.
    if(mode == BuildMode::RELEASE)
    {
        for(auto flag : proj.flagsRelease)
            command += flag + " ";
    }
    else if(mode == BuildMode::DEBUG)
    {
        for(auto flag : proj.flagsDebug)
            command += flag + " ";
    }

    // add includes.
    for(auto inclDir : proj.includeDirs)
        command +=
            (compiler == Compiler::MSVC) ? COMP_MSVC_INCLUDE_DIR(inclDir.c_str()) : COMP_INCLUDE_DIR(inclDir.c_str());

    // add libraries.
    for(auto lib : compiledLibs)
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(lib.path) : COMP_LINK_LIBRARY(lib.path);

    // add system libraries.
    for(auto sysLib : proj.sysLibs)
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(sysLib) : COMP_LINK_LIBRARY(sysLib);

    // add prebuilt libraries.
    for(auto prebuiltLib : proj.preBuiltLibs)
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(prebuiltLib) : COMP_LINK_LIBRARY(prebuiltLib);

        // add output file.

        // executable
#if defined(IPLATFORM_WINDOWS)
    std::string libExecutableExt = ".exe";
#elif defined(IPLATFORM_MACOS)
    std::string libExecutableExt = "";
#elif defined(IPLATFORM_LINUX) || defined(IPLATFORM_FREEBSD)
    std::string libExecutableExt = "";
#elif defined(IPLATFORM_UNIX) || defined(IPLATFORM_POSIX)
    std::string libExecutableExt = "";
#else
    LLOG(RED_TEXT("[YMAKE ERROR]: "), "unsupported platform.\n");
    #error "[YMAKE ERROR]: unsupported platform"
#endif

// shared lib
#if defined(IPLATFORM_WINDOWS)
    std::string libDynamicExt = ".dll";
#elif defined(IPLATFORM_MACOS)
    std::string libDynamicExt = ".dylib";
#elif defined(IPLATFORM_LINUX) || defined(IPLATFORM_FREEBSD)
    std::string libDynamicExt = ".so";
#elif defined(IPLATFORM_UNIX) || defined(IPLATFORM_POSIX)
    std::string libDynamicExt = ".so";
#else
    LLOG(RED_TEXT("[YMAKE ERROR]: "), "unsupported platform.\n");
    #error "[YMAKE ERROR]: unsupported platform"
#endif

// static lib
#if defined(IPLATFORM_WINDOWS)
    std::string libStaticExt = ".lib";
#elif defined(IPLATFORM_LINUX) || defined(IPLATFORM_FREEBSD) || defined(IPLATFORM_MACOS)
    std::string libStaticExt = ".a";
#elif defined(IPLATFORM_UNIX) || defined(IPLATFORM_POSIX)
    std::string libStaticExt = ".a";
#else
    LLOG(RED_TEXT("[YMAKE LINKER ERROR]: "), "unsupported platform.\n");
    #error "[YMAKE LINKER ERROR]: unsupported platform"
#endif

    string outname;
    if(proj.buildType == BuildType::EXECUTABLE)
    {
        outname = string(proj.buildDir) + "/" + proj.name + libExecutableExt;
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outname) : COMP_OUTPUT_FILE(outname);
    }
    else if(proj.buildType == BuildType::STATIC_LIB)
    {
        outname = string(proj.buildDir) + "/" + proj.name + libStaticExt;
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outname) : COMP_OUTPUT_FILE(outname);
    }
    else if(proj.buildType == BuildType::SHARED_LIB)
    {
        outname = string(proj.buildDir) + "/" + proj.name + libDynamicExt;
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outname) : COMP_OUTPUT_FILE(outname);
    }

    LTRACE(true, "COMMAND TO LINK ALL: \n\t", command.c_str(), "\n");

    // link the project.
    i32 result = std::system(command.c_str());
    LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to link project: ", proj.name, "\n\t",
            "exit code: ", result, "\n");

    LTRACE(true, "linked project at: ", proj.buildDir, "\n");

    // TODO: might remove this message.
    LLOG(GREEN_TEXT("[YMAKE BUILD]: "), "linked project: ", CYAN_TEXT(proj.name), "\n");

    return outname;
}

void BuildProject(Project proj, BuildMode mode, bool cleanBuild)
{
    LLOG(BLUE_TEXT("[YMAKE BUILD]: "), "building project: ", CYAN_TEXT(proj.name), "...\n");

    if(!Cache::DirExists(proj.buildDir.c_str()))
    {
        LTRACE(true, BLUE_TEXT("[YMAKE BUILD]: "), "creating build directory: ", CYAN_TEXT(proj.buildDir), "\n");
        Cache::CreateDir(proj.buildDir.c_str());
    }

    // start timer to measure build time.
    auto start  = std::chrono::high_resolution_clock::now();
    f32 percent = 0.0f;

    // FIXME: let's do a clean build and then worry about the caching later.

    //_____________________ BUILDING LIBRARIES ____________________
    f32 percentPerPart = 100.0f / (proj.libs.size() + 1);

    vector<Library> compiledLibs;
    for(auto lib : proj.libs)
    {
        try
        {
            LTRACE(true, "building library: ", lib.name, "...\n");
            Library compiledLib = BuildLibrary(proj, lib, proj.buildDir.c_str(), percentPerPart, percent);
            compiledLibs.push_back(compiledLib);
        }
        catch(Y::Error &err)
        {
            LLOG(RED_TEXT("[YMAKE BUILD]: "), "error building library: ", CYAN_TEXT(lib.name), "\n\t", err.what(),
                 "\n");
        }
    }

    //_____________________ BUILDING PROJECT SRC ____________________
    // build project files.
    // build project files -> returns list of .o files.
    vector<string> files = Cache::GetSrcFilesRecursive(proj.src);
    LTRACE(true, "project files: ", files.size(), "\n");
    for(auto file : files)
    {
        LTRACE(true, "\tFILE: ", file, "\n");
    }

    string projCacheDir = string(YMAKE_CACHE_DIR) + "/" + proj.name;
    if(!Cache::DirExists(projCacheDir.c_str()))
        Cache::CreateDir(projCacheDir.c_str());

    string cacheDir = string(projCacheDir) + "/" + "src";
    if(!Cache::DirExists(cacheDir.c_str()))
        Cache::CreateDir(cacheDir.c_str());

    // percentage calculation.
    f32 filePercent_f = 100.0f / files.size();
    filePercent_f *= (percentPerPart / 100.0f);
    i32 filePercent = static_cast<i32>(filePercent_f);

    vector<string> compiledFiles;
    for(auto file : files)
    {
        try
        {
            string compiledFile = CompileFile(proj, file.c_str(), cacheDir.c_str(), mode, proj.buildType);
            compiledFiles.push_back(compiledFile);
        }
        catch(Y::Error &err)
        {
            LLOG(RED_TEXT("[YMAKE BUILD]: "), "error building file: ", CYAN_TEXT(file), "\n\t", err.what(), "\n");
        }

        percent += filePercent;
        if(percent == 99)
            percent = 100;

        LLOG(GREEN_TEXT("[YMAKE BUILD]: "), BLUE_TEXT("[", percent, "%] "), "built file: ", CYAN_TEXT(file), "\n");
    }

    //____________________ LINK ALL ___________________
    LLOG(BLUE_TEXT("[YMAKE BUILD]: "), "linking project: ", CYAN_TEXT(proj.name), "...\n")

    string outfile;
    try
    {
        LTRACE(true, "linking everything...\n");
        outfile = LinkEverything(proj, compiledFiles, compiledLibs, mode);
    }
    catch(Y::Error &err)
    {
        LLOG(RED_TEXT("[YMAKE BUILD ERROR]: "), "error linking project: ", CYAN_TEXT(proj.name), "\n\t", err.what(),
             "\n");
    }
    catch(...)
    {
        LLOG(RED_TEXT("[YMAKE BUILD ERROR]: "), "error linking project: ", CYAN_TEXT(proj.name), "\n");
    }

    // get final build time
    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    auto seconds      = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    LLOG(GREEN_TEXT("[YMAKE BUILD SUCCESS]: ", "built project: ", CYAN_TEXT(proj.name), "\n"))
    LLOG("\t build took ", GREEN_TEXT(seconds), "s ", GREEN_TEXT(milliseconds), "ms\n");

    if(proj.buildType == BuildType::EXECUTABLE)
        LLOG(PURPLE_TEXT("to run project -> "), "\'", outfile, "\'\n");
}

} // namespace Y::Build
