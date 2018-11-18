#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <locale>
#include <numeric>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class ArgumentParser {
 public:
  class ArgumentException : public std::runtime_error {
   public:
    ArgumentException(const char *msg) noexcept : std::runtime_error(msg) {}
  };

  ArgumentParser(const std::string &desc) : _desc(desc), _help(false) {}
  ArgumentParser(const std::string desc, int argc, char *argv[])
      : ArgumentParser(desc) {
    parse(argc, argv);
  }

  void add_argument(const std::string &name, const std::string &desc,
                    bool required = false) {
    _arguments.push_back({name, desc, required});
  }

  bool is_help() { return _help; }

  void print_help(char *f) {
    std::cout << "Usage: " << f << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    for (auto &a : _arguments) {
      std::cout << "    " << std::setw(23) << std::left << a._name
                << std::setw(23) << a._desc + (a._required ? " (Required)" : "")
                << std::endl;
    }
  }

  void parse(int argc, char *argv[]) {
    if (argc > 1) {
      std::vector<std::string> argsvec(argv + 1, argv + argc);
      std::string argstr =
          std::accumulate(argsvec.begin(), argsvec.end(), std::string(),
                          [](std::string a, std::string b) {
                            return a + " " + std::string(b);
                          });
      std::vector<std::string> split_args = split(argstr, "^-|\\s-");
      std::vector<std::string> arg_parts;
      std::string name;
      for (auto &split_arg : split_args) {
        std::vector<std::string> arg_parts = split(split_arg, "\\s+");
        name = trim_copy(arg_parts[0]);
        if (name.empty()) continue;
        if (name[0] == '-') {
          _add_variable(name, arg_parts, argv);
        } else {
          for (auto c : name) {
            _add_variable(std::string(1, c), arg_parts, argv);
          }
        }
      }
    }
    if (!_help) {
      for (auto &a : _arguments) {
        if (a._required) {
          if (_variables.find(a._name) == _variables.end())
            throw ArgumentException(
                ("Required argument not found: " + a._name).c_str());
        }
      }
    }
  }

  bool exists(const std::string &name) {
    return _variables.find(delimit(name)) != _variables.end();
  }

  template <typename T>
  std::vector<T> getv(const std::string &name) {
    std::string argstr = get<std::string>(name);
    std::vector<T> v;
    std::istringstream in(argstr);
    T t;
    while (in >> t >> std::ws) v.push_back(t);
    return v;
  }

  template <typename T>
  T get(const std::string &name) {
    std::istringstream in(get<std::string>(name));
    T t;
    in >> t >> std::ws;
    return t;
  }

 private:
  struct Argument {
   public:
    Argument(const std::string &name, const std::string &desc,
             bool required = false)
        : _name(name), _desc(desc), _required(required) {}

   private:
    std::string _name;
    std::string _desc;
    bool _required;
    friend class ArgumentParser;
  };
  inline bool _add_variable(std::string name,
                            std::vector<std::string> &arg_parts, char *argv[]) {
    if (name == "h" || name == "-help") {
      _help = true;
      print_help(argv[0]);
    }
    ltrim(name, [](int c) { return c != (int)'-'; });
    name = delimit(name);
    std::string val;
    if (arg_parts.size() > 1)
      val = std::accumulate(arg_parts.begin() + 1, arg_parts.end(),
                            std::string(), [](std::string a, std::string b) {
                              return trim_copy(a) + " " + trim_copy(b);
                            });
    else
      val = "";
    _variables[name] = val;
  }
  static std::string delimit(const std::string &name) {
    return std::string(std::min(name.size(), (size_t)2), '-').append(name);
  }
  static std::string strip(const std::string &name) {
    size_t begin = 0;
    begin += name.size() > 0 ? name[0] == '-' : 0;
    begin += name.size() > 3 ? name[1] == '-' : 0;
    return name.substr(begin);
  }
  static std::string upper(const std::string &in) {
    std::string out(in);
    std::transform(out.begin(), out.end(), out.begin(), ::toupper);
    return out;
  }
  static std::string escape(const std::string &in) {
    std::string out(in);
    if (in.find(' ') != std::string::npos)
      out = std::string("\"").append(out).append("\"");
    return out;
  }
  static std::vector<std::string> split(const std::string &input,
                                        const std::string &regex) {
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator first{input.begin(), input.end(), re, -1}, last;
    return {first, last};
  }
  static bool not_space(int ch) { return !std::isspace(ch); }
  static inline void ltrim(std::string &s, bool (*f)(int) = not_space) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), f));
  }
  static inline void rtrim(std::string &s, bool (*f)(int) = not_space) {
    s.erase(std::find_if(s.rbegin(), s.rend(), f).base(), s.end());
  }
  static inline void trim(std::string &s, bool (*f)(int) = not_space) {
    ltrim(s, f);
    rtrim(s, f);
  }
  static inline std::string ltrim_copy(std::string s,
                                       bool (*f)(int) = not_space) {
    ltrim(s, f);
    return s;
  }
  static inline std::string rtrim_copy(std::string s,
                                       bool (*f)(int) = not_space) {
    rtrim(s, f);
    return s;
  }
  static inline std::string trim_copy(std::string s,
                                      bool (*f)(int) = not_space) {
    trim(s, f);
    return s;
  }

  bool _help;
  std::string _desc;
  std::vector<Argument> _arguments;
  std::unordered_map<std::string, std::string> _variables;
};
template <>
inline std::string ArgumentParser::get<std::string>(const std::string &name) {
  auto v = _variables.find(delimit(name));
  if (v != _variables.end()) return v->second;
  return "";
}
template <>
inline bool ArgumentParser::get<bool>(const std::string &name) {
  return exists(name);
}
template <>
inline std::vector<std::string> ArgumentParser::getv<std::string>(
    const std::string &name) {
  std::string argstr = get<std::string>(name);
  return split(argstr, "\\s");
}

#endif
