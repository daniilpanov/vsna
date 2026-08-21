#include "menu.h"

bool HelpCommand::handle(CONST_ARG_VECTOR args)
{
	std::cout << "[=] Available commands:" << std::endl;
	for (const auto& [name, cmd] : _commands)
	{
		auto usage = cmd->usage;
		std::cout << "\t" << name;
		if (!usage.empty())
			std::cout << " " << usage;
		std::cout << " - " << cmd->desc << std::endl;
	}
	return false;
}