#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace h4 {

// The side panels shown now (resolution.cpp): black or the pictures, painted again at once.
void ApplySidePanels(bool black);

inline const char* SidePanelsPath() {
  const char* path = std::getenv("H4_SIDE_PANELS_FILE");
  return path ? path : "";
}

inline bool ReadBlackSidePanels(const char* path) {
  if (FILE* file = std::fopen(path, "r")) {
    char mode[32] = {};
    std::fgets(mode, sizeof mode, file);
    std::fclose(file);
    mode[std::strcspn(mode, " \t\r\n")] = '\0';
    return std::strcmp(mode, "black") == 0;
  }
  return false;
}

inline bool SaveBlackSidePanels(bool on) {
  if (FILE* file = std::fopen(SidePanelsPath(), "w")) {
    const bool written = std::fputs(on ? "black\n" : "wings\n", file) >= 0;
    return std::fclose(file) == 0 && written;
  }
  return false;
}

}  // namespace h4
