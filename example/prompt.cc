#include "zsh/zsh.h"

#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <chrono>
#include <sstream>

std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.emplace_back(item);
    }
    return elems;
}

zsh::i64 get_unix_timestamp()
{
    const auto time = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(time).count();
}

struct Console
{
    std::string cwd;

    Console()
      : cwd("~")
    {
    }

    void print_prompt() const
    {
        std::cout << cwd << "> ";
    }

    void print_error(const std::string& msg) const
    {
        std::cerr << msg << std::endl;
    }
};

struct Computer;

struct Computer
{
    bool on = true;
    Console console;
    zsh::Zsh zsh;

    zsh::SortAlgorithm zsh_algorithm = zsh::SortAlgorithm::Frecent;

    void jump_to(const std::string& path)
    {
        console.cwd = path;
        zsh.add(console.cwd, get_unix_timestamp());
    }

    void cd_to_relative(const std::string& path)
    {
        const auto parts = split(path, '/');
        for (const auto& part : parts)
        {
            if (part == "..")
            {
                if (console.cwd == "~")
                {
                    console.print_error("Cannot go up from root folder");
                    return;
                }
                console.cwd = console.cwd.substr(0, console.cwd.rfind('/'));
            }
            else if (part == ".")
            {
                continue;
            }
            else
            {
                console.cwd += "/" + part;
            }
        }\

        zsh.add(console.cwd, get_unix_timestamp());
    }

    void cd_to(const std::string& path)
    {
        if (path.find('/') == 0)
        {
            cd_to_relative(path);
        }
        else
        {
            jump_to(path);
        }
    }
    
    std::map<std::string, std::function<void(Computer*, const std::vector<std::string>&)>> commands;
};

void handle_exit(Computer* computer, const std::vector<std::string>&)
{
    computer->on = false;
}

void handle_cd(Computer* computer, const std::vector<std::string>& args)
{
    if (args.empty())
    {
        computer->console.cwd = "~";
    }
    else
    {
        const auto folder = args[0];
        computer->cd_to(folder);
    }
}

void handle_zsh(Computer* computer, const std::vector<std::string>& args)
{
    if (args.empty())
    {
        computer->console.print_error("zsh: missing argument");
    }
    else
    {
        auto folder = computer->zsh.get_single(args, get_unix_timestamp(), computer->zsh_algorithm);

        if (folder.has_value())
        {
            computer->cd_to(*folder);
        }
        else
        {
            computer->console.print_error("zsh: no matches found");
        }
    }
}

void handle_zsh_list(Computer* computer, const std::vector<std::string>& args)
{
    if (args.empty())
    {
        computer->console.print_error("zsh: missing argument");
    }
    else
    {
        const auto folders = computer->zsh.get_all(args, get_unix_timestamp(), computer->zsh_algorithm);
        for (const auto& folder : folders)
        {
            std::cout << folder.path << ": " << folder.rank << std::endl;
        }
    }
}

std::vector<std::string> drop_first(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        return {};
    }
    return std::vector(std::next(args.begin()), args.end());
}


int main(int, char **)
{
    Computer computer;

    computer.commands["exit"] = handle_exit;
    computer.commands["cd"] = handle_cd;
    computer.commands["z"] = handle_zsh;
    computer.commands["zsh"] = handle_zsh_list;

    while(true)
    {
        std::string line;

        computer.console.print_prompt();
        if(!std::getline(std::cin, line))
            break;

        const auto command = split(line, ' ');
        if(command.empty())
            continue;

        const auto& command_name = command[0];
        const auto command_args = drop_first(command);

        if (const auto handler = computer.commands.find(command_name); handler != computer.commands.end())
        {
            handler->second(&computer, command_args);
        }
        else
        {
            computer.console.print_error("Unknown command: " + command_name);
        }
    }

    return 0;
}
