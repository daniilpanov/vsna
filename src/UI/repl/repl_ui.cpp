#include "repl_ui.h"

#include <iostream>
#include <string>
#include <vector>

int ReplUI::run(int argc, char **argv)
{
	_ui.parse(argc, argv);
	_ui.start();

	std::cout << _ui.describe() << std::endl;

	loop();

	_ui.stop();
	return EXIT_SUCCESS;
}

void ReplUI::loop()
{
	std::string input;
	while (true)
	{
		std::cout << "> ";
		std::getline(std::cin, input);
		auto [name, cmdArgs] = UiController::splitLine(input);

		if (name.empty())
			continue;

		if (_ui.commands().execute(name, cmdArgs))
			break;
	}
}