#include "parser.h"

#include <filesystem>
namespace fs = std::filesystem;

namespace Y::Parse {

std::vector<Project> GetProjects(const char *filepath)
{
    std::vector<Project> projects;

    std::ifstream file(filepath, std::ios::in);
    if(!file.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't load config file in path: ", filepath, "\n");
        exit(1);
    }

    std::string line;
    while(std::getline(file, line))
    {
        std::stringstream ss{line};
        std::string word;

        ss >> word;
        if(word == "#")
        {
            ss >> word;
            if(word == "PROJECT:")
            {
                ss >> word;
                projects.push_back(Project(word));
            }
        }
    }

    file.close();
    return projects;
}

std::optional<std::vector<std::string>> GetArrayFromToml(const toml::table &config, const std::string &tableName,
                                                         const std::string &arrayName)
{
    // Check if the table exists
    if(const auto *table = config[tableName].as_table())
    {
        // Check if the array exists within the table
        if(const auto *array = (*table)[arrayName].as_array())
        {
            std::vector<std::string> result;
            for(const auto &value : *array)
            {
                if(value.is_string())
                {
                    result.push_back(value.as_string()->get());
                }
            }
            return result;
        }
    }
    return std::nullopt;
}

// clang-format off
template<typename T>
std::optional<T> GetValueFromTOML(const toml::table &tbl, const std::string &key)
{
    if(tbl.contains(key))
    {
        if(auto val = tbl[key].value<T>())
            return val;
    }
    for(const auto &[k, v] : tbl)
    {
        if(v.is_table())
        {
            if(auto val = GetValueFromTOML<T>(*v.as_table(), key))
                return val;
        }
    }
    return std::nullopt;
}

// clang-format on

void ParseLibrary(const toml::table *lib, Project &proj)
{
    if(!lib->is_table())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "libraries for project: ", proj.name, " aren't setup correctly.\n");
        throw Y::Error("problem parsing the libraries for a project");
    }

    auto libTable = *(lib->as_table());

    Library library;
    auto name = GetValueFromTOML<std::string>(libTable, "name");
    if(name.has_value())
        library.name = name.value();
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "didn't specify a name for a library for project: ", proj.name, "\n");
        throw Y::Error("didn't specify a name for a library\n");
    }

    auto path = GetValueFromTOML<std::string>(libTable, "path");
    if(path.has_value())
        library.path = path.value();
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "didn't specify a path for library: ", library.name,
             " for project: ", proj.name, "\n");
        throw Y::Error("didn't specify a path for a library");
    }

    auto type = GetValueFromTOML<std::string>(libTable, "type");
    if(type.has_value())
    {
        if(type.value() == "shared")
            library.type = BuildType::SHARED_LIB;
        else if(type.value() == "static")
            library.type = BuildType::STATIC_LIB;
        else if(type.value() == "executable")
            library.type = BuildType::EXECUTABLE;
        else
        {
            LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "specified build mode for ", library.name, " for project: ", proj.name,
                 " is not supported\n");
            LLOG("\tsupported modes: [executable, static, shared]\n");
            LLOG("\tusing build type \'executable\' by default.\n");
            library.type = BuildType::EXECUTABLE;
        }
    }
    else
    {
        LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "didn't specify the build type for library: ", library.name,
             " for project: ", proj.name, "\n");
        LLOG("\tusing build type \'executable\' by default.\n");
        library.type = BuildType::EXECUTABLE;
    }

    proj.libs.push_back(library);

    auto includePath = GetValueFromTOML<std::string>(libTable, "include");
    if(includePath.has_value())
    {
        bool found = false;
        for(std::string dir : proj.includeDirs)
        {
            if(includePath.value() == dir)
            {
                found = true;
                break;
            }
        }

        if(!found)
            proj.includeDirs.push_back(includePath.value());
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "didn't specify an include path for library: ", library.name,
             " for project: ", proj.name, "\n");
        throw Y::Error("didn't specify an include path for a library.\n");
    }
}

void ParseProjectData(const toml::table &config, Project &proj)
{
    auto table = *config[proj.name].as_table();

    if(table.empty())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't find project table: ", proj.name, " in the config file\n");
        throw Y::Error("couldn't find project in the config file.");
    }

    // src dir
    auto src = GetValueFromTOML<std::string>(table, "src");
    if(src.has_value())
    {
        proj.src = src.value();
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "source path wasn't specified for project: ", proj.name, "\n");
        throw Y::Error("source path wasn't specified for a project.\n");
    }

    // env dir
    auto env = GetValueFromTOML<std::string>(table, "env");
    if(env.has_value())
        proj.env = env.value();

    // languages
    auto langs = table["lang"];
    // auto langs = GetArrayFromToml(config, proj.name.c_str(), "lang");

    if(!langs)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "didn't specify languages used in project: ", proj.name, "\n");
        throw Y::Error("didn't specify languages used in project.\n");
    }

    if(langs.is_array())
    {
        auto langsArr = GetArrayFromToml(config, proj.name.c_str(), "lang").value();
        for(auto lang : langsArr)
        {
            bool cxx = false, c = false;
            if(lang == "C++" || lang == "CXX" || lang == "CPP")
            {
                proj.langs.push_back(Lang::CXX);
                cxx = true;
            }
            else if(lang == "C")
            {
                proj.langs.push_back(Lang::C);
                c = true;
            }
            else
            {
                LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "ignoring unsopported language: ", lang,
                     ", from project: ", proj.name, "\n");
            }

            if(cxx)
            {
                auto cppTable = *table["cpp"].as_table();

                // std.
                auto cppStd = GetValueFromTOML<i64>(cppTable, "std");
                if(!cppStd.has_value())
                {
                    LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "didn't specify the C++ standard used in project: ", proj.name,
                         "\n");
                    LLOG("\tusing standard: c++14 by default.\n");
                    proj.cppStd = 14;
                }
                else
                {
                    proj.cppStd = cppStd.value();
                }

                // compiler
                auto cppCompiler = GetValueFromTOML<std::string>(cppTable, "compiler");
                if(!cppCompiler.has_value())
                {
                    LLOG(RED_TEXT("[YMAKE ERROR]: "), "didn't specify a C++ compiler for project: ", proj.name, "\n");
                    throw Y::Error("didn't specify a C++ compiler");
                }
                else
                {
                    proj.cppCompiler = cppCompiler.value();
                }
            }

            if(c)
            {
                auto cTable = *table["c"].as_table();

                // std.
                auto cStd = GetValueFromTOML<i64>(cTable, "std");
                if(!cStd.has_value())
                {
                    LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "didn't specify the C standard used in project: ", proj.name,
                         "\n");
                    LLOG("\tusing standard: c11 by default.\n");
                    proj.cStd = 11;
                }
                else
                {
                    proj.cStd = cStd.value();
                }

                // compiler
                auto cCompiler = GetValueFromTOML<std::string>(cTable, "compiler");
                if(!cCompiler.has_value())
                {
                    LLOG(RED_TEXT("[YMAKE ERROR]: "), "didn't specify a C compiler for project: ", proj.name, "\n");
                    throw Y::Error("didn't specify a C compiler");
                }
                else
                {
                    proj.cCompiler = cCompiler.value();
                }
            }
        }
    }
    else if(langs.is_string())
    {
        std::string lang = langs.as_string()->get();
        bool cxx = false, c = false;
        if(lang == "C++" || lang == "CXX" || lang == "CPP")
        {
            proj.langs.push_back(Lang::CXX);
            cxx = true;
        }
        else if(langs == "C")
        {
            proj.langs.push_back(Lang::C);
            c = true;
        }
        else
        {
            LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "ignoring unsopported language: ", lang, ", from project: ", proj.name,
                 "\n");
        }

        if(cxx)
        {
            auto cppTable = *table["cpp"].as_table();

            // std.
            auto cppStd = GetValueFromTOML<i64>(cppTable, "std");
            if(!cppStd.has_value())
            {
                LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "didn't specify the C++ standard used in project: ", proj.name,
                     "\n");
                LLOG("\tusing standard: c++14 by default.\n");
                proj.cppStd = 14;
            }
            else
            {
                proj.cppStd = cppStd.value();
            }

            // compiler
            auto cppCompiler = GetValueFromTOML<std::string>(cppTable, "compiler");
            if(!cppCompiler.has_value())
            {
                LLOG(RED_TEXT("[YMAKE ERROR]: "), "didn't specify a C++ compiler for project: ", proj.name, "\n");
                throw Y::Error("didn't specify a C++ compiler");
            }
            else
            {
                proj.cppCompiler = cppCompiler.value();
            }
        }

        if(c)
        {
            auto cTable = *table["c"].as_table();

            // std.
            auto cStd = GetValueFromTOML<i64>(cTable, "std");
            if(!cStd.has_value())
            {
                LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "didn't specify the C standard used in project: ", proj.name, "\n");
                LLOG("\tusing standard: c11 by default.\n");
                proj.cStd = 11;
            }
            else
            {
                proj.cStd = cStd.value();
            }

            // compiler
            auto cCompiler = GetValueFromTOML<std::string>(cTable, "compiler");
            if(!cCompiler.has_value())
            {
                LLOG(RED_TEXT("[YMAKE ERROR]: "), "didn't specify a C compiler for project: ", proj.name, "\n");
                throw Y::Error("didn't specify a C compiler");
            }
            else
            {
                proj.cCompiler = cCompiler.value();
            }
        }
    }

    // build
    auto buildTable = *table["build"].as_table();
    if(!buildTable.empty())
    {
        auto buildType = GetValueFromTOML<std::string>(buildTable, "type");

        if(buildType.has_value())
        {
            if(buildType.value() == "executable")
                proj.buildType = BuildType::EXECUTABLE;
            else if(buildType.value() == "shared")
                proj.buildType = BuildType::SHARED_LIB;
            else if(buildType.value() == "static")
                proj.buildType = BuildType::STATIC_LIB;
            else
            {
                LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "build type \'", buildType.value(), "\' for project \'", proj.name,
                     "\' is not one of the options.\n");
                LLOG("\tbuild type options are: executable, shared, static.\n");
                LLOG("\tusing executable build type by default\n");
                proj.buildType = BuildType::EXECUTABLE;
            }
        }
        else
        {
            LLOG(YELLOW_TEXT("[YMAKE ERROR]: "), "didn't specify build type for project: ", proj.name,
                 "\n\t using executable by default.\n");
            proj.buildType = BuildType::EXECUTABLE;
        }

        auto buildDir = GetValueFromTOML<std::string>(buildTable, "dir");

        if(buildDir.has_value())
        {
            proj.buildDir = buildDir.value();
        }
        else
        {
            LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "didn't specify build directory for project: ", proj.name,
                 "\n\tusing \'./bin/\' by default.\n");
            proj.buildDir = "./bin/";
        }
    }
    else
    {
        LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "didn't specify build directory or type for project: ", proj.name,
             "\n\tusing \'./bin/\' as directory, and \'executable\' as type by default.\n");
        proj.buildDir  = "./bin/";
        proj.buildType = BuildType::EXECUTABLE;
    }

    // include paths.
    auto includes = GetArrayFromToml(config, proj.name.c_str(), "includes");

    if(!includes.has_value())
    {
        LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "didn't specify any includes for project: ", proj.name, "\n\t using \'",
             proj.src, "\' by default.\n");
        proj.includeDirs.push_back(proj.src);
    }
    else
    {
        for(std::string inc : includes.value())
        {
            proj.includeDirs.push_back(inc);
        }
    }

    // libraries
    if(auto libsArray = table["libs"].as_array())
    {
        auto libs = *libsArray;

        if(!libs.empty())
        {
            for(const auto &lib : libs)
            {
                ParseLibrary(lib.as_table(), proj);
            }
        }
    }

    // ________ compiler specific _________

    // defines
    if(auto compilerTableConf = table["compiler"].as_table())
    {
        auto compilerTable = *compilerTableConf;

        if(!compilerTable.empty())
        {
            auto definesRelease = GetArrayFromToml(compilerTable, "defines", "release");
            if(definesRelease.has_value())
            {
                for(std::string define : definesRelease.value())
                {
                    proj.definesRelease.push_back(define);
                }
            }

            auto definesDebug = GetArrayFromToml(compilerTable, "defines", "debug");
            if(definesDebug.has_value())
            {
                for(std::string define : definesDebug.value())
                {
                    proj.definesDebug.push_back(define);
                }
            }

            auto flags = GetArrayFromToml(table, "compiler", "flags");
            if(flags.has_value())
            {
                for(std::string flag : flags.value())
                {
                    proj.flags.push_back(flag);
                }
            }
        }
    }
}

Project LoadProjectFromConfig(const char *filePath, const char *projName)
{
    std::string filepath = filePath;

    std::ifstream file(filepath, std::ios::in);
    if(!file.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "The File ", filepath, " was not found.\n");
        LLOG("\tYou must create the file: \'", filepath, "\'\n");
    }

    std::vector<Project> projects = GetProjects(filepath.c_str());

    if(projects.size() == 0)
    {
        LERROR("No Projects found in ", "YMake.toml", ".\n");
        LERROR("\tmust specify at least one \'# PROJECT: project_name\' comment in the file.\n");
        throw Y::Error("couldn't find any project in a specified file.\n");
    }

    auto config = toml::parse_file(filepath);

    bool found = false;
    for(Project &proj : projects)
    {
        try
        {
            ParseProjectData(config, proj);
            if(proj.name == projName)
            {
                found = true;
                return proj;
            }
        }
        catch(toml::parse_error &err)
        {
            LLOG(RED_TEXT("[TOML ERROR]: "), err.what(), "\n");
            exit(1);
        }
        catch(Y::Error err)
        {
            LLOG(RED_TEXT("[TOML ERROR]: "), err.what(), "\n");
            exit(1);
        }
        // NOTE: or load project cache.

        // TODO: fix some things:
        // add things like RES_PATH from CMake (Compile Definitions)
        // need to add build.mode (release, debug, etc...)
        //      but the defines need to be specific to the build mode
        // need to see if I can compile to other platforms. (x86, ARM, linux, etc..)
    }

    if(!found)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't find project with specified name: ", projName, "\n");
        throw Y::Error("couldn't find a specific project.");
    }

    return Project{};
}

std::vector<Project> LoadProjectsFromConfig(const char *filePath)
{
    std::string filepath = filePath;

    if(!fs::exists(filepath))
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "the File ", filepath, " was not found.\n");
        LLOG("\tyou could create the file: \'", filepath, "\' or choose another path.\n");
        exit(1);
    }

    std::ifstream file(filepath, std::ios::in);
    if(!file.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "the file ", filepath, " was not found.\n");
        LLOG("\tYou must create the file: \'", filepath, "\'\n");
    }

    std::vector<Project> projects = GetProjects(filepath.c_str());

    if(projects.size() == 0)
    {
        LERROR("No Projects found in ", filepath, ".\n");
        LERROR("\tmust specify at least one \'# PROJECT: project_name\' comment in the file.\n");
        throw Y::Error("couldn't find any project in a specified file.\n");
    }

    auto config = toml::parse_file(filepath);

    for(Project &proj : projects)
    {
        try
        {
            ParseProjectData(config, proj);
        }
        catch(toml::parse_error &err)
        {
            LLOG(RED_TEXT("[TOML PARSE ERROR]: "), err.what(), "\n");
            exit(1);
        }
        catch(Y::Error err)
        {
            LLOG(RED_TEXT("[YMAKE TOML PARSE ERROR]: "), err.what(), "\n");
            exit(1);
        }
    }

    return projects;
}

} // namespace Y::Parse