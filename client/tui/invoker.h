#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct Command {
  std::string usage;
  std::string description;
  std::function<void(const std::vector<std::string> &args)> handler;
};

class Invoker {
 public:
  void register_command(
      const std::string &name, const char *usage, const char *description,
      std::function<void(const std::vector<std::string> &args)> handler);
  bool execute(const std::string &name, const std::vector<std::string> &args);
  const std::unordered_map<std::string, Command> &commands() const;

 private:
  std::unordered_map<std::string, Command> commands_;
};
