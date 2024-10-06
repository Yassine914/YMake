#pragma once

#include "../core/defines.h"
#include "../core/logger.h"
#include "../core/error.h"

#include <toml++/toml.h>

#include <type_traits>
#include <vector>
#include <string>
#include <optional>

enum class Lang
{
    C   = 0,
    CPP = 1,
};

enum class BuildType
{
    EXECUTABLE,
    SHARED_LIB,
    STATIC_LIB,
};

struct Library // dependency
{
    std::string name;
    std::string path;
    BuildType type;

    Library() {}
    Library(std::string name, std::string path) : name{name}, path{path} {}
    Library(std::string name, std::string path, BuildType type) : name{name}, path{path}, type{type} {}
};

// clang-format off

template <typename T>
std::string SerializeVectorOfEnums(const std::vector<T> &vec)
{
    std::ostringstream oss;
    oss << vec.size() << "\n";
    for (const T &item : vec)
    {
        oss << static_cast<i32>(item) << "\n";
    }
    return oss.str();   
}

// Serialize a vector of items
template <typename T>
std::string SerializeVector(const std::vector<T> &vec)
{
    std::ostringstream oss;
    oss << vec.size() << "\n";
    for (const auto &item : vec)
    {
        oss << item << "\n";
    }
    return oss.str();
}

// Deserialize a vector of items
template <typename T>
std::vector<T> DeserializeVector(std::istringstream &iss)
{
    std::vector<T> vec;
    std::string line;
    std::getline(iss, line);
    usize size = std::stoul(line);
    vec.resize(size);
    for (usize i = 0; i < size; ++i)
    {
        std::getline(iss, line);
        std::istringstream itemStream(line);
        itemStream >> vec[i];
    }
    return vec;
}

template <typename T>
std::vector<T> DeserializeVectorOfEnums(std::istringstream &iss)
{
    std::vector<T> vec;
    std::string line;
    std::getline(iss, line);
    usize size = std::stoul(line);
    vec.resize(size);
    for (usize i = 0; i < size; ++i)
    {
        std::getline(iss, line);
        i32 value = std::stoi(line);
        vec[i] = static_cast<T>(value);
    }
    return vec;
}

// clang-format on

class Project
{
    public:
    std::string name{};
    std::string version;

    std::vector<Lang> langs;

    std::string src{};
    std::string env;

    // langs
    u32 cppStd{};
    u32 cStd{};
    std::string cppCompiler{};
    std::string cCompiler{};

    // build
    BuildType buildType;
    std::string buildDir;

    // libs
    std::vector<std::string> includeDirs;
    std::vector<Library> libs;

    std::vector<std::string> preBuiltLibs;
    std::vector<std::string> sysLibs;

    // compiler specific
    std::vector<std::string> definesRelease;
    std::vector<std::string> definesDebug;

    i32 optimizationDebug;
    i32 optimizationRelease;

    std::vector<std::string> flagsDebug;
    std::vector<std::string> flagsRelease;

    public:
    Project() {}
    Project(std::string name) : name{name} {}

    std::string Serialize() const
    {
        std::ostringstream oss;
        oss << name << "\n";
        oss << version << "\n";

        oss << static_cast<int>(buildType) << "\n";

        oss << buildDir << "\n";
        oss << cppStd << "\n";
        oss << cStd << "\n";
        oss << cppCompiler << "\n";
        oss << cCompiler << "\n";

        oss << src << "\n";
        oss << env << "\n";

        // Serialize vectors
        oss << SerializeVectorOfEnums<Lang>(langs);
        oss << SerializeVector(includeDirs);

        oss << libs.size() << "\n";
        for(auto lib : libs)
        {
            oss << lib.name << "\n";
            oss << lib.path << "\n";
            oss << (int)lib.type << "\n";
        }

        oss << SerializeVector(preBuiltLibs);
        oss << SerializeVector(sysLibs);

        oss << SerializeVector(definesRelease);
        oss << SerializeVector(definesDebug);

        oss << optimizationDebug << "\n" << optimizationRelease << "\n";

        oss << SerializeVector(flagsDebug);
        oss << SerializeVector(flagsRelease);
        return oss.str();
    }

    void Deserialize(const std::string &data)
    {
        std::istringstream iss(data);
        std::string line;

        std::getline(iss, name);
        std::getline(iss, version);

        int buildTypeInt;
        std::getline(iss, line);
        buildTypeInt = std::stoi(line);
        buildType    = static_cast<BuildType>(buildTypeInt);

        std::getline(iss, buildDir);
        std::getline(iss, line);
        cppStd = std::stoul(line);
        std::getline(iss, line);
        cStd = std::stoul(line);
        std::getline(iss, cppCompiler);
        std::getline(iss, cCompiler);
        std::getline(iss, src);
        std::getline(iss, env);

        // Deserialize vectors
        langs       = DeserializeVectorOfEnums<Lang>(iss);
        includeDirs = DeserializeVector<std::string>(iss);

        std::string sz;
        std::getline(iss, sz);
        usize size = std::atoi(sz.c_str());
        for(usize i = 0; i < size; i++)
        {
            std::string libName;
            std::getline(iss, libName);
            std::string libPath;
            std::getline(iss, libPath);
            std::string libTypeStr;
            std::getline(iss, libTypeStr);
            BuildType libType = (BuildType)std::atoi(libTypeStr.c_str());

            libs.push_back(Library(libName, libPath, libType));
        }

        preBuiltLibs = DeserializeVector<std::string>(iss);
        sysLibs      = DeserializeVector<std::string>(iss);

        definesRelease = DeserializeVector<std::string>(iss);
        definesDebug   = DeserializeVector<std::string>(iss);

        std::string opDebug, opRelease;
        std::getline(iss, opDebug);
        optimizationDebug = std::atoi(opDebug.c_str());
        std::getline(iss, opRelease);
        optimizationRelease = std::atoi(opRelease.c_str());

        flagsDebug   = DeserializeVector<std::string>(iss);
        flagsRelease = DeserializeVector<std::string>(iss);
    }

    void OutputInfo()
    {
        LLOG(BLUE_TEXT("Project: "), name, "\n\n");

        LLOG(PURPLE_TEXT("\tLanguages: \n"));
        for(auto lang : langs)
        {
            if(lang == Lang::CPP)
            {
                LLOG("\t\tC++\tstd: c++", cppStd, "\tC++ Compiler: ", cppCompiler, "\n");
            }
            else
            {
                LLOG("\t\tC\tstd: c", cStd, "\tC Compiler: ", cCompiler, "\n");
            }
        }

        LLOG(GREEN_TEXT("\tBuild Type: "));
        switch(buildType)
        {
        case BuildType::EXECUTABLE:
            LLOG("Executable");
            break;
        case BuildType::SHARED_LIB:
            LLOG("Shared (Dynamic) Library");
            break;
        case BuildType::STATIC_LIB:
            LLOG("Static Library");
            break;
        }
        LLOG("\n");

        LLOG(CYAN_TEXT("\tSource Directory: "), src, "\n");
        if(env != "")
            LLOG(CYAN_TEXT("\t.env Directory: "), env, "\n");

        LLOG(BLUE_TEXT("\tInclude Directories: \n"));
        for(std::string inc : includeDirs)
        {
            LLOG("\t\t", inc, "\n");
        }

        if(libs.size() != 0)
        {
            LLOG(GREEN_TEXT("\tLibraries: \n"));
            for(Library lib : libs)
            {
                LLOG("\t\tName: ", lib.name, "\n");
                LLOG("\t\t\tPath: ", lib.path, "\n");
                LLOG("\t\t\tType: ",
                     lib.type == BuildType::EXECUTABLE   ? "Executable"
                     : lib.type == BuildType::SHARED_LIB ? "Shared (Dynamic) Library"
                                                         : "Static Library",
                     "\n");
            }
        }

        if(preBuiltLibs.size() != 0)
        {
            LLOG(GREEN_TEXT("\tPre-Built Libraries: \n"));
            for(const auto &lib : preBuiltLibs)
            {
                LLOG("\t\t", lib, "\n");
            }
        }

        if(sysLibs.size() != 0)
        {
            LLOG(GREEN_TEXT("\tSystem Libraries: \n"));
            for(const auto &lib : sysLibs)
            {
                LLOG("\t\t", lib, "\n");
            }
        }

        if(definesRelease.size() != 0 || definesDebug.size() != 0)
        {
            LLOG(PURPLE_TEXT("\tDefines:\n"));

            if(definesRelease.size() != 0)
            {
                LLOG(GREEN_TEXT("\t\tRelease Defines:\t"))
                for(std::string def : definesRelease)
                {
                    LLOG(def, " ");
                }
                LLOG("\n");
            }

            if(definesDebug.size() != 0)
            {
                LLOG(GREEN_TEXT("\t\tDebug Defines:\t"))
                for(std::string def : definesDebug)
                {
                    LLOG(def, " ");
                }
                LLOG("\n");
            }

            LLOG("\n");
        }

        LLOG(CYAN_TEXT("\tOptimization Level: \n"));
        LLOG("\t\tDebug:\t", optimizationDebug, "\n");
        LLOG("\t\tRelease:\t", optimizationRelease, "\n");

        if(flagsDebug.size() != 0 || flagsRelease.size() != 0)
        {
            LLOG(CYAN_TEXT("\tCompiler Flags:\n"));
            if(flagsDebug.size() != 0)
            {
                LLOG(PURPLE_TEXT("\t\tDebug:\t"));
                for(const auto &fl : flagsDebug)
                {
                    LLOG(fl, " ");
                }
                LLOG("\n");
            }

            if(flagsRelease.size() != 0)
            {
                LLOG(PURPLE_TEXT("\t\tRelease:\t"));
                for(const auto &fl : flagsRelease)
                {
                    LLOG(fl, " ");
                }
                LLOG("\n");
            }
        }
    }
};

namespace Y::Parse {

std::vector<Project> GetProjects(const char *configPath);

std::optional<std::vector<std::string>> GetArrayFromToml(const toml::table &config, const std::string &tableName,
                                                         const std::string &arrayName);

// clang-format off
template<typename T>
std::optional<T> GetValueFromTOML(const toml::table &tbl, const std::string &key);

void ParseProjectData(const toml::table &config, Project &proj);

Project LoadProjectFromConfig(const char *filepath, const char *projName);
std::vector<Project> LoadProjectsFromConfig(const char *filepath);

// clang-format on

} // namespace Y::Parse