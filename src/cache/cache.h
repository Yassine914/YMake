#pragma once

#include "../core/defines.h"
#include "../core/logger.h"
#include "../core/error.h"

#include "../toml/parser.h"

#include <unordered_map>

namespace Y::Cache {

// PROJECT METADATA

void CreateDir(const char *path);

void CreateCacheDirectories();

bool DirExists(const char *path);
bool FileExists(const char *path);

std::time_t ToTimeT(const std::string &time);

std::time_t GetTimeSinceLastWrite(const char *file);

u64 GetFileSize(const char *file);

std::string ConcatenatePath(const std::string &basePath, const std::string &relativePath);

std::string ToAbsolutePath(const std::string &path);

bool IsConfigCacheValid(const char *configFilePath);

// creates the cache. (overrites any existing cache files)
void CreateProjectsCache(const std::vector<Project> &projects, const char *configFilePath);

std::vector<Project> LoadProjectsCache();

std::vector<Project> SafeLoadProjectsFromCache(const char *configPath = YMAKE_DEFAULT_FILE);

bool RemoveAllCache();

// PROJECT FILES

struct FileMetadata
{
    std::time_t lastWriteTime;
    u64 fileSize;
};

bool HasFileChanged(const std::string &filepath, const FileMetadata &cachedMetadata);
bool HasSourceFileChanged(const char *path, const std::unordered_map<std::string, FileMetadata> &metadataCache);

void CreateMetadataCache(const std::vector<std::string> files, std::string projectName);

std::unordered_map<std::string, FileMetadata> LoadMetadataCache(const std::string &cacheFileath);
void UpdateMetadataCache(const std::string &file, const char *projCacheDir);

// get (.c or .cpp or .cc) files recursively. (outputs absolut path.)
std::vector<std::string> GetSrcFilesRecursive(const std::string &dirPath);
std::vector<std::string> GetFilesWithExt(const std::string &path, const std::string &ext);
bool RemoveAllMetadataCache();

//_______________________________ PREPROCESSED/SRC_FILE CACHE ____________________

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

Compiler WhatCompiler(const std::string &compiler);
FileType GetFileType(const std::string &filepath);

std::string PreprocessUnit(const Project &proj, const std::string &file, const std::string &path);

void CreatePreprocessedCache(const std::vector<std::string> &files, const char *projCacheDir);

void UpdatePreprocessedCache(const std::string &file, const char *projCacheDir);

std::map<std::string, u64> LoadPreprocessedCache(const char *projCacheDir);

std::vector<std::string> GeneratePreprocessedFiles(const Project &proj, const std::vector<std::string> &files,
                                                   const char *path);

} // namespace Y::Cache