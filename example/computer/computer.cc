#include "computer/computer.h"

#include <iostream>
#include <chrono>
#include <sstream>

namespace computer
{
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

    Console::Console()
        : cwd("~")
    {
    }

    void Console::print_prompt() const
    {
        std::cout << cwd << "> ";
    }

    void Console::print_error(const std::string& msg) const
    {
        std::cerr << "ERROR: " << msg << "\n" << std::endl;
    }

    void Console::jump_to(const std::string& path)
    {
        cwd = path;
    }

    void Console::cd_to_relative(const std::string& path)
    {
        const auto parts = split(path, '/');
        for (const auto& part : parts)
        {
            if (part == "..")
            {
                if (cwd == "~")
                {
                    print_error("Cannot go up from root folder");
                    return;
                }
                cwd = cwd.substr(0, cwd.rfind('/'));
            }
            else if (part == ".")
            {
                continue;
            }
            else
            {
                cwd += "/" + part;
            }
        }
    }

    void Console::cd_to(const std::string& path)
    {
        if (path.find('~') == 0)
        {
            jump_to(path);
        }
        else
        {
            cd_to_relative(path);
        }
    }

    struct Computer;

    void Computer::cd_to(const std::string& path)
    {
        console.cd_to(path);
        zsh.add(console.cwd, get_unix_timestamp());
    }

    void Computer::add(const std::string& name, const CommandHandler& command)
    {
	    commands[name] = command;
    }
    
    void handle_exit(Computer* computer, const std::vector<std::string>&)
    {
        computer->on = false;
    }

    void handle_zsh_list(Computer* computer, const std::vector<std::string>& args)
    {
        const auto folders = computer->zsh.get_all(args, get_unix_timestamp(), computer->zsh_algorithm);
        for (const auto& folder : folders)
        {
            std::cout << folder.path << ": " << folder.rank << "\n";
        }
        std::cout << std::endl;
    }

    void handle_dump(Computer* computer, const std::vector<std::string>& args)
    {
        if (args.empty() == false)
        {
            computer->console.print_error("dump takes 0 arguments");
        }
        else
        {
            for(const auto& e: computer->zsh.entries)
            {
                std::cout << e.first << ": " << e.second.time << " / " << e.second.rank << "\n";
            }
            std::cout << std::endl;
        }
    }

    void handle_help(Computer* computer, const std::vector<std::string>& args)
    {
        if (args.empty() == false)
        {
            computer->console.print_error("help takes 0 arguments");
        }

        std::cout << "Available commands:\n";
        for (const auto& command : computer->commands)
        {
            std::cout << "  " << command.first << "\n";
        }
        std::cout << std::endl;
    }

    std::vector<std::string> drop_first(const std::vector<std::string>& args)
    {
        if (args.empty())
        {
            return {};
        }
        return std::vector(std::next(args.begin()), args.end());
    }


    void add_default_commands(Computer* computer)
    {
        computer->add("exit", handle_exit);
        computer->add("all", handle_zsh_list);
        computer->add("dump", handle_dump);
        computer->add("help", handle_help);
    }


    int run(Computer* computer)
    {
        while(computer->on)
        {
            std::string line;

            computer->console.print_prompt();
            if(!std::getline(std::cin, line))
                break;

            const auto command = split(line, ' ');
            if(command.empty())
                continue;

            const auto& command_name = command[0];
            const auto command_args = drop_first(command);

            if (const auto handler = computer->commands.find(command_name); handler != computer->commands.end())
            {
                handler->second(computer, command_args);
            }
            else
            {
                computer->console.print_error("Unknown command: " + command_name);
            }
        }

        return 0;
    }
}
