#include "invoker.h"

void Invoker::register_command(
    const std::string &name, const char *usage, const char *description,
    std::function<void(const std::vector<std::string> &args)> handler) {
  commands_[name] = {usage, description, std::move(handler)};
}

bool Invoker::execute(const std::string &name,
                       const std::vector<std::string> &args) {
  auto it = commands_.find(name);
  if (it == commands_.end()) return false;
  it->second.handler(args);
  return true;
}

const std::unordered_map<std::string, Command> &Invoker::commands() const {
  return commands_;
}
