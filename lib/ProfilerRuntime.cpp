#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <string_view>

#define PROFILER_VERBOSE_ENV "PROFILER_VERBOSE_ENV"
#define PROFILER_THRESHOLD_ENV "PROFILER_THRESHOLD_ENV"

static const int64_t DEFAULT_THRESHOLD = 1000;

static std::map<std::string, int64_t> counts;

static std::string join_c_strings(char *a, char *b, char *c) {
  std::string a_string(a);
  std::string b_string(b);
  std::string c_string(c);

  return a_string + b_string + c_string;
}

static bool is_verbose() {
  return std::getenv(PROFILER_VERBOSE_ENV) != nullptr ? true : false;
}

static int64_t get_threshold() {

  int64_t threshold = DEFAULT_THRESHOLD;
  if (const char *env_p = std::getenv(PROFILER_THRESHOLD_ENV)) {
    threshold = strtol(env_p, nullptr, 10);
  }

  return threshold;
}

extern "C" __attribute__((noinline)) void
bb_exec(char *bb_name, char *function_name, char *module_name) {
  // TODO: there must be a faster way to generate/compute
  //       a unique key based on these three values.

  std::string key = join_c_strings(bb_name, function_name, module_name);

  if (is_verbose()) {
    std::cout << "[PROFILER] Profiling basic block '" << bb_name
              << "' from function '" << function_name << "' in module '"
              << module_name << "'\n";
  }

  auto it = counts.find(key);
  int64_t newCount = (it != counts.end() ? it->second : 0) + 1;

  if (newCount == get_threshold() + 1) {
    std::cout << "[PROFILER] Basic block '" << bb_name << "' from function '"
              << function_name << "' in module '" << module_name
              << "' was executed more than " << get_threshold() << " times.\n";
  }

  // We are updating the new count even for basic blocks which are already
  // reported.
  counts[key] = newCount;
}