// configure.hpp - innoextract's build configuration (src/configure.hpp.in)
// for iOS and Android (bionic, API 29+: timegm, utimensat, iconv), written out
// instead of probed: the library is cross-compiled, where CMake's checks
// cannot run the results.
#ifndef INNOEXTRACT_CONFIGURE_HPP
#define INNOEXTRACT_CONFIGURE_HPP

// Console functions
#define INNOEXTRACT_HAVE_ISATTY 1
#define INNOEXTRACT_HAVE_IOCTL 1

// Time functions
#define INNOEXTRACT_HAVE_TIMEGM 1
#define INNOEXTRACT_HAVE_GMTIME_R 1

// File functions
#define INNOEXTRACT_HAVE_UTIMENSAT 1
#define INNOEXTRACT_HAVE_DYNAMIC_UTIMENSAT 0
#define INNOEXTRACT_HAVE_AT_FDCWD 1
#define INNOEXTRACT_HAVE_UTIMES 1

// Shared functions
#define INNOEXTRACT_HAVE_DLSYM 0

// Endianness
#define INNOEXTRACT_HAVE_BUILTIN_BSWAP16 1
#define INNOEXTRACT_HAVE_BUILTIN_BSWAP32 1
#define INNOEXTRACT_HAVE_BUILTIN_BSWAP64 1
#define INNOEXTRACT_HAVE_BSWAP_16 0
#define INNOEXTRACT_HAVE_BSWAP_32 0
#define INNOEXTRACT_HAVE_BSWAP_64 0

// C++11 functionality
#define INNOEXTRACT_HAVE_ALIGNOF 1
#define INNOEXTRACT_HAVE_STD_CODECVT_UTF8_UTF16 0
#define INNOEXTRACT_HAVE_STD_UNIQUE_PTR 1

// Optional dependencies (the system's libiconv)
#define INNOEXTRACT_HAVE_ARC4 1
#define INNOEXTRACT_HAVE_LZMA 1
#define INNOEXTRACT_HAVE_ICONV 1
#define INNOEXTRACT_HAVE_WIN32_CONV 0
#define INNOEXTRACT_HAVE_BUILTIN_CONV 1

// Process functions: none (no helper programs on iOS; --gog's unrar is not used)
#define INNOEXTRACT_HAVE_POSIX_SPAWNP 0
#define INNOEXTRACT_HAVE_UNISTD_ENVIRON 0
#define INNOEXTRACT_HAVE_FORK 0
#define INNOEXTRACT_HAVE_EXECVP 0
#define INNOEXTRACT_HAVE_WAITPID 0

// The Switch's newlib: no timegm, utimensat or ioctl (file times are left as extracted).
#ifdef __SWITCH__
#undef INNOEXTRACT_HAVE_IOCTL
#define INNOEXTRACT_HAVE_IOCTL 0
#undef INNOEXTRACT_HAVE_TIMEGM
#define INNOEXTRACT_HAVE_TIMEGM 0
#undef INNOEXTRACT_HAVE_UTIMENSAT
#define INNOEXTRACT_HAVE_UTIMENSAT 0
#undef INNOEXTRACT_HAVE_AT_FDCWD
#define INNOEXTRACT_HAVE_AT_FDCWD 0
#undef INNOEXTRACT_HAVE_UTIMES
#define INNOEXTRACT_HAVE_UTIMES 0
#undef INNOEXTRACT_HAVE_ICONV
#define INNOEXTRACT_HAVE_ICONV 0  // newlib has the header, not the functions: the built-in converters
#endif

// MinGW has the Win32 console APIs, not the POSIX ioctl header.  The
// innoextract library only needs console output for diagnostic progress.
#ifdef _WIN32
#undef INNOEXTRACT_HAVE_ISATTY
#define INNOEXTRACT_HAVE_ISATTY 0
#undef INNOEXTRACT_HAVE_IOCTL
#define INNOEXTRACT_HAVE_IOCTL 0
#undef INNOEXTRACT_HAVE_ICONV
#define INNOEXTRACT_HAVE_ICONV 0
#undef INNOEXTRACT_HAVE_WIN32_CONV
#define INNOEXTRACT_HAVE_WIN32_CONV 1
#undef INNOEXTRACT_HAVE_UTIMENSAT
#define INNOEXTRACT_HAVE_UTIMENSAT 0
#undef INNOEXTRACT_HAVE_AT_FDCWD
#define INNOEXTRACT_HAVE_AT_FDCWD 0
#undef INNOEXTRACT_HAVE_UTIMES
#define INNOEXTRACT_HAVE_UTIMES 0
#endif

#endif // INNOEXTRACT_CONFIGURE_HPP
