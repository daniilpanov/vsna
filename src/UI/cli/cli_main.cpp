#include "cli_ui.h"

int main(int argc, char *argv[])
{
	CliUI cliUI;
	cliUI.run(argc, argv);

	return EXIT_SUCCESS;
}