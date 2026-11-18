// The Switch's newlib (devkitA64) has no pread: lseek + read (each picked
// descriptor is read from one thread at a time). Force-included there only.
#pragma once
#ifdef __SWITCH__
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifndef O_DIRECTORY
#define O_DIRECTORY 0  // Boost.Filesystem's opendir flags
#endif
#ifdef __cplusplus
extern "C" {
inline ssize_t pread(int fd, void* buf, size_t n, off_t at) {
  if (lseek(fd, at, SEEK_SET) == (off_t)-1) return -1;
  return read(fd, buf, n);
}
}
#endif
#endif
