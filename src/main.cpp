#include "toml/parser.h"
#include "cmd/cmd.h"

// TODO: fix some things:
// need to see if I can compile to other platforms. (x86, ARM, linux, etc..)

int main(int argc, char **argv)
{
    LOG_CHANGE_PRIORITY(LOG_NONE);
    std::vector<std::string> args(argv, argv + argc);

    // clang-format off

    // NOTE: to look for a value in the commandArgument map remember: the key is the name of the argument.
    std::vector<Y::Command> commands = {
        Y::Command("help", "view help information"),

        Y::Command("default", "[args...]\tcreate the default YMake.toml config file to get started (overrides existing file in path)", { 
            Y::CommandArgument("config", "/path/to/YMake.toml (new file)", "-c", "--config-file")
        }, Y::CreateDefaultConfig),

        Y::Command("new", "[args...]\tcreate a new project with default directory structure, and YMake.toml file (ovverrides existing dir with same name as project name)", {
            Y::CommandArgument("project name", "name of the project", "-n", "--name"),
            Y::CommandArgument("project path", "path to create the project", "-p", "--path"),
            Y::CommandArgument("project type", "type of project to create [executable, static, shared]", "-t", "--type"),
            Y::CommandArgument("project lang", "language of the project [c, cpp]", "-l", "--lang"),
        }, Y::CreateNewProject),

        Y::Command("list", "lists all the projects specified in the toml file (generates new cache as well)", {
            Y::CommandArgument("config", "/path/to/YMake.toml (existing file)", "-c", "--config-file"),
        }, Y::OutputProjectsInfo),

        Y::Command("setup", "[args...]\tloads the YMake.toml and caches needed information", {
            Y::CommandArgument("config", "/path/to/YMake.toml (existing file)", "-c", "--config-file")
        }, Y::SetupProjectInfo),

        Y::Command("build", "<projects> [args...]\tbuild project(s) specified. (must be in the toml file) [if no project(s) are specified, builds all projects]", {
            Y::CommandArgument("config", "/path/to/YMake.toml", "-c", "--config-file"),
            Y::CommandArgument("build mode", "build the project in [release or debug] mode", "-b", "--build-mode"),
            Y::CommandArgument("clean build", "rebuild the project entirely (including libraries)", "-C", "--clean", Y::ValueType::BOOL),
        }, Y::BuildProjects),

        Y::Command("clean", "[args...]\tclean all the YMake-generated cache", {
            Y::CommandArgument("config", "/path/to/YMake.toml", "-c", "--config-file"),
        }, Y::CleanAllCache),
    };

    // clang-format on

    if(args.size() == 1 || (args.size() == 2 && (args[1] == "-?" || args[1] == "--version")))
    {
        Y::OutputYMakeInfo();
        exit(0);
    }

    if(args.size() == 2)
    {
        if(args[1] == "-h" || args[1] == "--help" || args[1] == "help" || args[1] == "h")
        {
            Y::OutputHelpInfo(commands);
            exit(0);
        }
    }

    Y::CommandInfo cmdInfo;
    try
    {
        cmdInfo = Y::ParseCLI(args, commands);
    }
    catch(Y::Error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couldn't parse the command.\n\t", err.what(), "\n");
        exit(1);
    }

    try
    {
        cmdInfo.CallFunction();
    }
    catch(Y::Error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couln't call the command: ", cmdInfo.cmd.name, "\n\t", err.what(), "\n");
        exit(1);
    }

    return 0;
}