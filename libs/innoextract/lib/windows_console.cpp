// Minimal Win32 console support for innoextract as an embedded library.
// Upstream util/windows.cpp also defines a standalone main() wrapper, which
// is not appropriate when the extractor is linked into Heroes IV.
#include <windows.h>

namespace util {

int isatty(int fd) {
  DWORD mode = 0;
  const DWORD stream = fd == 0 ? STD_INPUT_HANDLE :
                       fd == 1 ? STD_OUTPUT_HANDLE :
                       fd == 2 ? STD_ERROR_HANDLE : 0;
  return stream && GetConsoleMode(GetStdHandle(stream), &mode) ? 1 : 0;
}

int console_width() {
  CONSOLE_SCREEN_BUFFER_INFO info{};
  HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
  return GetConsoleScreenBufferInfo(out, &info) ? int(info.dwSize.X) : 0;
}

}  // namespace util
