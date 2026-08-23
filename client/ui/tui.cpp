#include "tui.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {

const char *kAutoMessages[] = {
    "Heartbeat received - all systems operational.",
    "Checkpoint saved successfully.",
    "Network latency nominal (12 ms).",
    "Background job finished without errors.",
    "Cache refreshed (128 entries).",
    "Telemetry batch uploaded to the server.",
    "Disk usage at 42 percent - no action required.",
    "Security scan completed: no threats found.",
};
constexpr int kAutoMessageCount =
    static_cast<int>(sizeof(kAutoMessages) / sizeof(kAutoMessages[0]));

struct ThemeOption {
  const char *name;
  Theme (*factory)();
};

const ThemeOption kThemeOptions[] = {
    {"Dark", &Theme::Dark},
    {"Light", &Theme::Light},
    {"Nord", &Theme::Nord},
    {"Tokyo Night", &Theme::TokyoNight},
    {"Solarized Light", &Theme::SolarizedLight},
};
constexpr int kThemeOptionCount =
    static_cast<int>(sizeof(kThemeOptions) / sizeof(kThemeOptions[0]));

const char *kHelloBanner =
    R"(
+------------------------------------------------+
| ___      ___ ________  ________   ________     |
||\  \    /  /|\   ____\|\   ___  \|\   __  \    |
|\ \  \  /  / | \  \___|\ \  \\ \  \ \  \|\  \   |
| \ \  \/  / / \ \_____  \ \  \\ \  \ \   __  \  |
|  \ \    / /   \|____|\  \ \  \\ \  \ \  \ \  \ |
|   \ \__/ /      ____\_\  \ \__\\ \__\ \__\ \__\|
|    \|__|/      |\_________\|__| \|__|\|__|\|__||
|                \|_________|                    |
+------------------------------------------------+)";

}  // namespace

std::string timestamp() {
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm tm_buf{};
#ifdef _WIN32
  localtime_s(&tm_buf, &now);
#else
  localtime_r(&now, &tm_buf);
#endif
  char buf[16];
  std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm_buf);
  return buf;
}

std::vector<std::string> split_whitespace(const std::string &s) {
  std::vector<std::string> out;
  size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) i++;
    size_t j = i;
    while (j < s.size() && !std::isspace(static_cast<unsigned char>(s[j]))) j++;
    if (j > i) out.push_back(s.substr(i, j - i));
    i = j;
  }
  return out;
}

std::string human_size(uintmax_t bytes) {
  const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  double value = static_cast<double>(bytes);
  int unit = 0;
  while (value >= 1024.0 && unit < 4) {
    value /= 1024.0;
    unit++;
  }
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(unit == 0 ? 0 : 1) << value << " "
      << units[unit];
  return oss.str();
}

// HistoryInput

std::string HistoryInput::take_text() {
  std::string text = get_value();
  set_value("");
  index_ = static_cast<int>(entries_.size());
  return text;
}

void HistoryInput::add_history(const std::string &entry) {
  if (entry.empty()) return;
  if (!entries_.empty() && entries_.back() == entry) {
    index_ = static_cast<int>(entries_.size());
    return;
  }
  entries_.push_back(entry);
  if (entries_.size() > kMaxHistory) entries_.erase(entries_.begin());
  index_ = static_cast<int>(entries_.size());
}

bool HistoryInput::on_event(const Event &event) {
  if (has_focus() && event.is_key_event()) {
    if (event.is_nav_up()) {
      browse(-1);
      return true;
    }
    if (event.is_nav_down()) {
      browse(1);
      return true;
    }
    index_ = static_cast<int>(entries_.size());
  }
  return Input::on_event(event);
}

void HistoryInput::browse(int dir) {
  if (entries_.empty()) return;

  int end = static_cast<int>(entries_.size());
  if (index_ >= end) draft_ = get_value();

  int new_index = index_ + dir;
  if (new_index < 0) new_index = 0;
  if (new_index > end) new_index = end;
  if (new_index == index_) return;

  index_ = new_index;
  set_value(index_ == end ? draft_ : entries_[index_]);
}

// ChatApp

void ChatApp::run() {
  register_commands();
  build_ui();
  append_system("Welcome! Type a message below and press Enter to send it.");
  append_system("Automatic status messages arrive every 3 seconds (toggle in Settings).");
  append_system("Type 'help' to list available commands.");
  append_system("Press Ctrl+C or click Quit to exit.");
  app_.add_timer(3000, [this] { on_auto_message(); });
  focus_fix_timer_ = app_.add_timer(1, [this] {
    app_.remove_timer(focus_fix_timer_);
    input_->set_focus(true);
  });
  app_.run(root_);
}

void ChatApp::build_ui() {
  Theme::set_theme(Theme::Dark());
  root_ = std::make_shared<Vertical>();
  tabs_ = std::make_shared<Tabs>();

  tabs_->add_action_button(
      std::make_shared<Button>("Quit", [] { App::quit(); }));

  tabs_->add_tab("Output", build_output_page());
  tabs_->add_tab("Settings", build_settings_page());

  root_->add(tabs_);

  app_.register_key(13, [this] { submit(); });
  app_.register_key(10, [this] { submit(); });
  app_.register_key(
      27, [this] { close_list_dialog(); }, false, false, false, false);
}

std::shared_ptr<Vertical> ChatApp::build_output_page() {
  auto page = std::make_shared<Vertical>();

  output_text_ = std::make_shared<Static>(StyledText(""));
  output_scroll_ = std::make_shared<ScrollableVertical>();
  output_scroll_->tab_stop = false;
  output_scroll_->add(output_text_);

  auto output_border = std::make_shared<Border>(BorderStyle::Rounded);
  output_border->set_title(" Output ");
  output_border->add(output_scroll_);

  auto prompt = std::make_shared<Label>("> ");
  prompt->fixed_width = 2;

  input_ = std::make_shared<HistoryInput>();
  input_->placeholder = "Type a message and press Enter...";

  auto input_row = std::make_shared<Horizontal>();
  input_row->fixed_height = 1;
  input_row->add(prompt);
  input_row->add(input_);

  auto input_border = std::make_shared<Border>(BorderStyle::Rounded);
  input_border->fixed_height = 3;
  input_border->add(input_row);

  page->add(output_border);
  page->add(input_border);
  return page;
}

std::shared_ptr<Vertical> ChatApp::build_settings_page() {
  auto page = std::make_shared<Vertical>();

  settings_field_ = std::make_shared<Input>();
  settings_field_->set_value("Application Settings");
  page->add(settings_field_);
  page->add(std::make_shared<VerticalSpacer>(1));

  theme_list_ = std::make_shared<RadioSet>();
  std::vector<StyledText> names;
  for (const auto &option : kThemeOptions) names.emplace_back(option.name);
  theme_list_->set_options(names);
  theme_list_->on_change = [this](int idx) { apply_theme(idx); };
  page->add(theme_list_);

  page->add(std::make_shared<VerticalSpacer>(1));
  settings_checkbox_ = std::make_shared<Checkbox>(
      "Enable automatic status messages", true);
  page->add(settings_checkbox_);
  return page;
}

void ChatApp::apply_theme(int idx) {
  if (idx < 0 || idx >= kThemeOptionCount) return;
  Theme::set_theme(kThemeOptions[idx].factory());
  refresh_output();
}

void ChatApp::submit() {
  const std::string text = input_->take_text();
  if (text.empty()) return;
  input_->add_history(text);

  if (execute_command(text)) return;

  add_line(LineKind::kUser, "[" + timestamp() + "] You: " + text);
}

void ChatApp::register_commands() {
  invoker_.register_command(
      "exit", "exit", "Close the application.",
      [this](const std::vector<std::string> &) {
        append_system("Goodbye!");
        App::quit();
      });

  invoker_.register_command(
      "hello", "hello", "Print an ASCII banner.",
      [this](const std::vector<std::string> &) {
        add_line(LineKind::kResult, kHelloBanner);
      });

  invoker_.register_command(
      "calculate", "calculate <number> <number>",
      "Print the sum of two numbers.",
      [this](const std::vector<std::string> &args) {
        if (args.size() != 2) {
          add_line(LineKind::kError, "Usage: calculate <number> <number>");
          return;
        }
        try {
          double sum = std::stod(args[0]) + std::stod(args[1]);
          std::ostringstream oss;
          oss << sum;
          append_result(oss.str());
        } catch (...) {
          add_line(LineKind::kError, "Error: arguments must be numbers.");
        }
      });

  invoker_.register_command(
      "list", "list [directory]", "Open a directory browser dialog.",
      [this](const std::vector<std::string> &args) {
        if (args.size() > 1) {
          add_line(LineKind::kError, "Usage: list [directory]");
          return;
        }
        open_list_dialog(args.empty() ? "." : args[0]);
      });

  invoker_.register_command(
      "help", "help", "List available commands.",
      [this](const std::vector<std::string> &) {
        auto &cmds = invoker_.commands();
        std::vector<const std::pair<const std::string, Command> *> sorted;
        for (const auto &entry : cmds) sorted.push_back(&entry);
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto *a, const auto *b) {
                    return a->first < b->first;
                  });
        std::string out = "Available commands:";
        for (const auto *entry : sorted)
          out += "\n  " + entry->second.usage + " - " +
                 entry->second.description;
        append_result(out);
      });
}

bool ChatApp::execute_command(const std::string &text) {
  std::vector<std::string> tokens = split_whitespace(text);
  if (tokens.empty()) return true;

  return invoker_.execute(tokens[0], {tokens.begin() + 1, tokens.end()});
}

void ChatApp::scan_dir_into(TreeNode &node, const fs::path &dir) {
  std::error_code ec;
  std::vector<fs::directory_entry> dirs, files;
  fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec),
      end;
  for (; !ec && it != end; it.increment(ec)) {
    if (it->is_directory(ec) && !ec)
      dirs.push_back(*it);
    else
      files.push_back(*it);
    ec.clear();
  }

  auto name_of = [](const fs::directory_entry &e) {
    return e.path().filename().string();
  };
  std::sort(dirs.begin(), dirs.end(),
            [&name_of](const auto &a, const auto &b) {
              return name_of(a) < name_of(b);
            });
  std::sort(files.begin(), files.end(),
            [&name_of](const auto &a, const auto &b) {
              return name_of(a) < name_of(b);
            });

  std::vector<TreeNode> kids;
  for (const auto &d : dirs) {
    TreeNode child;
    child.label = name_of(d);
    child.user_data = "dir:" + d.path().string();
    child.icon = "+";
    kids.push_back(std::move(child));
  }
  for (const auto &f : files) {
    TreeNode child;
    child.label = name_of(f);
    child.user_data = "file:" + f.path().string();
    kids.push_back(std::move(child));
  }
  node.children = std::move(kids);
}

void ChatApp::open_list_dialog(const std::string &path_str) {
  std::error_code ec;
  fs::path target = fs::absolute(fs::path(path_str), ec);
  if (!fs::is_directory(target, ec)) {
    add_line(LineKind::kError, "Error: not a directory: " + path_str);
    return;
  }

  list_dialog_ = std::make_shared<Dialog>(&app_, BorderStyle::Single);
  list_dialog_->modal = true;
  list_dialog_->fixed_width = 62;
  list_dialog_->fixed_height = 20;
  auto term_size = Terminal::getSize();
  int dlg_w = 62;
  int dlg_h = 20;
  if (dlg_w > term_size.first - 2) dlg_w = term_size.first - 2;
  if (dlg_h > term_size.second - 2) dlg_h = term_size.second - 2;
  if (dlg_w < 24) dlg_w = 24;
  if (dlg_h < 6) dlg_h = 6;
  list_dialog_->width = dlg_w;
  list_dialog_->height = dlg_h;

  std::string title = target.string();
  if (title.size() > 44) title = "..." + title.substr(title.size() - 41);
  list_dialog_->set_title(" List: " + title +
                          " (Esc - close, click file - select) ");

  list_tree_ = std::make_shared<TreeView>();
  TreeNode root;
  root.label = target.filename().string();
  if (root.label.empty()) root.label = target.string();
  root.user_data = "dir:" + target.string();
  root.expanded = true;
  scan_dir_into(root, target);
  list_tree_->root_nodes.push_back(std::move(root));
  list_tree_->refresh();

  list_tree_->on_submit = [this](TreeNode *node) {
    const std::string &data = node->user_data;
    if (data.rfind("dir:", 0) == 0) {
      if (!node->children.empty()) {
        node->expanded = true;
      } else {
        scan_dir_into(*node, fs::path(data.substr(4)));
        node->icon.clear();
        node->expanded = true;
      }
      list_tree_->refresh();
      return;
    }

    fs::path file(data.substr(5));
    std::error_code ec;
    uintmax_t size = fs::file_size(file, ec);
    append_result(node->label + " | Size: " +
                  (ec ? std::string("unknown") : human_size(size)));
    close_list_dialog();
  };

  list_dialog_->add(list_tree_);
  app_.open_dialog(list_dialog_);
}

void ChatApp::close_list_dialog() {
  if (list_dialog_ && list_dialog_->is_open) {
    app_.close_dialog(list_dialog_);
    input_->set_focus(true);
  }
}

void ChatApp::on_auto_message() {
  if (!settings_checkbox_->is_checked()) return;
  auto_message_index_ =
      (auto_message_index_ + 1) % kAutoMessageCount;
  int n = ++auto_message_number_;
  add_line(LineKind::kSystem,
           "[" + timestamp() + "] System: auto message #" +
               std::to_string(n) + ": " + kAutoMessages[auto_message_index_]);
}

void ChatApp::append_system(const std::string &text) {
  add_line(LineKind::kSystem, "[" + timestamp() + "] System: " + text);
}

void ChatApp::append_result(const std::string &text) {
  add_line(LineKind::kResult, "[" + timestamp() + "] Result: " + text);
}

void ChatApp::add_line(LineKind kind, const std::string &text) {
  lines_.push_back({text, kind});
  refresh_output();
}

void ChatApp::refresh_output() {
  const Theme &theme = Theme::current();
  StyledText styled;
  int total_rows = 0;
  for (const auto &line : lines_) {
    switch (line.kind) {
      case LineKind::kUser:
        styled.colored(line.text + "\n", theme.success);
        break;
      case LineKind::kError:
        styled.colored(line.text + "\n", theme.error);
        break;
      case LineKind::kResult:
        styled.colored(line.text + "\n", theme.primary);
        break;
      default:
        styled.add(line.text + "\n");
    }
    total_rows +=
        1 + static_cast<int>(std::count(line.text.begin(), line.text.end(),
                                        '\n'));
  }
  output_text_->set_text(styled);
  output_text_->fixed_height = total_rows;

  int view_height = output_scroll_->height;
  int max_scroll = total_rows - view_height;
  output_scroll_->scroll_offset = max_scroll > 0 ? max_scroll : 0;
}

int main() {
  ChatApp app;
  app.run();
  return 0;
}
