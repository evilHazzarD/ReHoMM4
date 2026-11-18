// What the in-app installer needs from the platform: HTTPS, GOG's login page
// in a web view, picking the installer files, a place for the login token.
// iOS: platform_apple.mm (NSURLSession, WKWebView, UIDocumentPicker, Keychain).
// Linux and the Mac app: platform_desktop.cpp (libcurl, kdialog/zenity or
// osascript, a file; GOG's page in the browser on Linux, gog_login_mac.mm's
// web view on the Mac).
// Android: platform_android.cpp + HeroesActivity.java (HttpURLConnection,
// a WebView activity, ACTION_OPEN_DOCUMENT, the app's private storage).
#pragma once

#include <string>
#include <vector>

namespace h4::install {

// HTTPS GET, with "Authorization: Bearer <bearer>" unless it is empty, and
// "Range: bytes=<range>" unless that is. Any thread. False on a network error
// (*error says why); otherwise *status is the HTTP status and *body its body.
bool HttpGet(const std::string& url, const std::string& bearer, int* status, std::string* body,
             std::string* error, const std::string& range = "");

// Shows GOG's login page (`url`) and waits until GOG sends the browser to
// its login-success page: returns the `code` from that address, or "" if
// the player closed the page. Main thread.
std::string GogLoginCode(const std::string& url);
// Whether GogLoginCode can show the page (Linux: xdg-open and a dialog tool).
bool CanSignInToGog();

// One file the player picked: a path the installer can open (it stays
// readable until EndPickedFiles) and the file's own name.
struct PickedFile {
  std::string path, name;
};
// Lets the player pick files (several at once): shows the system's picker
// and returns at once (false if it cannot be shown); then PickedFiles is
// polled every frame (the app keeps drawing and handling its events: on
// Android the activity is paused and its surface replaced meanwhile) until
// it returns true, with the files (none if cancelled). Main thread.
bool BeginPickFiles();
bool PickedFiles(std::vector<PickedFile>* out);
void EndPickedFiles();

// The GOG refresh token kept between starts (a download can be continued
// after the app was closed); "" if none. Saving "" deletes it.
std::string LoadToken();
void SaveToken(const std::string& token);

// A private scratch folder that can hold symbolic links (Android's shared
// storage cannot): the app's internal storage or temporary folder.
std::string ScratchDir();

// Called once on the main thread before the others (Android: finds the
// Java class while the app's class loader is reachable).
void PlatformInit();

}  // namespace h4::install
