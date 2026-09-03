#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>

#include "node_ui.h"

int main(int argc, char *argv[])
{
	NodeUI nodeUI;
	nodeUI.run(argc, argv);

	return EXIT_SUCCESS;
}
