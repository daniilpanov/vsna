#include "cli_ui.h"

#include <iostream>
#include <string>
#include <vector>

int CliUI::run(int argc, char **argv)
{
	auto extras = _ui.parse(argc, argv);
	_ui.start();

	if (!extras.empty())
	{
		std::string name = extras[0];
		std::vector<std::string> cmdArgs(extras.begin() + 1, extras.end());
		_ui.commands().execute(name, cmdArgs);
	}

	_ui.stop();
	return EXIT_SUCCESS;
}