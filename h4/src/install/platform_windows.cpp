// Windows implementation of install/platform.h: WinHTTP, the standard file
// picker and the user's browser. Nothing external is bundled or required.
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <SDL3/SDL.h>

#include <atomic>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "install/platform.h"

namespace fs = std::filesystem;
namespace h4::install {
namespace {
std::mutex g_pick_mu;
std::vector<PickedFile> g_picked;
std::atomic<bool> g_pick_done{true};

std::wstring Wide(const std::string& s) {
  if (s.empty()) return {};
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
  std::wstring r(n, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), r.data(), n);
  return r;
}
std::string Utf8(const wchar_t* s) {
  if (!s) return {};
  int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
  std::string r(n > 0 ? n - 1 : 0, '\0');
  if (n > 1) WideCharToMultiByte(CP_UTF8, 0, s, -1, r.data(), n, nullptr, nullptr);
  return r;
}
std::string LocalAppData() {
  PWSTR p = nullptr; std::string r = ".";
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) { r = Utf8(p); CoTaskMemFree(p); }
  return r;
}
std::string TokenPath() { return LocalAppData() + "/ReHoMM4/player/gog.token"; }
std::string CodeIn(std::string s) {
  const size_t at = s.find("code=");
  if (at == std::string::npos || s.find("on_login_success") == std::string::npos) return {};
  size_t e = at + 5; while (e < s.size() && s[e] != '&' && s[e] != '#') ++e;
  return s.substr(at + 5, e - at - 5);
}
void AddFolder(const std::string& root, std::vector<PickedFile>* out) {
  std::error_code ec;
  for (fs::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec))
    if (it->is_regular_file(ec)) out->push_back({it->path().string(), it->path().lexically_relative(root).generic_string()});
  if (out->empty()) out->push_back({root, ""});
}
}

void PlatformInit() {}
std::string ScratchDir() { return LocalAppData() + "/ReHoMM4/cache"; }

bool HttpGet(const std::string& url, const std::string& bearer, int* status, std::string* body,
             std::string* error, const std::string& range) {
  std::wstring u = Wide(url); URL_COMPONENTS c{sizeof c}; wchar_t host[256] = {}, path[4096] = {};
  c.lpszHostName = host; c.dwHostNameLength = 256; c.lpszUrlPath = path; c.dwUrlPathLength = 4096;
  if (!WinHttpCrackUrl(u.c_str(), DWORD(u.size()), 0, &c)) { *error = "invalid URL"; return false; }
  HINTERNET ses = WinHttpOpen(L"ReHoMM4", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
  HINTERNET con = ses ? WinHttpConnect(ses, host, c.nPort, 0) : nullptr;
  HINTERNET req = con ? WinHttpOpenRequest(con, L"GET", path, nullptr, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES, c.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0) : nullptr;
  if (!req) { if (con) WinHttpCloseHandle(con); if (ses) WinHttpCloseHandle(ses); *error = "WinHTTP could not open a connection"; return false; }
  std::wstring headers = L"User-Agent: ReHoMM4\r\n";
  if (!bearer.empty()) headers += L"Authorization: Bearer " + Wide(bearer) + L"\r\n";
  if (!range.empty()) headers += L"Range: bytes=" + Wide(range) + L"\r\n";
  bool ok = WinHttpSendRequest(req, headers.c_str(), DWORD(headers.size()), nullptr, 0, 0, 0) && WinHttpReceiveResponse(req, nullptr);
  DWORD code = 0, n = sizeof code; if (ok) WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &code, &n, nullptr);
  body->clear();
  while (ok) { DWORD avail = 0; if (!WinHttpQueryDataAvailable(req, &avail) || !avail) break; size_t at = body->size(); body->resize(at + avail); DWORD got = 0; if (!WinHttpReadData(req, body->data() + at, avail, &got)) { ok = false; break; } body->resize(at + got); }
  WinHttpCloseHandle(req); WinHttpCloseHandle(con); WinHttpCloseHandle(ses);
  if (!ok) { *error = "network error"; return false; } *status = int(code); return true;
}

bool CanSignInToGog() { return true; }
std::string GogLoginCode(const std::string& url) {
  ShellExecuteW(nullptr, L"open", Wide(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Sign in to GOG.com",
    "Sign in in the browser. When it finishes on a blank page, copy that page's address (Ctrl+L, Ctrl+C).", nullptr);
  for (uint64_t until = SDL_GetTicks() + 300000; SDL_GetTicks() < until; SDL_Delay(250)) {
    char* p = SDL_GetClipboardText(); std::string code = CodeIn(p ? p : ""); SDL_free(p);
    if (!code.empty()) return code;
  }
  return {};
}

bool BeginPickFiles(bool folder) {
  { std::lock_guard<std::mutex> l(g_pick_mu); g_picked.clear(); } g_pick_done = false;
  std::thread([folder] {
    std::vector<PickedFile> out; CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IFileOpenDialog* d = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&d)))) {
      FILEOPENDIALOGOPTIONS o = 0; d->GetOptions(&o); d->SetOptions(o | FOS_FORCEFILESYSTEM | (folder ? FOS_PICKFOLDERS : FOS_ALLOWMULTISELECT));
      d->SetTitle(folder ? L"Choose the installed Heroes IV folder" : L"Choose both GOG installer files");
      if (SUCCEEDED(d->Show(nullptr))) { IShellItemArray* a = nullptr; if (SUCCEEDED(d->GetResults(&a))) { DWORD n = 0; a->GetCount(&n); for (DWORD i = 0; i < n; ++i) { IShellItem* it = nullptr; PWSTR p = nullptr; if (SUCCEEDED(a->GetItemAt(i, &it)) && SUCCEEDED(it->GetDisplayName(SIGDN_FILESYSPATH, &p))) { std::string s = Utf8(p); if (folder) AddFolder(s, &out); else out.push_back({s, fs::path(s).filename().string()}); CoTaskMemFree(p); } if (it) it->Release(); } a->Release(); } }
      d->Release();
    }
    CoUninitialize(); { std::lock_guard<std::mutex> l(g_pick_mu); g_picked = std::move(out); } g_pick_done = true;
  }).detach(); return true;
}
bool PickedFiles(std::vector<PickedFile>* out) { if (!g_pick_done) return false; std::lock_guard<std::mutex> l(g_pick_mu); *out = g_picked; return true; }
void EndPickedFiles() {}
std::string LoadToken() { std::string t; if (FILE* f = fopen(TokenPath().c_str(), "rb")) { char b[4096]; t.assign(b, fread(b, 1, sizeof b, f)); fclose(f); } return t; }
void SaveToken(const std::string& token) { fs::create_directories(fs::path(TokenPath()).parent_path()); if (FILE* f = fopen(TokenPath().c_str(), "wb")) { fwrite(token.data(), 1, token.size(), f); fclose(f); } }
}  // namespace h4::install
