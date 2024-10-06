#include "parser.h"

#include <filesystem>
namespace fs = std::filesystem;

namespace Y::Parse {

std::map<std::string, std::string> ParseDotEnv(const std::string &filename)
{
    std::map<std::string, std::string> envMap;
    std::ifstream file(filename);
    std::string line;

    if(!file.is_open())
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't load the .env file in path: ", filename, "\n");
        return envMap;
    }

    while(std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key, value;

        if(std::getline(iss, key, '=') && std::getline(iss, value))
        {
            envMap[key] = value;
        }
    }

    file.close();
    return envMap;
}

// Function to get value by key
std::string GetEnvValue(const std::map<std::string, std::string> &envMap, const std::string &key)
{
    auto it = envMap.find(key);
    if(it != envMap.end())
    {
        std::string value = it->second;
        if(!value.empty() && value.front() == '"' && value.back() == '"')
        {
            value = value.substr(1, value.size() - 2);
        }
        return value;
    }
    return "";
}

std::string ExpandMacros(const std::string &input, const std::map<std::string, std::string> &dotenv)
{
    std::string result;
    size_t pos = 0;
    while(pos < input.size())
    {
        if(input[pos] == '$' && pos + 1 < input.size() && input[pos + 1] == '(')
        {
            size_t endPos = input.find(')', pos + 2);
            if(endPos != std::string::npos)
            {
                std::string macro = input.substr(pos + 2, endPos - pos - 2);
                std::string value = GetEnvValue(dotenv, macro);
                result += value;
                pos = endPos + 1;
            }
            else
            {
                result += input[pos];
                pos++;
            }
        }
        else
        {
            result += input[pos];
            pos++;
        }
    }
    return result;
}

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

// helper functions
BuildType ToBuildType(const std::string &str)
{
    if(str == "executable" || str == "exe")
        return BuildType::EXECUTABLE;
    if(str == "static" || str == "static" || str == "st")
        return BuildType::STATIC_LIB;
    if(str == "shared" || str == "dy" || str == "dynamic")
        return BuildType::SHARED_LIB;

    throw Y::Error("specified an unknown build type");
}

Lang ToLang(const std::string &str)
{
    if(str == "C++" || str == "CPP" || str == "CC" || str == "CXX")
        return Lang::CPP;
    if(str == "C")
        return Lang::C;

    throw Y::Error("specified an unknown (or unsupported) language.");
}

void ParseProjectData(const toml::table &config, Project &proj)
{
    if(!config.contains(proj.name.c_str()))
    {
        LLOG(RED_TEXT("[YMAKE TOML ERROR]: "), "couldn't find project table: ", proj.name, " in the config file\n");
        throw Y::Error("couldn't find project in the config file.");
    }

    auto mainTable = *config[proj.name].as_table();

    if(auto ver = mainTable[YMAKE_TOML_PROJECT_VERSION].value<std::string>())
    {
        proj.version = ver.value();
    }
    else
    {
        LLOG(YELLOW_TEXT("[YMAKE TOML WARN]: "), "didn't specify the version for project: ", proj.name, "\n");
        LLOG(PURPLE_TEXT("\tusing version: 0.0.1 by default.\n"));
        proj.version = "0.0.1";
    }

    // env
    std::map<std::string, std::string> dotenv;
    if(auto env = mainTable[YMAKE_TOML_ENV].value<std::string>())
    {
        proj.env = env.value();
        dotenv   = ParseDotEnv(proj.env);
    }

    // NOTE: YMake MACROS
    dotenv[YMAKE_MACRO_PROJECT_NAME] = proj.name;
    dotenv[YMAKE_MACRO_CURRENT_DIR]  = fs::current_path().string();
    dotenv[YMAKE_MACRO_BUILD_DIR]    = proj.buildDir;
    dotenv[YMAKE_MACRO_SRC_DIR]      = proj.src;

    if(!mainTable.contains(YMAKE_TOML_LANG))
    {
        LLOG(RED_TEXT("[YMAKE TOML ERROR]: "), "couldn't find the key: ", YMAKE_TOML_LANG, " in proj: ", proj.name,
             "\n");
        throw Y::Error("couldn't find lang key for a project in the config file.");
    }

    // langs.
    auto langs = mainTable[YMAKE_TOML_LANG];
    if(langs.is_array())
    {
        for(const auto &lang : *langs.as_array())
            proj.langs.push_back(ToLang(lang.value<std::string>().value()));
    }
    else if(langs.is_string())
    {
        proj.langs.push_back(ToLang(langs.value<std::string>().value()));
    }

    for(const auto &lang : proj.langs)
    {
        if(lang == Lang::CPP)
        {
            if(auto cppStd = mainTable[YMAKE_TOML_CPP][YMAKE_TOML_STD].value<i32>())
            {
                proj.cppStd = cppStd.value();
            }
            else
            {
                LLOG(YELLOW_TEXT("[YMAKE TOML WARN]: "), "the C++ standard (cpp.std) has not been set.\n");
                LLOG(PURPLE_TEXT("\tusing C++14 by default.\n"));
                proj.cppStd = 14;
            }

            if(auto cppComp = mainTable[YMAKE_TOML_CPP][YMAKE_TOML_COMPILER].value<std::string>())
            {
                proj.cppCompiler = cppComp.value();
            }
            else
            {
                LLOG(RED_TEXT("[YMAKE TOML ERROR]: "),
                     "didn't specify the C++ compiler to use (cpp.compiler) for project: ", proj.name, "\n");
                throw Y::Error("user didn't specify the C++ compiler in the config file.");
            }
        }
        else
        {
            if(auto cStd = mainTable[YMAKE_TOML_C][YMAKE_TOML_STD].value<i32>())
            {
                proj.cStd = cStd.value();
            }
            else
            {
                LLOG(YELLOW_TEXT("[YMAKE TOML WARN]: "), "the C standard (c.std) has not been set.\n");
                LLOG(PURPLE_TEXT("\tusing c11 by default.\n"));
                proj.cStd = 11;
            }

            if(auto cComp = mainTable[YMAKE_TOML_C][YMAKE_TOML_COMPILER].value<std::string>())
            {
                proj.cCompiler = cComp.value();
            }
            else
            {
                LLOG(RED_TEXT("[YMAKE TOML ERROR]: "),
                     "didn't specify the C compiler to use (c.compiler) for project: ", proj.name, "\n");
                throw Y::Error("user didn't specify the C compiler in the config file.");
            }
        }
    }

    // build
    if(auto buildType = mainTable[YMAKE_TOML_BUILD][YMAKE_TOML_TYPE].value<std::string>())
    {
        try
        {
            proj.buildType = ToBuildType(buildType.value());
        }
        catch(Y::Error &err)
        {
            LLOG(RED_TEXT("[YMAKE TOML ERROR]: "), "build type specified for project \'", proj.name,
                 "\' is not supported.\n");
            LLOG(PURPLE_TEXT("\tavailable types: "), "executable, shared, static");
            LLOG(PURPLE_TEXT("\tusing executable by default\n"));
            proj.buildType = BuildType::EXECUTABLE;
        }
    }
    else
    {
        LLOG(YELLOW_TEXT("[YMAKE TOML WARN]: "), "didn't specify the build type (build.type) for project \'", proj.name,
             "\'\n");
        LLOG(PURPLE_TEXT("\tusing executable by default."));
        proj.buildType = BuildType::EXECUTABLE;
    }

    if(auto buildDir = mainTable[YMAKE_TOML_BUILD][YMAKE_TOML_DIR].value<std::string>())
    {
        proj.buildDir = ExpandMacros(buildDir.value(), dotenv);
    }
    else
    {
        LLOG(YELLOW_TEXT("[YMAKE TOML WARN]: "), "didn't specify the build dir (build.dir) for project \'", proj.name,
             "\'\n");
        LLOG(PURPLE_TEXT("\tusing build directory: ", "\"./build/\" by default"));
        proj.buildDir = "./build";
    }

    // src
    if(auto src = mainTable[YMAKE_TOML_SRC].value<std::string>())
    {
        proj.src = ExpandMacros(src.value(), dotenv);
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE TOML ERROR]: "), "didn't specify the source directory for project \'", proj.name, "\'\n");
        throw Y::Error("didn't specify the src for a project in the config file.");
    }

    // includes
    if(auto includes = mainTable[YMAKE_TOML_INCLUDES].as_array())
    {
        for(const auto &incl : *includes)
        {
            std::string include = incl.value<std::string>().value();
            include             = ExpandMacros(include, dotenv);
            proj.includeDirs.push_back(include);
        }
    }
    else
    {
        LLOG(YELLOW_TEXT("[YMAKE TOML WARN]: "), "didn't specify any includes for project \'", proj.name, "\'\n");
        LLOG(PURPLE_TEXT("\tadding include path \"", proj.src, "\" by default.\n"));
        proj.includeDirs.push_back(ExpandMacros(proj.src, dotenv));
    }

    // libs.src
    if(auto libsSrc = mainTable[YMAKE_TOML_LIBS][YMAKE_TOML_SRC].as_array())
    {
        for(auto const &libTable : *libsSrc)
        {
            auto lib = *libTable.as_table();

            Library library;
            if(auto libName = lib[YMAKE_TOML_LIB_NAME].value<std::string>())
            {
                library.name = libName.value();
            }
            else
            {
                LLOG(RED_TEXT("[YMAKE ERROR]: "), "didn't specify a name for a library for project: ", proj.name, "\n");
                throw Y::Error("problem parsing the libraries for a project");
            }

            if(auto libPath = lib[YMAKE_TOML_LIB_PATH].value<std::string>())
            {
                library.path = ExpandMacros(libPath.value(), dotenv);
            }
            else
            {
                LLOG(RED_TEXT("[YMAKE TOML ERROR]: "), "didn't specify a path for library: ", library.name,
                     " for project: ", proj.name, "\n");
                throw Y::Error("didn't specify a path for a library");
            }

            if(auto libInclPath = lib[YMAKE_TOML_LIB_INCLUDE].value<std::string>())
            {
                bool found = false;
                for(std::string dir : proj.includeDirs)
                {
                    if(libInclPath.value() == dir)
                    {
                        found = true;
                        break;
                    }
                }

                if(!found)
                    proj.includeDirs.push_back(ExpandMacros(libInclPath.value(), dotenv));
            }
            else
            {
                LLOG(RED_TEXT("[YMAKE TOML ERROR]: "), "didn't specify an include path for library: ", library.name,
                     " for project: ", proj.name, "\n");
                throw Y::Error("didn't specify an include path for a library.\n");
            }

            if(auto libType = lib[YMAKE_TOML_LIB_TYPE].value<std::string>())
            {
                library.type = ToBuildType(libType.value());
            }
            else
            {
                LLOG(YELLOW_TEXT("[YMAKE TOML WARN]: "), "didn't specify the build type for library: ", library.name,
                     " for project: ", proj.name, "\n");
                LLOG("\tusing build type \'static\' by default.\n");
                library.type = BuildType::STATIC_LIB;
            }

            proj.libs.push_back(library);
        }
    }

    // libs.built (preBuiltLibs)
    if(auto libsBuilt = mainTable[YMAKE_TOML_LIBS][YMAKE_TOML_BUILT].as_array())
    {
        for(const auto &lib : *libsBuilt)
        {
            std::string libPath = lib.value<std::string>().value();
            libPath             = ExpandMacros(libPath, dotenv);

            proj.preBuiltLibs.push_back(libPath);
        }
    }

    // libs.sys (sysLibs)
    if(auto libsSys = mainTable[YMAKE_TOML_LIBS][YMAKE_TOML_SYS].as_array())
    {
        for(const auto &lib : *libsSys)
        {
            std::string libPath = lib.value<std::string>().value();
            libPath             = ExpandMacros(libPath, dotenv);

            proj.sysLibs.push_back(libPath);
        }
    }

    // compiler.

    // debug optimization.
    if(auto opt = mainTable[YMAKE_TOML_COMPILER][YMAKE_TOML_DEBUG][YMAKE_TOML_OPTIMIZATION].value<i32>())
    {
        proj.optimizationDebug = opt.value();
    }
    else
    {
        proj.optimizationDebug = 0;
    }

    // release optimization.
    if(auto opt = mainTable[YMAKE_TOML_COMPILER][YMAKE_TOML_RELEASE][YMAKE_TOML_OPTIMIZATION].value<i32>())
    {
        proj.optimizationRelease = opt.value();
    }
    else
    {
        proj.optimizationRelease = 3;
    }

    // debug defines.
    if(auto defines = mainTable[YMAKE_TOML_COMPILER][YMAKE_TOML_DEBUG][YMAKE_TOML_DEFINES].as_array())
    {
        for(const auto &define : *defines)
        {
            std::string def = define.value<std::string>().value();
            def             = ExpandMacros(def, dotenv);

            proj.definesDebug.push_back(def);
        }
    }

    // release defines.
    if(auto defines = mainTable[YMAKE_TOML_COMPILER][YMAKE_TOML_RELEASE][YMAKE_TOML_DEFINES].as_array())
    {
        for(const auto &define : *defines)
        {
            std::string def = define.value<std::string>().value();
            def             = ExpandMacros(def, dotenv);

            proj.definesRelease.push_back(def);
        }
    }

    // debug flags
    if(auto flags = mainTable[YMAKE_TOML_COMPILER][YMAKE_TOML_DEBUG][YMAKE_TOML_FLAGS].as_array())
    {
        for(const auto &flag : *flags)
        {
            std::string fl = flag.value<std::string>().value();
            fl             = ExpandMacros(fl, dotenv);

            proj.flagsDebug.push_back(fl);
        }
    }

    // release flags
    if(auto flags = mainTable[YMAKE_TOML_COMPILER][YMAKE_TOML_RELEASE][YMAKE_TOML_FLAGS].as_array())
    {
        for(const auto &flag : *flags)
        {
            std::string fl = flag.value<std::string>().value();
            fl             = ExpandMacros(fl, dotenv);

            proj.flagsRelease.push_back(fl);
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