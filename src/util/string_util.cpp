//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// string_util.cpp
//
// Identification: /peloton/src/util/string_util.cpp
//
// Copyright (c) 2015-2017, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "util/string_util.h"

#include <string>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <stdarg.h>
#include <string.h>
#include <sstream>

namespace peloton {

bool StringUtil::Contains(const std::string &haystack,
              const std::string &needle) {
  return (haystack.find(needle) != std::string::npos);
}

bool StringUtil::StartsWith(const std::string &str,
                       const std::string &prefix) {
  return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool StringUtil::EndsWith(const std::string &str,
                     const std::string &suffix) {
  if (suffix.size() > str.size()) return (false);
  return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::string StringUtil::Repeat(const std::string &str, const std::size_t n) {
  std::ostringstream os;
  if (n == 0 || str.empty()) {
    return (os.str());
  }
  for (int i = 0; i < static_cast<int>(n); i++) {
    os << str;
  }
  return (os.str());
}

std::vector<std::string> StringUtil::Split(const std::string &str) {
  std::stringstream ss(str);
  std::vector<std::string> lines;
  std::string temp;
  while (std::getline(ss, temp, '\n')) {
    lines.push_back(temp);
  }  // WHILE
  return (lines);
}

std::string StringUtil::Prefix(const std::string &str, const std::string &prefix) {
  std::vector<std::string> lines = StringUtil::Split(str);
  if (lines.empty()) return ("");

  std::ostringstream os;
  for (int i = 0, cnt = lines.size(); i < cnt; i++) {
    if (i > 0) os << std::endl;
    os << prefix << lines[i];
  }  // FOR
  return (os.str());
}

std::string StringUtil::FormatSize(long bytes) {
  double BASE = 1024;
  double KB = BASE;
  double MB = KB * BASE;
  double GB = MB * BASE;

  std::ostringstream os;

  if (bytes >= GB) {
    os << std::fixed << std::setprecision(2) << (bytes / GB) << " GB";
  } else if (bytes >= MB) {
    os << std::fixed << std::setprecision(2) << (bytes / MB) << " MB";
  } else if (bytes >= KB) {
    os << std::fixed << std::setprecision(2) << (bytes / KB) << " KB";
  } else {
    os << std::to_string(bytes) + " bytes";
  }
  return (os.str());
}

std::string StringUtil::Bold(const std::string &str) {
  std::string SET_PLAIN_TEXT = "\033[0;0m";
  std::string SET_BOLD_TEXT = "\033[0;1m";

  std::ostringstream os;
  os << SET_BOLD_TEXT << str << SET_PLAIN_TEXT;
  return (os.str());
}

std::string StringUtil::Upper(const std::string &str) {
  std::string copy(str);
  std::transform(copy.begin(), copy.end(), copy.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return (copy);
}

std::string StringUtil::Format(const std::string fmt_str, ...) {
  // Reserve two times as much as the length of the fmt_str
  int final_n, n = ((int)fmt_str.size()) * 2;
  std::string str;
  std::unique_ptr<char[]> formatted;
  va_list ap;

  while (1) {
    // Wrap the plain char array into the unique_ptr
    formatted.reset(new char[n]);
    strcpy(&formatted[0], fmt_str.c_str());
    va_start(ap, fmt_str);
    final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
    va_end(ap);
    if (final_n < 0 || final_n >= n)
      n += abs(final_n - n + 1);
    else
      break;
  }
  return std::string(formatted.get());
}

}