#include "repl_ui.h"

int main(int argc, char *argv[])
{
	ReplUI replUI;
	replUI.run(argc, argv);

	return EXIT_SUCCESS;
}