#include "cmd.h"

namespace Y {

CommandInfo ParseCLI(std::vector<std::string> arguments, std::vector<Command> commands)
{
    std::vector<std::string> args = arguments;
    args.erase(std::find(args.begin(), args.end(), args[0]));
    Command calledCmd;

    // first => check the commands and choose the correct one
    for(Command cmd : commands)
    {
        if(args[0] == cmd.name)
        {
            // if command is found: remove it from the vector.
            calledCmd = cmd;
            args.erase(std::find(args.begin(), args.end(), args[0]));
            break;
        }
    }

    std::map<std::string, std::string> foundArgs;
    std::vector<std::string> cmdIn;

    // FIXME: check if a command has value or not while parsing, not after.

    for(usize i = 0; i < args.size(); i++)
    {
        // argument with dash -> argument.
        if(args[i][0] == '-')
        {
            // followed by another argument or is the last argument (no value)
            if(i == args.size() - 1 || args[i + 1][0] == '-')
            {
                foundArgs[args[i]] = "NULL";
            }
            // followed by a value
            else
            {
                foundArgs[args[i]] = args[i + 1];
                i++; // skip the value in the next iteration
            }
        }
        // argument without dash -> stdin
        else
        {
            cmdIn.push_back(args[i]);
        }
    }

    // throw error if a found arg is not specified.
    std::map<std::string, std::string> foundAvailableArgs;
    for(auto &[k, v] : foundArgs)
    {
        bool found = false;
        for(CommandArgument &arg : calledCmd.args)
        {
            if(k == arg.shortOpt || k == arg.longOpt)
            {
                found = true;

                foundAvailableArgs[arg.name] = v;
            }
            else
            {
                if(k == "NULL" && (arg.valType == ValueType::BOOL || arg.valType == ValueType::NONE))
                {
                    found = true;

                    foundAvailableArgs[arg.name] = "NULL";
                }
                else
                {
                    LLOG(RED_TEXT("[YMAKE ERROR]: "), "command \'", calledCmd.name, "\' is incorrect.\n");
                    calledCmd.OutputCommandInfo();
                    throw Y::Error("incorrect arguments for a command.");
                }
            }
        }

        if(!found)
        {
            LLOG(RED_TEXT("[YMAKE ERROR]: "), "provided unknown argument: ", k, " for command: ", calledCmd.name, "\n");
            calledCmd.OutputCommandInfo();
            throw Y::Error("user passed an unkown/invalid argument");
        }
    }

    LTRACE(true, BLUE_TEXT("COMMAND INPUT: \n"));
    for(auto entry : cmdIn)
    {
        LTRACE(true, "\t", entry, "\n");
    }

    LTRACE(true, BLUE_TEXT("COMMAND ARGS: \n"));
    for(auto &[k, v] : foundAvailableArgs)
    {
        LTRACE(true, "\tkey: ", k, "\tval: ", v, "\n");
    }

    CommandInfo info = {
        .cmd       = calledCmd,
        .cmdIn     = cmdIn,
        .arguments = foundAvailableArgs,
    };

    return info;
}

// command specific.
void OutputYMakeInfo()
{
    LLOG(BLUE_TEXT("YMake v", YMAKE_VERSION_MAJOR, ".", YMAKE_VERSION_MINOR, ".", YMAKE_VERSION_PATCH, "\n"));
    LLOG("A simple build tool for C/C++\n");
    LLOG("To get started, run \'ymake help\' for usage info, or run \'ymake default\' to generate a default "
         "configuration.\n");
}

void OutputHelpInfo(std::vector<Command> commands)
{
    LLOG(BLUE_TEXT("YMake v", YMAKE_VERSION_MAJOR, ".", YMAKE_VERSION_MINOR, ".", YMAKE_VERSION_PATCH, "\n"));

    LLOG("Create a YMake.toml using \'ymake default\' to get started.\n");
    LLOG("\tCheck docs at https://github.com/Yassine914/YMake for more information.\n\n");

    LLOG(GREEN_TEXT("USAGE: \n"));

    for(Command cmd : commands)
    {
        LLOG("ymake ", CYAN_TEXT(cmd.name), "\t", cmd.desc, "\n");

        if(cmd.args.size() > 0)
            LLOG(PURPLE_TEXT("\targuments: \n"));
        for(CommandArgument arg : cmd.args)
        {
            LLOG("\t", CYAN_TEXT(arg.name), ": ", arg.shortOpt, ", ", arg.longOpt, "\t", arg.desc, "\n");
        }

        LLOG("\n");
    }
}

void CreateDefaultConfig(std::vector<std::string> &input, std::map<std::string, std::string> &args)
{
    std::string path = (args.size() > 0) ? args["config"] : YMAKE_DEFAULT_FILE;

    LLOG(BLUE_TEXT("[YMAKE]: "), "creating default YMake.toml in ", path, "\n");
    LLOG(YELLOW_TEXT("[NOTE]: "), "this will override any current YMake.toml in dir: ", path, "\n");
    LLOG("are you sure you want to continue? [Y/n] ");

    char x;
    std::cin >> x;
    if(x != 'y' && x != 'Y')
    {
        LLOG("Exiting...\n");
        exit(1);
    }

    std::ofstream file(path, std::ios::out);

    std::string contents =
        "# PROJECT: Default\n"
        "\n"
        "[Default]\n"
        "\n"
        "#could be an array for multiple language support"
        "lang = \"C++\"\n"
        "\n"
        "cpp.std = 14\n"
        "cpp.compiler = \"clang++\"\n"
        "\n"
        "build.type = \"executable\"\n"
        "build.dir = \"./build\"\n"
        "\n"
        "src = \"./src\"\n"
        "\n"
        "includes = [\n"
        "    \"./src\"\n"
        "]\n"
        "\n"
        "# replace with your libraries\n"
        "libs = [\n"
        "    {name = \"GLFW\", path = \"./thirdparty/GLFW/\", include = \"./thirdparty/GLFW/include\"}\n"
        "]\n"
        "\n"
        "[Default.compiler]\n"
        "defines.debug = [\n"
        "    \"DDEBUG\",\n"
        "    \"DTEST=1\"\n"
        "]\n\n"
        "defines.release = [\n"
        "    \"DNDEBUG\",\n"
        "]\n"
        "\n"
        "flags = [\n"
        "    \"-Wall\",\n"
        "    \"-Wargs\",\n"
        "    \"-g\"\n"
        "]";

    file << contents;
    file.close();

    LLOG(BLUE_TEXT("[YMAKE]: "), "successfully created file at: ", path, "\n");
}

void OutputProjectsInfo(std::vector<std::string> &input, std::map<std::string, std::string> &args)
{
    (void)input;

    std::string path = (args.size() > 0) ? args["config"] : YMAKE_DEFAULT_FILE;

    std::vector<Project> projects = Cache::SafeLoadProjectsFromCache(path.c_str());

    for(Project proj : projects)
    {
        proj.OutputInfo();
        LLOG("-----------------------------------\n");
    }

    exit(0);
}

void SetupProjectInfo(std::vector<std::string> &input, std::map<std::string, std::string> &args)
{
    (void)input;

    std::string path = (args.size() > 0) ? args["config"] : YMAKE_DEFAULT_FILE;

    LTRACE(true, "begin setup. config path: ", path, "\n");

    // always reload the cache when setting up
    std::vector<Project> projects = Parse::LoadProjectsFromConfig(path.c_str());
    Cache::CreateProjectsCache(projects, path.c_str());

    if(projects.size() == 0)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "no projects were found in the config file: ", path, "\n");
        LLOG("\tto specify a project in a YMake.toml: add a comment like:\n");
        LLOG(CYAN_TEXT("\t\t# PROJECT: ProjectName\n"));
        exit(1);
    }

    LLOG(BLUE_TEXT("[YMAKE]: "), "projects loaded succesfully.\n");

    LLOG(CYAN_TEXT("\tLoaded Projects: \n"));
    for(auto proj : projects)
    {
        LLOG("\t\t", proj.name, "\n");
    }

    exit(0);
}

void BuildProjects(std::vector<std::string> &input, std::map<std::string, std::string> &args)
{
    Build::BuildMode mode = Build::BuildMode::DEBUG;
    std::string path;
    if(args.count("config") > 0)
        path = args["config"];
    else
        path = YMAKE_DEFAULT_FILE;

    if(args.count("build mode") > 0)
    {
        if(args["build mode"] == "debug")
            mode = Build::BuildMode::DEBUG;
        else if(args["build mode"] == "release")
            mode = Build::BuildMode::RELEASE;
        else
        {
            LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "build mode \'", args["build mode"], "\' unsopported.\n");
            LLOG("\tchoosing build mode = debug by default.\n");
            mode = Build::BuildMode::DEBUG;
        }
    }

    bool cleanBuild = (args.count("clean build") > 0);

    std::vector<Project> allProjects = Cache::SafeLoadProjectsFromCache(path.c_str());

    std::vector<Project> projectsToBuild;

    for(std::string &projName : input)
    {
        // look if the project's metadata exist

        bool found = false;
        for(Project proj : allProjects)
        {
            if(projName == proj.name)
            {
                projectsToBuild.push_back(proj);
                found = true;
            }
        }

        if(!found)
        {
            LLOG(YELLOW_TEXT("[YMAKE WARN]: "), "ignoring unspecified project: ", projName, "\n");
            LLOG("\tplease specify the project's config in the \'", path, "\' file.\n");
        }
    }

    if(projectsToBuild.size() == 0)
    {
        LLOG(GREEN_TEXT("[YMAKE]: "), "building all projects...\n", BLUE_TEXT("\tProjects to build: "));

        for(Project proj : allProjects)
        {
            proj.OutputInfo();
            LLOG("-----------------------------------\n");
        }

        for(Project proj : allProjects)
        {
            try
            {
                Build::BuildProject(proj, mode, cleanBuild);
            }
            catch(Y::Error &err)
            {
                LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't build project:", proj.name, "\n\t", err.what(), "\n");
                exit(1);
            }
        }
    }
    else
    {
        LLOG(GREEN_TEXT("[YMAKE]: "), "building projects...\n", BLUE_TEXT("\tProjects to build: "));

        for(Project proj : projectsToBuild)
        {
            proj.OutputInfo();
            LLOG("-----------------------------------\n");
        }

        for(Project proj : projectsToBuild)
        {
            try
            {
                Build::BuildProject(proj, mode, cleanBuild);
            }
            catch(Y::Error &err)
            {
                LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't build project:", proj.name, "\n\t", err.what(), "\n");
                exit(1);
            }
        }
    }

    return;
}

void CleanAllCache(std::vector<std::string> &input, std::map<std::string, std::string> &args)
{

    if(Cache::RemoveAllCache())
    {
        LLOG(GREEN_TEXT("[YMAKE]: "), "removed all cache successfully.\n");
    }
    else
    {
        LLOG(RED_TEXT("[YMAKE ERROR]:"), "failed to remove all cache.\n");
        LLOG("\tif errors persist, try removing the directory: \'", YMAKE_CACHE_DIR, "\' manually.\n");
    }

    return;
}

} // namespace Y