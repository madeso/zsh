#include "computer/computer.h"

#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <chrono>
#include <sstream>

void handle_add(computer::Computer* computer, const std::vector<std::string>& args)
{
    if (args.empty())
    {
        computer->console.print_error("add: missing argument to add");
    }
    else
    {
        computer->zsh.add(args[0], computer::get_unix_timestamp());
    }
}

void handle_single(computer::Computer* computer, const std::vector<std::string>& args)
{
    if (args.empty())
    {
        computer->console.print_error("single: missing argument");
    }
    else
    {
        auto found = computer->zsh.get_single(args, computer::get_unix_timestamp(), computer->zsh_algorithm);

        if (found.has_value())
        {
            std::cout << *found;
        }
        else
        {
            computer->console.print_error("single: no matches found");
        }
    }
}


int main(int, char **)
{
    computer::Computer c;

    c.console.cwd = "";

    computer::add_default_commands(&c);
    c.add("add", handle_add);
    c.add("single", handle_single);

    return computer::run(&c);
}
