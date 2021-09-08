#include "computer/computer.h"

#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <chrono>
#include <sstream>

void handle_cd(computer::Computer* computer, const std::vector<std::string>& args)
{
    if (args.empty())
    {
        computer->cd_to("~");
    }
    else
    {
        computer->cd_to(args[0]);
    }
}

void handle_zsh(computer::Computer* computer, const std::vector<std::string>& args)
{
    if (args.empty())
    {
        computer->console.print_error("zsh: missing argument");
    }
    else
    {
        auto folder = computer->zsh.get_single(args, computer::get_unix_timestamp(), computer->zsh_algorithm);

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

int main(int, char **)
{
    computer::Computer c;

    computer::add_default_commands(&c);
    c.add("cd", handle_cd);
    c.add("z", handle_zsh);

    return computer::run(&c);
}
