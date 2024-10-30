#include "build2.h"

using std::string;
using std::vector;
namespace fs = std::filesystem;

namespace Y::Build {

// path/to/filename.c -> filename
string Basename(string fullpath)
{
    fs::path path(fullpath);
    return path.stem().string();
}

string Basepath(string fullpath)
{
    fs::path path(fullpath);
    return path.parent_path().string();
}

string GetHashedFileNameFromPath(const string &file)
{
    std::hash<string> hasher;
    usize hashValue = hasher(file);
    return Basename(file) + "_" + std::to_string(hashValue);
}

// path/to/file.c -> outDir/file_HASH.o
string CompileFile(Project proj, const string &file, const string &outDir, BuildMode mode, BuildType type, bool project)
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

    if(type == BuildType::SHARED_LIB && compiler != Compiler::CLANG)
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

    command += (compiler == Compiler::MSVC) ? COMP_MSVC_INCLUDE_DIR(Basepath(file).c_str())
                                            : COMP_INCLUDE_DIR(Basepath(file).c_str());

    // add build dir as include.
    command += (compiler == Compiler::MSVC) ? COMP_MSVC_INCLUDE_DIR(proj.buildDir.c_str())
                                            : COMP_INCLUDE_DIR(proj.buildDir.c_str());

    if(project)
    {
        for(auto lib : proj.libs)
        {
            command += (compiler == Compiler::MSVC) ? COMP_MSVC_INCLUDE_DIR(lib.include.c_str())
                                                    : COMP_INCLUDE_DIR(lib.include.c_str());
        }
    }

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
    return (result == 0); // 0 means success.
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

    bool onWindows = false;

#if defined(IPLATFORM_WINDOWS)
    std::string libDynamicExt = ".dll";
    onWindows                 = true;
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

    // add system libraries.
    for(auto sysLib : proj.sysLibs)
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(sysLib) : COMP_LINK_LIBRARY(sysLib);

    // add prebuilt libraries.
    for(auto prebuiltLib : proj.preBuiltLibs)
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(prebuiltLib) : COMP_LINK_LIBRARY(prebuiltLib);

    // add output file.
    const string outname = string(buildDir) + "/" + lib.name + libDynamicExt;
    LTRACE(true, "OUTNAME: ", outname, "\n------------------------------------------\n");
    command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outname) : COMP_OUTPUT_FILE(outname);

    LTRACE(true, "COMMAND TO LINK SHARED LIB: \n\t", command.c_str(), "\n");

    // link the library.
    i32 result = std::system(command.c_str());
    LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to link library: ", lib.name, "\n\t",
            "exit code: ", result, "\n");

    if(!onWindows)
        return outname;

    // NOTE: THANK GOD FOR GCC...
    if(compiler == Compiler::GCC)
    {
        LTRACE(true, "COMPILER IS GCC. NO NEED TO GENERATE A .lib file.",
               "\n-------------------------------------------\n");
        return outname;
    }

    // windows specific.

    // compile the .lib file.
    bool gendefAvailable   = IsToolAvailable("gendef");
    bool dumpbinAvailable  = IsToolAvailable("dumpbin");
    bool pexportsAvailable = IsToolAvailable("pexports");

    bool dlltoolAvailable = IsToolAvailable("dlltool");
    bool msvcAvailable    = IsToolAvailable("lib");
    bool gccAvailable     = IsToolAvailable("gcc");
    bool llvmArAvailable  = IsToolAvailable("llvm-ar");

    string defFile = Basepath(outname) + "/" + lib.name + ".def";

    // Step 1: Generate the .def file
    if(gendefAvailable)
    {
        string defCommand = "gendef " + outname;

        result = std::system(defCommand.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to generate .def file for: ", lib.name, "\n\t",
                "exit code: ", result, "\n");
    }

    if(!fs::exists(defFile) && dumpbinAvailable)
    {
        string defCommand = "dumpbin /exports " + outname + " > " + defFile;

        result = std::system(defCommand.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to generate .def file for: ", lib.name, "\n\t",
                "exit code: ", result, "\n");
    }
    else if(!fs::exists(defFile) && pexportsAvailable)
    {
        string defCommand = "pexports " + outname + " > " + defFile;

        result = std::system(defCommand.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to generate .def file for: ", lib.name, "\n\t",
                "exit code: ", result, "\n");
    }
    else if(!fs::exists(defFile))
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "),
             ".def generator tools are not available (or didn't generate a .def file). Cannot generate .def file for "
             "library: ",
             lib.name, "\n");
        throw Y::Error("couldn't generate a .def file required for generating a .lib file. (required for dynamic "
                       "libraries on windows.)");
    }

    // Step 2: Generate the .lib file
    if(dlltoolAvailable)
    {
        string libCommand = "dlltool -d " + defFile + " -l " + Basepath(outname) + "/" + lib.name + ".lib";
        result            = std::system(libCommand.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to generate .lib file for: ", lib.name, "\n\t",
                "exit code: ", result, "\n");
    }
    else if(msvcAvailable)
    {
        string libCommand = "lib /DEF:" + defFile + " /OUT:" + outname.substr(0, outname.find_last_of('.')) + ".lib";
        result            = std::system(libCommand.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "),
                "failed to generate .lib file using MSVC for: ", lib.name, "\n\t", "exit code: ", result, "\n");
    }
    else if(gccAvailable)
    {
        // TODO: this takes a .o file...
        string libCommand = "gcc -shared -o " + outname + " " + outname.substr(0, outname.find_last_of('.')) +
                            ".o -Wl,--out-implib," + outname.substr(0, outname.find_last_of('.')) + ".lib";
        result = std::system(libCommand.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "),
                "failed to generate .lib file using GCC for: ", lib.name, "\n\t", "exit code: ", result, "\n");
    }
    else if(llvmArAvailable)
    {
        // TODO: this takes a .o file...
        string libCommand = "llvm-ar rcs " + outname.substr(0, outname.find_last_of('.')) + ".lib " +
                            outname.substr(0, outname.find_last_of('.')) + ".o";
        result = std::system(libCommand.c_str());
        LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "),
                "failed to generate .lib file using LLVM for: ", lib.name, "\n\t", "exit code: ", result, "\n");
    }
    else
    {
        if(!gendefAvailable)
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "),
                 "'gendef' tool is not available. Cannot generate .def file for library: ", lib.name, "\n");
        }
        if(!dlltoolAvailable)
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "),
                 "'dlltool' tool is not available. Cannot generate .lib file for library: ", lib.name, "\n");
        }
        if(!msvcAvailable)
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "),
                 "'lib' tool is not available. Cannot generate .lib file using MSVC for library: ", lib.name, "\n");
        }
        if(!gccAvailable)
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "), "'gcc' tool is not available. Cannot generate .lib file using GCC.\n");
        }
        if(!llvmArAvailable)
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "),
                 "'llvm-ar' tool is not available. Cannot generate .lib file using LLVM.\n");
        }

        LLOG(YELLOW_TEXT("[YMAKE]: Tools you can install:\n"));
        LLOG("\tgendef, and dlltool\n\tmsvc (cl)\n\tgcc (mingw)\n\tllvm-ar (llvm tools)\n");
        throw Y::Error("couldn't generate a .lib file required for a dynamic library (dll) [necessary for windows]");
    }

    // Remove the .def file if it was generated
    if(gendefAvailable)
    {
        if(std::remove(defFile.c_str()) != 0)
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "), "failed to remove .def file: ", defFile, "\n");
        }
    }

    LTRACE(true, "OUTNAME: ", outname, "\n------------------------------------------\n");

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
                string compiledFile =
                    CompileFile(proj, file.c_str(), cacheDir.c_str(), BuildMode::RELEASE, lib.type, false);
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
        else
            throw Y::Error("unknown library type.");
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
    // NOTE: path is path to final lib (ex: build/DEngine.dll)
    compLib.path = builtLib;
    LTRACE(true, "COMPILED LIBRARY AT: ", builtLib, "\n---------------------------------------\n");
    compLib.type    = lib.type;
    compLib.include = lib.include;

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
    {
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_INCLUDE_DIR(lib.include) : COMP_INCLUDE_DIR(lib.include);
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LIBRARY_DIR(Basepath(lib.path))
                                                : COMP_LIBRARY_DIR(Basepath(lib.path));

        // NOTE: could use Basename(lib.path) instead of lib.name
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(lib.name) : COMP_LINK_LIBRARY(lib.name);
    }

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

bool NeedsRecompiling(const Project &proj, const string &filePath)
{
    string cacheDir = string(YMAKE_CACHE_DIR) + "/" + proj.name;

    string filepath = Cache::ToAbsolutePath(filePath);
    LTRACE(true, "checking if file \'", filepath, "\' needs re-compiling...\n");

    // load cache.
    std::unordered_map<string, Cache::FileMetadata> cacheReg;
    try
    {
        cacheReg = Cache::LoadMetadataCache(string(cacheDir.c_str()) + "/" + YMAKE_METADATA_CACHE_FILENAME);
    }
    catch(Y::Error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't load cache from cache directory.\n\t", err.what(), "\n");
        exit(1);
    }

    LTRACE(true, "files found in metadata.cache for proj: ", proj.name, ", are: \n");
    for(auto &[k, v] : cacheReg)
    {
        LTRACE(true, "entry => k: ", k, "\tv: ", v.fileSize, "\n");
    }

    // if file doesn't exist in cache reg -> recompile
    if(cacheReg.count(filepath) == 0)
    {
        // update cache.
        LTRACE(true, "file is not in the cache registry -> it needs recompiling.\n");
        Cache::UpdateMetadataCache(filepath, cacheDir.c_str());

        string preFile = Cache::PreprocessUnit(proj, filepath, cacheDir.c_str());
        Cache::UpdatePreprocessedCache(preFile.c_str(), cacheDir.c_str());

        return true; // it needs recompiling.
    }

    if(cacheReg[filepath.c_str()].fileSize != Cache::GetFileSize(filepath.c_str()))
    {
        LTRACE(true, "file is in the cache registry. and file size has changed. recompiling.\n");
        Cache::UpdateMetadataCache(filepath, cacheDir.c_str());

        string preFile = Cache::PreprocessUnit(proj, filepath, cacheDir.c_str());
        Cache::UpdatePreprocessedCache(preFile.c_str(), cacheDir.c_str());

        return true; // it needs recompiling.
    }
    else if(cacheReg[filepath.c_str()].lastWriteTime != Cache::GetTimeSinceLastWrite(filepath.c_str()))
    {
        LTRACE(true, "file is in the cache registry. and file size has changed. recompiling.\n");
        Cache::UpdateMetadataCache(filepath, cacheDir.c_str());

        string preFile = Cache::PreprocessUnit(proj, filepath, cacheDir.c_str());
        Cache::UpdatePreprocessedCache(preFile.c_str(), cacheDir.c_str());

        return true; // it needs recompiling.
    }

    LTRACE(true, "file is in the cache registry, but it is unchanged.\n");
    return false;
}

/*
if(cleanBuild || !Cache::DirExists(projectCacheDir.c_str()) || mode == BuildMode::RELEASE)
    {
        LTRACE(true, "cache doesn't exist. performing a clean build.\n");

        // remove old cache.
        if(!Cache::RemoveAllMetadataCache())
            throw Y::Error("couldn't remove projects cache files.");

        // build libs.
        std::vector<Library> compiledLibs;
        for(auto lib : proj.libs)
        {
            Library compiledLib = BuildLibrary(proj, lib, proj.buildDir.c_str());

            compiledLibs.push_back(compiledLib);
        }

        Cache::CreateDir(projectCacheDir.c_str());

        // build project.
        // compile source files.
        for(const auto &file : files)
        {
            translationUnits.push_back(CompileUnit(proj, mode, file.c_str(), projectCacheDir.c_str()));
        }

        try
        {
            // regen cache for .cpp files.
            Cache::CreateMetadataCache(files, projectCacheDir.c_str());

            // gen cache for .i files.
            std::vector<std::string> preprocessedFiles =
                GeneratePreprocessedFiles(proj, files, projectCacheDir.c_str());

            for(auto file : preprocessedFiles)
            {
                LTRACE(true, "generated file at: ", file, "\n");
            }
        }
        catch(Y::Error &err)
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't generate cache data for project: ", proj.name, "\n");
        }

        // take the list of obj files and link them all...
        try
        {
            LinkAll(proj, translationUnits, compiledLibs);
        }
        catch(Y::Error &err)
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't link files and libraries for project: ", proj.name, "\n");
            exit(1);
        }

        LLOG(GREEN_TEXT("[YMAKE BUILD]: "), "successfully built project \'", proj.name, "\', binary at dir: \'",
             proj.buildDir, "\n");

        exit(0);
    }
*/
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
    string projCacheDir = string(YMAKE_CACHE_DIR) + "/" + proj.name;
    if(cleanBuild || !Cache::DirExists(projCacheDir.c_str()) || mode == BuildMode::RELEASE)
    {
        // TODO: build libs.
    }
    else
    {
        // TODO: return built libs' path.
    }

    //_____________________ BUILDING LIBRARIES ____________________
    f32 percentPerPart = 100.0f / (proj.libs.size() + 1);

    // TODO: cache compiled libraries to a file and load them into the vector instead if !cleanBuild.
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

            LLOG(RED_TEXT("EXITING....\n"));
            return;
        }
    }

    //_____________________ BUILDING PROJECT SRC ____________________
    // build project files.
    // build project files -> returns list of .o files.
    vector<string> allFiles = Cache::GetSrcFilesRecursive(proj.src);

    // percentage calculation.
    f32 filePercent_f = 100.0f / allFiles.size();
    filePercent_f *= (percentPerPart / 100.0f);
    i32 filePercent = static_cast<i32>(filePercent_f);

    if(!Cache::DirExists(projCacheDir.c_str()))
        Cache::CreateDir(projCacheDir.c_str());

    string cacheDir = string(projCacheDir) + "/" + "src";
    if(!Cache::DirExists(cacheDir.c_str()))
        Cache::CreateDir(cacheDir.c_str());

    vector<string> compiledFiles;
    vector<string> files;
    for(auto file : allFiles)
    {
        if(NeedsRecompiling(proj, file))
        {
            files.push_back(file);
        }
        else
        {
            percent += filePercent;
            if(percent == 99)
                percent = 100;

            string hashedName = GetHashedFileNameFromPath(file);
            compiledFiles.push_back(projCacheDir + "/" + hashedName);
        }
    }

    LTRACE(true, "project files: ", allFiles.size(), "\n");
    for(auto file : allFiles)
    {
        LTRACE(true, "\tFILE: ", file, "\n");
    }

    LTRACE(true, "files to recompile: ", files.size(), "\n");
    for(auto file : files)
    {
        LTRACE(true, "\tFILE: ", file, "\n");
    }

    // TODO: output the number as float (2 digits after point.)

    ThreadPool threadPool;
    for(auto file : files)
    {
        threadPool.AddTask([&proj, file, cacheDir, mode, &compiledFiles, &percent, &filePercent, &threadPool] {
            try
            {
                string compiledFile = CompileFile(proj, file.c_str(), cacheDir.c_str(), mode, proj.buildType, true);
                compiledFiles.push_back(compiledFile);
            }
            catch(Y::Error &err)
            {
                LLOG(RED_TEXT("[YMAKE BUILD]: "), "error building file: ", CYAN_TEXT(file), "\n\t", err.what(), "\n");
                LLOG(RED_TEXT("EXITING....\n"));
                return;
            }

            threadPool.Lock();
            percent += filePercent;
            if(percent == 99)
                percent = 100;

            LLOG(GREEN_TEXT("[YMAKE BUILD]: "), BLUE_TEXT("[", percent, "%] "), "built file: ", CYAN_TEXT(file), "\n");
            threadPool.Unlock();
        });
    }

    threadPool.JoinAll();

    //____________________ LINK ALL ___________________
    // TODO: use docker etc... to link if in release mode.
    // TODO: get the target of the debug build (for fixing clang errors).
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
        LLOG(RED_TEXT("EXITING...\n"));
        return;
    }
    catch(...)
    {
        LLOG(RED_TEXT("[YMAKE BUILD ERROR]: "), "error linking project: ", CYAN_TEXT(proj.name), "\n");
        LLOG(RED_TEXT("EXITING..."));
        return;
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
