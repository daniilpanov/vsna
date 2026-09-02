#pragma once
#include <string>
#include <vector>

#include "command_manager.h"
#include "node_api.h"

// A UiController owns a NodeApi and the shared UI plumbing used by every
// front-end (cli, repl, web). It parses CLI options with CLI11, binds the
// peer-discovery observer, starts the node (listening + optional startup
// connects) and exposes the command manager. Front-ends only decide how to
// drive it interactively afterwards.
class UiController {
  public:
	UiController() : _commandManager(_api)
	{}

	// Parse argc/argv with CLI11. Returns the leftover (non-option) arguments,
	// or std::nullopt if the process should terminate (e.g. --help handled).
	std::vector<std::string> parse(int argc, char **argv);

	// Install the peer-discovery observer, start listening and process any
	// startup --connect targets.
	void start();

	// Stop the underlying node and release worker threads.
	void stop();

	// Split one line of input into a command name + arguments.
	static std::pair<std::string, std::vector<std::string>> splitLine(const std::string& input);

	CommandManager& commands()
	{
		return _commandManager;
	}
	const NodeApi& api() const
	{
		return _api;
	}
	std::string describe() const
	{
		return _api.describe();
	}

  private:
	NodeApi _api;
	CommandManager _commandManager;
	std::vector<std::string> _startConnects;
};