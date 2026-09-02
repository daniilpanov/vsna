#pragma once
#include "ui_controller.h"

// One-shot CLI front-end (vsna_cli). Owns a UiController (and so a NodeApi),
// parses CLI options, then executes the remaining arguments as a single
// command and exits (no interactive loop).
class CliUI {
  public:
	CliUI() : _ui()
	{}
	int run(int argc, char **argv);

  private:
	UiController _ui;
};