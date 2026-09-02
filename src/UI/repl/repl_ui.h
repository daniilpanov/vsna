#pragma once
#include "ui_controller.h"

// Interactive REPL front-end (vsna_repl). Owns a UiController (and so a
// NodeApi), parses CLI options, then runs a `> ` command loop until `exit`.
class ReplUI {
  public:
	ReplUI() : _ui()
	{}
	int run(int argc, char **argv);

  private:
	UiController _ui;
	void loop();
};