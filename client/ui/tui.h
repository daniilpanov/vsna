#pragma once
#include <libs/cpptui.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "invoker.h"

namespace fs = std::filesystem;
using namespace cpptui;

std::string timestamp();

enum class LineKind {
  kSystem,
  kUser,
  kResult,
  kError,
};

struct OutputLine {
  std::string text;
  LineKind kind;
};

std::vector<std::string> split_whitespace(const std::string &s);
std::string human_size(uintmax_t bytes);

class HistoryInput : public Input {
 public:
  std::string take_text();
  void add_history(const std::string &entry);
  bool on_event(const Event &event) override;

 private:
  void browse(int dir);

  static constexpr size_t kMaxHistory = 100;
  std::vector<std::string> entries_;
  int index_ = 0;
  std::string draft_;
};

class ChatApp {
 public:
  void run();

 private:
  void build_ui();
  std::shared_ptr<Vertical> build_output_page();
  std::shared_ptr<Vertical> build_settings_page();
  void apply_theme(int idx);
  void submit();

  void register_commands();
  bool execute_command(const std::string &text);

  static void scan_dir_into(TreeNode &node, const fs::path &dir);
  void open_list_dialog(const std::string &path_str);
  void close_list_dialog();

  void on_auto_message();
  void append_system(const std::string &text);
  void append_result(const std::string &text);
  void add_line(LineKind kind, const std::string &text);
  void refresh_output();

  App app_;
  std::shared_ptr<Vertical> root_;
  std::shared_ptr<Tabs> tabs_;
  std::shared_ptr<ScrollableVertical> output_scroll_;
  std::shared_ptr<Static> output_text_;
  std::shared_ptr<HistoryInput> input_;
  std::shared_ptr<Input> settings_field_;
  std::shared_ptr<Dialog> list_dialog_;
  std::shared_ptr<TreeView> list_tree_;
  std::shared_ptr<RadioSet> theme_list_;
  std::shared_ptr<Checkbox> settings_checkbox_;
  TimerId focus_fix_timer_{-1};
  std::vector<OutputLine> lines_;
  Invoker invoker_;
  int auto_message_index_ = 0;
  int auto_message_number_ = 0;
};
