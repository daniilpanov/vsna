#include "menu.h"
#include "command_manager.h"

bool HelpCommand::handle(CONST_ARG_VECTOR args)
{
	auto commands = _manager.listCommands();
	std::cout << "[=] Available commands:" << std::endl;
	for (const auto& cmd : commands)
	{
		std::cout <<
		"\t" << cmd.name <<
		" " << cmd.usage <<
		" - " << cmd.description << std::endl;
	}
	return false;
}
