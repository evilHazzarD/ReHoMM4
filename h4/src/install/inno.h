// Heroes IV from GOG's offline installer (setup_heroes_of_might_and_magic_4_
// complete_*.exe + its -1.bin), unpacked in-process by innoextract
// (libs/innoextract, recomp_innoextract.h).
#pragma once

#include <string>
#include <vector>

#include "install/platform.h"
#include "install/task.h"

namespace h4::install::inno {

// The installer among `files` (the .exe and its "<name>-1.bin" side by
// side): fills *exe and *bin. False with *error naming what is missing.
bool FindInstaller(const std::vector<PickedFile>& files, PickedFile* exe, PickedFile* bin,
                   std::string* error);

// The installer files in folder `dir` (not its subfolders), or none.
std::vector<PickedFile> FilesIn(const std::string& dir);

// Checks that `exe` is GOG's Heroes IV Complete and unpacks the manifest's
// files into `dir` (which should be empty). `stage` is a scratch folder for
// links that give the two files the names innoextract expects.
bool Extract(const PickedFile& exe, const PickedFile& bin, const std::string& stage,
             const std::string& dir, Task* task, std::string* error);

}  // namespace h4::install::inno
