#pragma once

#include "zsh/zsh.h"

#include <vector>
#include <map>
#include <functional>

namespace computer
{
    zsh::i64 get_unix_timestamp();

    struct Console
    {
        std::string cwd;

        Console();

        void print_prompt() const;

        void print_error(const std::string& msg) const;

        void jump_to(const std::string& path);
        void cd_to_relative(const std::string& path);

        void cd_to(const std::string& path);
    };

    struct Computer;

    using CommandHandler = std::function<void(Computer*, const std::vector<std::string>&)>;

    struct Computer
    {
        bool on = true;
        zsh::sort_algorithm zsh_algorithm = zsh::sort_algorithm::frecent;

        Console console;
        zsh::zsh zsh;
        std::map<std::string, CommandHandler> commands;

        void cd_to(const std::string& path);
        void add(const std::string& name, const CommandHandler& command);
    };

    void add_default_commands(Computer* computer);
    int run(Computer* computer);

}