// Heroes IV from the player's GOG.com library: GOG Galaxy's login (the
// public Galaxy client, as open-source GOG clients use it), the ownership
// check, and the download of the English Galaxy build ("3.0 win11 GOG 0.2",
// the same files as the offline installer: manifest.h) from GOG's CDN.
#pragma once

#include <string>

#include "install/task.h"

namespace h4::install::gog {

constexpr const char* kProductId = "1207658915";  // Heroes of Might and Magic 4: Complete

// GOG's login page; its success page carries the code for SignIn.
std::string LoginUrl();
bool IsLoginSuccessUrl(const std::string& url);

class Session {
 public:
  // Exchanges the login page's code for tokens (and keeps the refresh token:
  // platform SaveToken).
  bool SignIn(const std::string& code, std::string* error);
  // Signs in again from the kept refresh token; false if there is none or
  // GOG refused it.
  bool Resume(std::string* error);
  void SignOut();

  // Whether the account owns Heroes IV Complete.
  bool OwnsGame(bool* owns, std::string* error);

  // Downloads the manifest's files into `dir` (created); what is already
  // there and matches GOG's chunk checksums is kept, so a cancelled or
  // interrupted download continues. The caller checks SHA-256 afterwards.
  bool Download(const std::string& dir, Task* task, std::string* error);

  // A valid access token (refreshed when near expiry); "" on failure.
  std::string Access(std::string* error);

 private:
  bool Token(const std::string& query, std::string* error);
  std::string access_, refresh_;
  long long expires_ = 0;  // unix seconds
};

}  // namespace h4::install::gog
