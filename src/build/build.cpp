#include "build.h"

#include <filesystem>
namespace fs = std::filesystem;

namespace Y::Build {

std::string ToLower(const std::string &str)
{
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower_str;
}

Compiler WhatCompiler(std::string comp)
{
    comp = ToLower(comp);
    if(comp == "clang" || comp == "clang++")
        return Compiler::CLANG;

    if(comp == "icc" || comp == "intel c++")
        return Compiler::ICC;

    if(comp == "gcc" || comp == "gnu" || comp == "g++" || comp == "gnu gcc")
        return Compiler::GCC;

    if(comp == "cl" || comp == "msvc" || comp == "cl++")
        return Compiler::MSVC;

    return Compiler::NONE;
}

FileType GetFileType(const char *filepath)
{
    std::string path(filepath);
    std::string extension = ToLower(fs::path(path).extension().string());

    if(extension == ".c")
        return FileType::C;

    if(extension == ".cpp" || extension == ".cxx" || extension == ".cc")
        return FileType::CPP;

    if(extension == ".h" || extension == ".hpp" || extension == ".hxx")
        return FileType::HEADER;

    if(extension == ".o" || extension == ".obj")
        return FileType::OBJ;

    if(extension == ".i")
        return FileType::INTERMEDIARY;

    return FileType::UNKOWN;
}

// path/base.c (or path/base.cpp) -> objPath/base.o (decides to use either the c compiler or the cpp compiler.)
std::string CompileUnit(const Project &proj, BuildMode mode, const char *filepath, const char *objPath)
{
    FileType fileType = GetFileType(filepath);

    Compiler compiler   = Compiler::NONE;
    std::string command = "";
    if(fileType == FileType::C)
    {
        compiler = WhatCompiler(proj.cCompiler);
        command += proj.cCompiler;
        command += " ";
    }
    else if(fileType == FileType::CPP)
    {
        compiler = WhatCompiler(proj.cppCompiler);
        command += proj.cppCompiler;
        command += " ";
    }

    // add -c flag.
    command += (compiler == Compiler::MSVC) ? COMP_MSVC_COMPILE_ONLY : COMP_COMPILE_ONLY;
    command += std::string(filepath) + " ";

    // add -o flag

    // get output path
    fs::path inputPath(filepath);
    fs::path outputPath = Cache::ConcatenatePath(objPath, inputPath.stem().string());
    outputPath += (compiler == Compiler::MSVC) ? ".obj" : ".o";

    command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outputPath.string().c_str())
                                            : COMP_OUTPUT_FILE(outputPath.string().c_str());

    // add defines from project...
    if(mode == BuildMode::RELEASE)
    {
        for(auto macro : proj.definesRelease)
            command += (compiler == Compiler::MSVC) ? (COMP_MSVC_DEFINE_MACRO(macro.c_str()))
                                                    : (COMP_DEFINE_MACRO(macro.c_str()));
    }
    else if(mode == BuildMode::DEBUG)
    {
        for(auto macro : proj.definesDebug)
            command += (compiler == Compiler::MSVC) ? (COMP_MSVC_DEFINE_MACRO(macro.c_str()))
                                                    : (COMP_DEFINE_MACRO(macro.c_str()));
    }

    // add includes from project...
    for(auto include : proj.includeDirs)
        command += (compiler == Compiler::MSVC) ? (COMP_MSVC_INCLUDE_DIR(include.c_str()))
                                                : (COMP_INCLUDE_DIR(include.c_str()));

    // add compiler flags from project...
    for(auto flag : proj.flags)
        command += flag + " ";

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

    i32 result = std::system(command.c_str());
    LASSERT(result == 0, RED_TEXT("[YMAKE COMPILE ERROR]: "), "failed to compile source file: ", filepath, "\n\t",
            "exit code: ", result, "\n");

    return outputPath.string();
}

// takes a /path/to/base.c -> /new/path/base.i
std::string PreprocessUnit(const Project &proj, const std::string &file, const std::string &path)
{
    LTRACE(true, "generating preprocessed file for src: ", file, "\n");
    FileType fileType = GetFileType(file.c_str());

    Compiler compiler   = Compiler::NONE;
    std::string command = "";
    if(fileType == FileType::C)
    {
        compiler = WhatCompiler(proj.cCompiler);
        command += std::string(proj.cCompiler) + " ";
    }
    else if(fileType == FileType::CPP)
    {
        compiler = WhatCompiler(proj.cppCompiler);
        command += std::string(proj.cppCompiler) + " ";
    }

    // add -E flag and file to preprocess.
    command +=
        (compiler == Compiler::MSVC) ? std::string(COMP_MSVC_PREPROCESS_ONLY) : std::string(COMP_PREPROCESS_ONLY);
    command += std::string(file) + " ";

    // output file.
    fs::path inputPath(file);
    fs::path outputPath = Cache::ConcatenatePath(path, inputPath.stem().string());
    outputPath += std::string(".i");

    LTRACE(true, "about to create preprocessed cache at: ", outputPath.string(), "\n");

    command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outputPath.string())
                                            : COMP_OUTPUT_FILE(outputPath.string());

    // add includes
    for(auto include : proj.includeDirs)
        command +=
            (compiler == Compiler::MSVC) ? COMP_MSVC_INCLUDE_DIR(include.c_str()) : COMP_INCLUDE_DIR(include.c_str());

    // supress output.
    command += (compiler == Compiler::MSVC) ? COMP_MSVC_SUPPRESS_OUTPUT : COMP_SUPPRESS_OUTPUT;

    i32 result = std::system(command.c_str());
    LASSERT(result == 0, RED_TEXT("[YMAKE COMPILE ERROR]: "), "failed to compile source file: ", file, "\n\t",
            "exit code: ", result, "\n");

    return outputPath.string();
}

void CreatePreprocessedCache(const std::vector<std::string> &files, const char *projCacheDir)
{
    LTRACE(true, "creating cache for preprocessed files.\n");

    // create files/dirs
    Cache::CreateDir(projCacheDir);
    std::string cachefilepath = std::string(projCacheDir) + "/" + YMAKE_PREPROCESS_CACHE_FILENAME;

    // create preprocess_metadata.cache file.
    std::ofstream cachefile(cachefilepath);
    if(!cachefile.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create a cache file.\n");
        throw Y::Error("couldn't create a metadata cache files.\n");
    }

    // load data into file.
    for(const auto &file : files)
    {
        cachefile << file << " " << Cache::GetFileSize(file.c_str()) << "\n";
    }

    cachefile.close();
}

void UpdatePreprocessedCache(const std::string &file, const char *projCacheDir)
{
    std::string cachefilepath = std::string(projCacheDir) + "/" + YMAKE_PREPROCESS_CACHE_FILENAME;
    std::map<std::string, u64> cacheData;

    // Load existing cache data
    std::ifstream cachefile_in(cachefilepath);
    if(cachefile_in.is_open())
    {
        std::string filepath;
        u64 filesize;
        while(cachefile_in >> filepath >> filesize)
        {
            cacheData[filepath] = filesize;
        }
        cachefile_in.close();
    }

    // Update or add the new file data
    cacheData[file] = Cache::GetFileSize(file.c_str());

    // Write the updated cache data back to the file
    std::ofstream cachefile_out(cachefilepath, std::ios::out | std::ios::trunc);
    if(!cachefile_out.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create a cache file.\n");
        throw Y::Error("couldn't create a metadata cache file.\n");
    }

    for(const auto &entry : cacheData)
    {
        cachefile_out << entry.first << " " << entry.second << "\n";
    }

    cachefile_out.close();
}

// returns a map of <filepath, filesize>
std::map<std::string, u64> LoadPreprocessedCache(const char *projCacheDir)
{
    if(!Cache::DirExists(projCacheDir))
        throw Y::Error("couldn't load a cache file... project cache directory doesn't exist");

    std::string cachefilepath = std::string(projCacheDir) + "/" + YMAKE_PREPROCESS_CACHE_FILENAME;

    std::ifstream cachefile(cachefilepath);
    if(!cachefile.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open a cache file.\n");
        throw Y::Error("couldn't open a metadata cache files.\n");
    }

    std::map<std::string, u64> files;
    std::string line;
    while(std::getline(cachefile, line))
    {
        std::istringstream iss(line);
        std::string path;
        u64 filesize;
        if(iss >> path >> filesize)
        {
            files[path] = filesize;
        }
    }

    return files;
}

std::vector<std::string> GeneratePreprocessedFiles(const Project &proj, const std::vector<std::string> &files,
                                                   const char *path)
{
    LTRACE(true, "generating preprocessed files for caching.\n");

    // cache
    std::string cacheDir = std::string(YMAKE_CACHE_DIR) + "/" + proj.name;
    Cache::CreateDir(cacheDir.c_str());

    std::vector<std::string> compiledFiles;
    for(const auto &file : files)
    {
        // generate .i file.
        compiledFiles.push_back(PreprocessUnit(proj, file, cacheDir));
    }

    // generate cache.
    try
    {
        LTRACE(true, "generating cache for .i files.\n");
        CreatePreprocessedCache(compiledFiles, cacheDir.c_str());
    }
    catch(Y::Error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create cache files in dir: ", cacheDir, ".\n\t", err.what());
    }

    return compiledFiles;
}

std::string SelectiveCompileUnit(const Project &proj, BuildMode mode, const std::string &filePath,
                                 const std::string &cacheDir)
{
    std::string filepath = Cache::ToAbsolutePath(filePath);
    LTRACE(true, "checking if file \'", filepath, "\' needs re-compiling...\n");

    // load cache.
    std::unordered_map<std::string, Cache::FileMetadata> cacheReg;
    try
    {
        cacheReg = Cache::LoadMetadataCache(std::string(cacheDir.c_str()) + "/" + YMAKE_METADATA_CACHE_FILENAME);
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

        std::string preFile = PreprocessUnit(proj, filepath, cacheDir.c_str());
        UpdatePreprocessedCache(preFile.c_str(), cacheDir.c_str());

        // recompile
        return CompileUnit(proj, mode, filepath.c_str(), cacheDir.c_str());
    }

    if(cacheReg[filepath.c_str()].fileSize != Cache::GetFileSize(filepath.c_str()))
    {
        LTRACE(true, "file is in the cache registry. and file size has changed. recompiling.\n");
        Cache::UpdateMetadataCache(filepath, cacheDir.c_str());

        std::string preFile = PreprocessUnit(proj, filepath, cacheDir.c_str());
        UpdatePreprocessedCache(preFile.c_str(), cacheDir.c_str());

        // recompile
        return CompileUnit(proj, mode, filepath.c_str(), cacheDir.c_str());
    }
    else if(cacheReg[filepath.c_str()].lastWriteTime != Cache::GetTimeSinceLastWrite(filepath.c_str()))
    {
        LTRACE(true, "file is in the cache registry. and file size has changed. recompiling.\n");
        Cache::UpdateMetadataCache(filepath, cacheDir.c_str());

        std::string preFile = PreprocessUnit(proj, filepath, cacheDir.c_str());
        UpdatePreprocessedCache(preFile.c_str(), cacheDir.c_str());

        // recompile
        return CompileUnit(proj, mode, filepath.c_str(), cacheDir.c_str());
    }

    LTRACE(true, "file is in the cache registry, but it is unchanged.\n");

    Compiler compiler = Compiler::NONE;

    for(const Lang lang : proj.langs)
    {
        if(lang == Lang::CXX)
            compiler = WhatCompiler(proj.cppCompiler.c_str());
    }

    if(compiler == Compiler::NONE)
        compiler = WhatCompiler(proj.cCompiler.c_str());

    fs::path inputPath(filepath);
    fs::path outputPath = Cache::ConcatenatePath(cacheDir, inputPath.stem().string());
    outputPath += (compiler == Compiler::MSVC) ? ".obj" : ".o";

    return outputPath.string();
}

// compiles and links lib -> returns path to built lib file
Library BuildLibrary(const Project &proj, const Library &lib, const char *buildDir)
{
    // TODO: compile and build (all in one step..) [or MAYBE: add multithreading...] (later...)
    return lib;
}

void LinkAll(const Project &proj, const std::vector<std::string> &translationUnits,
             const std::vector<Library> &compiledLibs)
{
    Cache::CreateDir(proj.buildDir.c_str());

    Compiler compiler   = Compiler::NONE;
    std::string command = "";
    for(const Lang &lang : proj.langs)
    {
        if(lang == Lang::CXX)
        {
            compiler = WhatCompiler(proj.cppCompiler.c_str());
            command += std::string(proj.cppCompiler) + " ";
        }
    }

    if(compiler == Compiler::NONE)
    {
        compiler = WhatCompiler(proj.cCompiler.c_str());
        command += std::string(proj.cCompiler) + " ";
    }

    // command like: clang++ main.o something.o -o project.exe -L./libpath -llibname
    for(const auto &file : translationUnits)
        command += file + " ";

    for(const auto &lib : compiledLibs)
    {
        // will be in buildDir/GLFW.dll for example.
        command +=
            (compiler == Compiler::MSVC) ? COMP_MSVC_LIBRARY_DIR(proj.buildDir) : COMP_LIBRARY_DIR(proj.buildDir);
        command += (compiler == Compiler::MSVC) ? COMP_MSVC_LINK_LIBRARY(lib.name) : COMP_LINK_LIBRARY(lib.name);
    }

    // output.
    std::string outname = proj.buildDir + "/" + proj.name;
    command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outname) : COMP_OUTPUT_FILE(outname);

    i32 result = std::system(command.c_str());
    LASSERT(result == 0, RED_TEXT("[YMAKE LINKER ERROR]: "), "failed to link project files for project: ", proj.name,
            "\n\t", "exit code: ", result, "\n");

    return;
}

void BuildProject(Project proj, BuildMode mode, bool cleanBuild)
{
    // TODO: add multithreading and start a timer to see how long it takes to compile.
    // maybe give a percentage based on the no. of files [ % per file = (no. files) / 100% ]

    LTRACE(true, "building project: ", proj.name, "...\n");
    std::string projectCacheDir = std::string(YMAKE_CACHE_DIR) + "/" + proj.name;

    // get all files.
    std::vector<std::string> files = Cache::GetSrcFilesRecursive(proj.src);
    std::vector<std::string> translationUnits;

    // if clean build (or no cache exists) -> recompile everything (including libs)
    // recreate the cache.
    if(cleanBuild || !Cache::DirExists(projectCacheDir.c_str()))
    {
        LTRACE(true, "cache doesn't exist. performing a clean build.\n");

        // remove old cache.
        if(!Cache::RemoveAllMetadataCache())
            throw Y::Error("couldn't remove projects cache files.");

        // build libs.
        std::vector<Library> compiledLibs;
        for(auto lib : proj.libs)
        {
            // get src files.
            // std::vector<std::string> libFiles = Cache::GetSrcFilesRecursive(lib.path);

            Library compiledLib = BuildLibrary(proj, lib, proj.buildDir.c_str());
            // the compiler only needs a path to the lib (great!)
            compiledLibs.push_back(compiledLib);
            // compile source files
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

    // if not:
    // loop through all files ->
    // if a file is changed recompile.
    // if not changed -> generate its preprocessed .i file.
    // if that is not changed -> leave alone
    // if changed -> recompile...
    //      returns list of translation units.

    // get list of all translation units.
    //      link all files and libs.
    // exit...

    LTRACE(true, "cache dir exists. building project...\n");

    // build libs.
    std::vector<Library> compiledLibs;
    for(auto lib : proj.libs)
    {
        Library compiledLib = BuildLibrary(proj, lib, proj.buildDir.c_str());

        // the compiler only needs a path to the lib (great!)
        compiledLibs.push_back(compiledLib);
    }

    // build project.
    // compile source files.
    for(const auto &file : files)
    {
        translationUnits.push_back(SelectiveCompileUnit(proj, mode, file.c_str(), projectCacheDir.c_str()));
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

} // namespace Y::Build