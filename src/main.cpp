#include "toml/parser.h"
#include "cmd/cmd.h"

// TODO: fix some things:
// need to see if I can compile to other platforms. (x86, ARM, linux, etc..)

// need to compile and run successfuly using a compiler other than clang...

int main(int argc, char **argv)
{
    // LOG_CHANGE_PRIORITY(LOG_NONE);
    std::vector<std::string> args(argv, argv + argc);

    // clang-format off

    // NOTE: to look for a value in the commandArgument map remember: the key is the name of the argument.
    std::vector<Y::Command> commands = {
        Y::Command("help", "view help information"),

        Y::Command("default", "[args...]\tcreate the default YMake.toml config file to get started (overrides existing file in path)", { 
            Y::CommandArgument("config", "/path/to/YMake.toml (new file)", "-c", "--config-file")
        }, Y::CreateDefaultConfig),

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
        LLOG(YELLOW_TEXT("[YMAKE]: "), "calling the function for command: ", cmdInfo.cmd.name, "\n");
        cmdInfo.CallFunction();
        LLOG(YELLOW_TEXT("[YMAKE]: "), "called the function for command: ", cmdInfo.cmd.name, "\n");
    }
    catch(Y::Error &err)
    {
        LLOG(RED_TEXT("[YMAKE ERROR]: "), "couln't call the command: ", cmdInfo.cmd.name, "\n\t", err.what(), "\n");
        exit(1);
    }

    return 0;
}