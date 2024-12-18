#include "cache.h"

#include <filesystem>
namespace fs = std::filesystem;

#include <chrono>
using namespace std::chrono;
using namespace std::chrono_literals;
using fs::path;
using std::string;

#include <sstream>
#include <vector>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>

namespace Y::Cache {

// PROJECT METADATA
string ToLower(const string &str)
{
    string lower_str = "";
    for(auto const &ch : str)
        lower_str += std::tolower(ch);

    return lower_str;
}

void CreateDir(const char *path)
{
    try
    {
        // cache dir.
        if(!fs::exists(path))
        {
            if(!fs::create_directories(path))
                throw Y::Error("couldn't create a directory.");
        }
    }
    catch(const fs::filesystem_error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create a directory.\n\t", err.what(), "\n");
    }
}

void CreateCacheDirectories()
{
    try
    {
        // cache dir.
        if(!fs::exists(YMAKE_CACHE_DIR))
        {
            if(!fs::create_directories(YMAKE_CACHE_DIR))
                throw Y::Error("couldn't create cache directory.");
        }
    }
    catch(const fs::filesystem_error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create cache directory.\n\t", err.what(), "\n");
    }
}

bool DirExists(const char *path)
{
    return (fs::exists(path) && fs::is_directory(path));
}

bool FileExists(const char *path)
{
    return (fs::exists(path) && fs::is_regular_file(path));
}

bool RemoveDir(const char *path)
{
    try
    {
        if(fs::exists(path))
        {
            fs::remove_all(path);
            return true;
        }
        return true;
    }
    catch(const fs::filesystem_error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't remove directory.\n\t", err.what(), "\n");
    }

    return false;
}

std::time_t ToTimeT(const std::string &time)
{
    std::tm tm = {};
    std::istringstream ss(time);
    ss >> std::get_time(&tm, "%Y-%m-%d:%H-%M-%S");
    return std::mktime(&tm); // Use mktime for local time
}

// Converts std::time_t to a string timestamp
std::string ToString(std::time_t time)
{
    std::tm tm;
    localtime_r(&time, &tm); // Use localtime_s for local time
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d:%H-%M-%S");
    return oss.str();
}

std::time_t GetTimeSinceLastWrite(const char *file)
{
    auto ftime = fs::last_write_time(file);
    auto sctp  = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(sctp);
}

u64 GetFileSize(const char *file)
{
    return fs::file_size(file);
}

// concatenates two paths, and returns the full path (absolute.)
std::string ConcatenatePath(const std::string &basePath, const std::string &relativePath)
{
    fs::path base(ToAbsolutePath(basePath));
    fs::path relative(relativePath);

    if(fs::is_regular_file(base))
        base = base.parent_path();

    // Resolve the relative path against the base path
    fs::path absolutePath = fs::absolute(base / relative).lexically_normal();

    return absolutePath.string();
}

std::string ToAbsolutePath(const std::string &path)
{
    return fs::absolute(path).lexically_normal().string();
}

// validates if a cache is valid or not based on the timestamp and config filepath
bool IsConfigCacheValid(const char *configFilePath)
{
    LTRACE(true, "checking if cache is valid...\n");

    try
    {
        CreateCacheDirectories();
    }
    catch(Y::Error err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create cache directories.\n\t", err.what(), "\n");
    }

    // read config file metadata.
    std::string configMetaFilePath = std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_CONFIG_METADATA_CACHE_FILENAME;
    if(!fs::exists(configMetaFilePath.c_str()))
    {
        LTRACE(true, "couldn't find the cache file: config.cache, cache is invalid.\n");
        return false;
    }

    std::ifstream configMetaFile(configMetaFilePath.c_str());
    if(!configMetaFile.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open cache file to read config file's metadata.\n");
        return false;
    }

    std::time_t currentLastTimeSinceWrite = GetTimeSinceLastWrite(configFilePath);
    u64 currentFileSize                   = GetFileSize(configFilePath);

    std::string path;
    std::string cachedConfigTimeStr;
    std::string cachedConfigSizeStr;

    std::string line;
    while(std::getline(configMetaFile, line))
    {
        std::istringstream iss(line);
        iss >> path;
        iss >> cachedConfigTimeStr;
        iss >> cachedConfigSizeStr;
    }

    u64 cachedConfigSize = std::atoi(cachedConfigSizeStr.c_str());

    if(ToAbsolutePath(path) != ToAbsolutePath(configFilePath))
    {
        LTRACE(true, "config file location changed. cache is invalid.\n");
        return false;
    }

    if(currentFileSize != cachedConfigSize || ToString(currentLastTimeSinceWrite) != cachedConfigTimeStr)
    {
        if(currentFileSize != cachedConfigSize)
        {
            LTRACE(true, "config file's SIZE have been updated. cache is invalid.\n");
            LTRACE(true, "\tcached size: ", cachedConfigSize, "\n");
            LTRACE(true, "\tfound file size: ", currentFileSize, "\n");
        }
        else
        {
            LTRACE(true, "config file's LAST TIME SINCE WRITE have been updated. cache is invalid.\n");
            LTRACE(true, "\tcached time as string:\t", cachedConfigTimeStr, "\n");
            LTRACE(true, "\tcached time as time_t:\t", ToString(ToTimeT(cachedConfigTimeStr)), "\n");
            LTRACE(true, "\tfound file time:\t", ToString(currentLastTimeSinceWrite), "\n");
        }

        return false;
    }

    // read timestamp
    std::ifstream timeCacheFile(std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_TIMESTAMP_CACHE_FILENAME);
    std::string timestamp;
    if(timeCacheFile.is_open())
    {
        std::getline(timeCacheFile, timestamp);
        timeCacheFile.close();
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open cache file to read timestamp.\n");
        return false;
    }

    // get current time
    auto now                = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // calc difference in seconds
    f64 timeDiff = std::difftime(currentTime, ToTimeT(timestamp));

    if(timeDiff > YMAKE_TIMESTAMP_THRESHHOLD_SEC)
    {
        LTRACE(true, "time diff is bigger than threshhold. cache is invalid.\n");
        return false; // cache is outdated.
    }

    // if it reached here, then cache is valid.
    return true;
}

// creates the cache. (overrites any existing cache files)
void CreateProjectsCache(const std::vector<Project> &projects, const char *configFilePath)
{
    try
    {
        CreateCacheDirectories();
        LTRACE(true, "created cache directory \'YMakeCache\'\n");
    }
    catch(Y::Error err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create cache directories.\n\t", err.what(), "\n");
    }

    // add timestamp
    auto now                = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::string timestamp   = ToString(currentTime);

    // write the timestamp to a cache file
    std::ofstream timeCacheFile(std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_TIMESTAMP_CACHE_FILENAME);
    if(timeCacheFile.is_open())
    {
        timeCacheFile << timestamp;
        timeCacheFile.close();
        LTRACE(true, "created timestamp cache file correctly.\n");
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open cache file to write timestamp.\n");
    }

    // write the names of all project files to a cache file
    std::ofstream projectsCacheFile(std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_PROJECTS_CACHE_FILENAME);
    if(projectsCacheFile.is_open())
    {
        for(const Project &proj : projects)
        {
            projectsCacheFile << proj.name << "\n";
        }
        projectsCacheFile.close();
        LTRACE(true, "created cache file with project names.\n");
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open cache file to write project names.\n");
    }

    for(const Project &proj : projects)
    {
        std::ofstream projFile(std::string(YMAKE_CACHE_DIR) + "/" + proj.name + ".cache");
        if(projFile.is_open())
        {
            projFile << proj.Serialize();
            projFile.close();
        }
        else
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open project cache file for " + proj.name + ".\n");
        }
    }

    LTRACE(true, "created cache for each project in the config.\n");

    // config metadata cache file
    std::string configCachePath(YMAKE_CACHE_DIR);
    configCachePath += std::string("/") + YMAKE_CONFIG_METADATA_CACHE_FILENAME;
    std::ofstream configCache(configCachePath);
    if(configCache.is_open())
    {
        configCache << configFilePath << " " << ToString(GetTimeSinceLastWrite(configFilePath)) << " "
                    << GetFileSize(configFilePath) << "\n";
    }

    configCache.close();

    LTRACE(true, "created cache for config file metadata.\n");
}

// loads from cache without checking cache validation.
std::vector<Project> LoadProjectsCache()
{
    LTRACE(true, "loading config files from cache.\n");

    std::vector<Project> projects;

    std::ifstream namesCacheFile(std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_PROJECTS_CACHE_FILENAME);
    if(!namesCacheFile.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't load the projects cache file.\n");
        return {};
    }

    std::string name;
    while(std::getline(namesCacheFile, name))
    {
        std::ifstream projectFile(std::string(YMAKE_CACHE_DIR) + "/" + name + ".cache");
        if(!projectFile.is_open())
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open project cache file for " + name + ".\n");
            continue; // skip that project and continue to the next one.
        }

        std::stringstream buff;
        buff << projectFile.rdbuf();

        Project proj;
        proj.Deserialize(buff.str());
        projects.push_back(proj);
        projectFile.close();
    }

    namesCacheFile.close();

    LTRACE(true, "successfully loaded projects' config from cache.\n");

    return projects;
}

// loads either from cache or config file, depending on cache state/validation.
std::vector<Project> SafeLoadProjectsFromCache(const char *configPath)
{
    std::vector<Project> projects;
    if(Cache::IsConfigCacheValid(configPath))
    {
        projects = Cache::LoadProjectsCache();
    }
    else
    {
        projects = Parse::LoadProjectsFromConfig(configPath);
        Cache::CreateProjectsCache(projects, configPath);
    }

    return projects;
}

// returns true if cache removed successfully.
bool RemoveAllCache()
{
    // remove all cache files.
    const fs::path cacheDir = YMAKE_CACHE_DIR;

    if(!fs::exists(cacheDir) || !fs::is_directory(cacheDir))
        return true;

    try
    {
        fs::remove_all(cacheDir);
        return true;
    }
    catch(const fs::filesystem_error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "Failed to remove cache files.\n\t", err.what(), "\n");
        return false;
    }
}

// FILE METADATA

bool HasFileChanged(const std::string &filepath, const FileMetadata &cachedMetadata)
{
    if(!fs::exists(filepath))
        return true;

    auto currentLastWriteTime = GetTimeSinceLastWrite(filepath.c_str());
    auto currentFileSize      = GetFileSize(filepath.c_str());

    return (currentLastWriteTime != cachedMetadata.lastWriteTime || currentFileSize != cachedMetadata.fileSize);
}

void SaveMetadataCache(const std::string &filepath, const std::unordered_map<std::string, FileMetadata> &metadataCache)
{
    std::ofstream cacheFile(filepath);
    if(!cacheFile.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't save cache to file: ", filepath, "\n");
        throw Y::Error("couln't save file metadata cache.\n");
    }

    LTRACE(true, "saving metadata cache to disk...\n");

    // add data to the file.
    for(auto &[path, data] : metadataCache)
    {
        cacheFile << ToAbsolutePath(path) << " " << data.lastWriteTime << " " << data.fileSize << "\n";
    }

    LTRACE(true, "successfully created metadata cache at path: ", filepath, "\n");

    cacheFile.close();
}

// create metadata(vec of filepaths, projCacheDir) -> void
void CreateMetadataCache(const std::vector<std::string> files, std::string dir)
{
    // create dirs.
    try
    {
        CreateCacheDirectories();
        CreateDir(dir.c_str());
    }
    catch(Y::Error err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create cache directories.\n\t", err.what(), "\n");
    }

    std::unordered_map<std::string, FileMetadata> metadata;

    // load data.
    for(const auto &file : files)
    {
        auto currentLastWriteTime = GetTimeSinceLastWrite(file.c_str());
        auto currentFileSize      = fs::file_size(file);

        metadata[file] = FileMetadata{currentLastWriteTime, currentFileSize};
    }

    LTRACE(true, "created metadata cache (in program) successfully.\n");

    std::string filename = std::string(dir) + "/" + YMAKE_METADATA_CACHE_FILENAME;
    // save to file.
    try
    {
        SaveMetadataCache(filename.c_str(), metadata);
    }
    catch(Y::Error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create metadata for path: ", filename, "\n");
        return;
    }
}

std::unordered_map<std::string, FileMetadata> LoadMetadataCache(const std::string &cacheFilepath)
{
    LTRACE(true, "trying to load metadata cache from disk...\n");

    std::unordered_map<std::string, FileMetadata> metadataCache;

    if(!fs::exists(cacheFilepath.c_str()))
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't load cache from file: ", cacheFilepath,
             "\n\tfile doesn't exist.\n");
        throw Y::Error("couldn't load cache from a file.\n");
    }

    std::ifstream cacheFile(cacheFilepath);
    if(!cacheFile.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open cache file: ", cacheFilepath, "\n");
        throw Y::Error("couldn't open a cache file.");
    }

    std::string line;
    while(std::getline(cacheFile, line))
    {
        std::istringstream iss(line);
        std::string filepath;
        FileMetadata metadata;
        if(iss >> filepath >> metadata.lastWriteTime >> metadata.fileSize)
        {
            metadataCache[filepath.c_str()] = metadata;
        }
    }

    LTRACE(true, "successfully loaded metadata cache from disk.\n");

    return metadataCache;
}

void UpdateMetadataCache(const std::string &file, const char *projCacheDir)
{
    std::string cachefilepath = std::string(projCacheDir) + "/" + YMAKE_METADATA_CACHE_FILENAME;
    std::map<std::string, FileMetadata> cacheData;

    // Load existing cache data
    std::ifstream cachefile_in(cachefilepath);
    if(cachefile_in.is_open())
    {
        std::string filepath;
        u64 filesize;
        std::string writeTime;
        while(cachefile_in >> filepath >> writeTime >> filesize)
        {
            FileMetadata fm;
            fm.lastWriteTime    = ToTimeT(writeTime);
            fm.fileSize         = filesize;
            cacheData[filepath] = fm;
        }
        cachefile_in.close();
    }

    // Update or add the new file data
    FileMetadata fm;
    fm.lastWriteTime = GetTimeSinceLastWrite(file.c_str());
    fm.fileSize      = GetFileSize(file.c_str());
    cacheData[file]  = fm;

    // Write the updated cache data back to the file
    std::ofstream cachefile_out(cachefilepath, std::ios::out | std::ios::trunc);
    if(!cachefile_out.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create a cache file.\n");
        throw Y::Error("couldn't create a metadata cache file.\n");
    }

    for(const auto &entry : cacheData)
    {
        cachefile_out << entry.first << " " << entry.second.lastWriteTime << " " << entry.second.fileSize << "\n";
    }

    cachefile_out.close();
}

bool HasSourceFileChanged(const char *path, std::unordered_map<std::string, FileMetadata> &metadataCache)
{
    std::ifstream file(path);
    if(!file.is_open())
    {
        throw Y::Error("couldn't open a file.");
        return true;
    }

    if(GetFileSize(path) != metadataCache[path].fileSize ||
       GetTimeSinceLastWrite(path) != metadataCache[path].lastWriteTime)
        return true;

    return false;
}

bool RemoveAllMetadataCache()
{
    const std::string cacheDir = YMAKE_CACHE_DIR;

    try
    {
        if(!fs::exists(cacheDir) || !fs::is_directory(cacheDir))
        {
            for(const auto &entry : fs::directory_iterator(cacheDir))
            {
                if(fs::is_directory(entry))
                    fs::remove_all(entry);
            }
        }
        return true;
    }
    catch(const fs::filesystem_error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "error removing cache.\n\t", err.what(), "\n");
        return false;
    }
}

std::vector<std::string> GetSrcFilesRecursive(const std::string &dirPath)
{
    if(!fs::exists(dirPath))
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't find directory: ", dirPath, "\n");
        throw Y::Error("couldn't find directory.");
    }

    std::vector<std::string> files;
    for(const auto &entry : fs::recursive_directory_iterator(dirPath))
    {
        if(entry.is_regular_file())
        {
            std::string ext = entry.path().extension().string();
            if(ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx")
            {
                files.push_back(ToAbsolutePath(entry.path().string()));
            }
        }
    }

    LTRACE(true, "source files found in ", dirPath, ": \n");
    for(const auto &file : files)
    {
        LTRACE(true, "\t", file, "\n");
    }

    return files;
}

std::vector<std::string> GetFilesWithExt(const std::string &path, const std::string &ext)
{
    std::vector<std::string> files;
    for(const auto &entry : fs::recursive_directory_iterator(path))
    {
        if(entry.is_regular_file())
        {
            std::string fileExt = entry.path().extension().string();
            if(fileExt == ext)
            {
                files.push_back(ToAbsolutePath(entry.path().string()));
            }
        }
    }

    return files;
}

//_______________________________ PREPROCESSED/SRC_FILE CACHE ____________________

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

FileType GetFileType(const string &filepath)
{
    string path(filepath);
    string extension = ToLower(fs::path(path).extension().string());

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

    std::string outputPath = path + "/src" + "/" + GetHashedFileNameFromPath(file);
    outputPath += std::string(".i");

    LTRACE(true, "about to create preprocessed cache at: ", outputPath, "\n");

    command += (compiler == Compiler::MSVC) ? COMP_MSVC_OUTPUT_FILE(outputPath) : COMP_OUTPUT_FILE(outputPath);

    // add includes
    for(auto include : proj.includeDirs)
        command +=
            (compiler == Compiler::MSVC) ? COMP_MSVC_INCLUDE_DIR(include.c_str()) : COMP_INCLUDE_DIR(include.c_str());

    // supress output.
    command += (compiler == Compiler::MSVC) ? COMP_MSVC_SUPPRESS_OUTPUT : COMP_SUPPRESS_OUTPUT;

    i32 result = std::system(command.c_str());
    LASSERT(result == 0, RED_TEXT("[YMAKE COMPILE ERROR]: "), "failed to compile source file: ", file, "\n\t",
            "exit code: ", result, "\n");

    return outputPath;
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
} // namespace Y::Cache