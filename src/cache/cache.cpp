#include "cache.h"

#include <filesystem>
namespace fs = std::filesystem;

#include <chrono>
using namespace std::chrono;
using namespace std::chrono_literals;

#include <sstream>
#include <vector>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>

namespace Y::Cache {

// PROJECT METADATA

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

std::time_t ToTimeT(std::string time)
{
    std::tm tm = {};
    std::istringstream ss(time);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::mktime(&tm);
}

std::time_t GetTimeSinceLastWrite(const char *file)
{
    auto currentLastWriteTime = fs::last_write_time(file);
    return std::chrono::duration_cast<std::chrono::seconds>(currentLastWriteTime.time_since_epoch()).count();
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
    try
    {
        CreateCacheDirectories();
    }
    catch(Y::Error err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create cache directories.\n\t", err.what(), "\n");
    }

    std::string cachefilepath = std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_CONFIG_PATH_CACHE_FILENAME;
    if(!fs::exists(cachefilepath.c_str()))
        return false;

    // read config filepath
    std::ifstream filepathCacheFile(std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_CONFIG_PATH_CACHE_FILENAME);
    std::string filepath;
    if(filepathCacheFile.is_open())
    {
        std::getline(filepathCacheFile, filepath);
        filepathCacheFile.close();
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open cache file to read config file path.\n");
        return false;
    }

    if(filepath != configFilePath)
        return false;

    // read config file metadata.
    std::string configMetaFilePath(YMAKE_CACHE_DIR);
    configMetaFilePath += std::string("/") + YMAKE_CONFIG_METADATA_CACHE_FILENAME;
    auto currentLastTimeSinceWrite = GetTimeSinceLastWrite(configMetaFilePath.c_str());
    auto currentFileSize           = GetFileSize(configMetaFilePath.c_str());

    std::ifstream configMetaFile(configMetaFilePath.c_str());
    if(!configMetaFile.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open cache file to read config file's metadata.\n");
        return false;
    }

    std::string path;
    std::string cachedConfigTime;
    std::string cachedConfigSize;

    std::string line;
    while(std::getline(configMetaFile, line))
    {
        std::istringstream iss(line);
        iss >> path;
        iss >> cachedConfigTime;
        iss >> cachedConfigSize;
    }

    if(currentFileSize != std::atoi(cachedConfigSize.c_str()) || currentLastTimeSinceWrite != ToTimeT(cachedConfigTime))
    {
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

    // convert the timestamp string back to a time_t object
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    std::time_t cacheTime = std::mktime(&tm);

    // get current time
    auto now                = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // calc difference in seconds
    f64 timeDiff = std::difftime(currentTime, cacheTime);

    if(timeDiff > YMAKE_TIMESTAMP_THRESHHOLD_SEC)
        return false; // cache is outdated.

    return true;
}

// creates the cache. (overrites any existing cache files)
void CreateProjectsCache(const std::vector<Project> &projects, const char *configFilePath)
{
    try
    {
        CreateCacheDirectories();
    }
    catch(Y::Error err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create cache directories.\n\t", err.what(), "\n");
    }

    // add timestamp
    auto now                = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // convert to a readable format
    std::stringstream ss;
    ss << std::put_time(std::localtime(&currentTime), "%Y-%m-%d %H:%M:%S");
    std::string timestamp = ss.str();

    // write the timestamp to a cache file
    std::ofstream timeCacheFile(std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_TIMESTAMP_CACHE_FILENAME);
    if(timeCacheFile.is_open())
    {
        timeCacheFile << timestamp;
        timeCacheFile.close();
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open cache file to write timestamp.\n");
    }

    // write the config file path to a cache file.
    std::ofstream filepathCacheFile(std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_CONFIG_PATH_CACHE_FILENAME);
    if(filepathCacheFile.is_open())
    {
        filepathCacheFile << configFilePath << "\n";
        filepathCacheFile.close();
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't open cache file to write config file path.\n");
    }

    // Write the names of all project files to a cache file
    std::ofstream projectsCacheFile(std::string(YMAKE_CACHE_DIR) + "/" + YMAKE_PROJECTS_CACHE_FILENAME);
    if(projectsCacheFile.is_open())
    {
        for(const Project &proj : projects)
        {
            projectsCacheFile << proj.name << "\n";
        }
        projectsCacheFile.close();
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

    // config metadata cache file
    std::string configCachePath(YMAKE_CACHE_DIR);
    configCachePath += std::string("/") + YMAKE_CONFIG_METADATA_CACHE_FILENAME;
    std::ofstream configCache(configCachePath);
    if(configCache.is_open())
    {
        configCache << configFilePath << " " << GetTimeSinceLastWrite(configCachePath.c_str()) << " "
                    << GetFileSize(configCachePath.c_str()) << "\n";
    }
}

// loads from cache without checking cache validation.
std::vector<Project> LoadProjectsCache()
{
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

    return projects;
}

// loads either from cache or config file, depending on cache state/validation.
std::vector<Project> SafeLoadProjectsFromCache(const char *configPath)
{
    std::vector<Project> projects;
    // TODO: either fix the isConfigCacheValid func or this one.
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

void SaveMetadataCache(const std::string &dir, const std::unordered_map<std::string, FileMetadata> &metadataCache)
{
    std::ofstream cacheFile(dir);
    if(!cacheFile.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't save cache to file: ", YMAKE_CACHE_DIR, "/",
             YMAKE_METADATA_CACHE_FILENAME, "\n");
        throw Y::Error("couln't save file metadata cache.\n");
    }

    // add data to the file.
    for(auto &[path, data] : metadataCache)
    {
        cacheFile << ToAbsolutePath(path) << " " << data.lastWriteTime << " " << data.fileSize << "\n";
    }

    cacheFile.close();
}

// create metadata(vec of filepaths, proj.name) -> void
void CreateMetadataCache(const std::vector<std::string> files, std::string dir)
{
    std::string path(dir);

    // create dirs.
    try
    {
        CreateDir(path.c_str());
        CreateCacheDirectories();
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

    // save to file.
    try
    {
        SaveMetadataCache(path.c_str(), metadata);
    }
    catch(Y::Error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't create metadata for path: ", path, "\n");
        return;
    }
}

std::unordered_map<std::string, FileMetadata> LoadMetadataCache(const std::string &cacheFilepath)
{
    std::unordered_map<std::string, FileMetadata> metadataCache;
    std::ifstream cacheFile(cacheFilepath);
    if(!cacheFile.is_open())
    {
        std::cerr << "Couldn't open cache file: " << cacheFilepath << std::endl;
        throw std::runtime_error("Couldn't open cache file.");
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

    return metadataCache;
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

} // namespace Y::Cache