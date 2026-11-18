// The game files an install must have: path, size and SHA-256 of each
// (manifest_en.inc, platforms/tools/gen_manifest.py), and checking a folder.
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace h4::install {

struct ManifestFile {
  const char* path;  // '/'-separated, the installer's spelling
  uint64_t size;
  const char* sha256;
};

const std::vector<ManifestFile>& Manifest();
uint64_t ManifestBytes();

// The manifest entry for `path` compared without case ('\\' or '/'), or null.
const ManifestFile* FindInManifest(const std::string& path);

// Checks every manifest file in `dir`; returns the paths that are missing,
// the wrong size or the wrong SHA-256. `progress(done, total)` gets bytes
// read; `cancel` stops it (then the result is incomplete).
std::vector<std::string> VerifyDir(const std::string& dir,
                                   const std::function<void(uint64_t, uint64_t)>& progress,
                                   const std::atomic<bool>& cancel);

// Small file helpers shared by the installer.
bool MakeDirs(const std::string& path);
bool RemoveTree(const std::string& path);
bool IsDirectory(const std::string& path);
bool IsFile(const std::string& path);
// Bytes free on the volume holding `dir`, or -1.
long long FreeBytes(const std::string& dir);

}  // namespace h4::install
