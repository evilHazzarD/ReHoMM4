// menu_button.h's save sync, iOS/iPadOS and the Mac app (H4_DESKTOP_APP): the
// saved games (<data>/data/h4/install/games, where the game's "games" folder is
// written) kept in step with iCloud Drive/ReHOMM4, which iCloud Drive carries to
// the player's other devices.
//
// The folder: ReHOMM4 in the folder the player picked (the picked one itself
// when it is called ReHOMM4), made when missing. The Mac app (not sandboxed)
// uses iCloud Drive/ReHOMM4 by itself, nothing to pick; on iOS the app may
// only reach a folder picked once in the Files picker, which opens in iCloud
// Drive: pick iCloud Drive itself. (Apple gives iCloud's own app containers
// only to paid developer accounts; a picked folder needs no entitlement.)
//
// Both ways, file by file, each side against how it was at the last pass (SyncOnce: after
// HommaRe's cloud_saves, MIT, Copyright (c) 2026 Ilia, github.com/livmaster-dev/HommaRe-public):
// a change is copied, a deletion is copied, a save changed on both sides keeps the newer one;
// what is overwritten or deleted is backed up first (<data>/sync-backup). A save still being
// written (changed in the last 3 s) waits for the next pass.
// Passes: at start, every 20 s, when the app goes to the background (iOS) or
// loses the focus or quits (Mac), and on "Sync now".
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#else
#import <AppKit/AppKit.h>
#endif

#include <dirent.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/stat.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "ui_text.h"
#include "menu_button.h"
#include "save_sync.h"
#include "app_ui/native.h"

namespace {

std::string g_data;              // <data>
std::mutex g_pass;               // one pass at a time (held through downloads)
std::mutex g_state;              // guards g_remote and g_status (held briefly: the sheet reads them)
NSURL* g_remote;                 // the picked folder (security-scoped) or the Mac's iCloud Drive; nil: off
std::string g_status;            // what the last pass did
std::atomic<bool> g_wake{false};

std::string BookmarkPath() { return g_data + "/icloud.bookmark"; }
std::string OffPath() { return g_data + "/icloud.off"; }  // the Mac: turned off by the player

NSString* const kFolder = @"ReHOMM4";

// The folder synced in `root` (the picked one): itself when it is ReHOMM4, else its ReHOMM4.
NSURL* SyncFolder(NSURL* root) {
  if ([root.lastPathComponent caseInsensitiveCompare:kFolder] == NSOrderedSame) return root;
  return [root URLByAppendingPathComponent:kFolder isDirectory:YES];
}

// The Mac's iCloud Drive (~/Library/Mobile Documents/com~apple~CloudDocs), nil without it.
NSURL* MacICloudDrive() {
#if TARGET_OS_IOS
  return nil;
#else
  NSURL* u = [NSFileManager.defaultManager.homeDirectoryForCurrentUser
      URLByAppendingPathComponent:@"Library/Mobile Documents/com~apple~CloudDocs"
                      isDirectory:YES];
  BOOL dir = NO;
  return [NSFileManager.defaultManager fileExistsAtPath:u.path isDirectory:&dir] && dir ? u : nil;
#endif
}

// The install overlay (<data>/data/h4/install: what the game writes into its folder) and its
// "games" folder (found without case), where the game's save dialog writes and received saves go.
NSString* LocalInstall() { return @((g_data + "/data/h4/install").c_str()); }
NSString* LocalGames() {
  const std::string install = g_data + "/data/h4/install";
  std::string name = "games";
  if (DIR* d = opendir(install.c_str())) {
    while (dirent* e = readdir(d))
      if (!strcasecmp(e->d_name, "games")) name = e->d_name;
    closedir(d);
  }
  NSString* p = @((install + "/" + name).c_str());
  [NSFileManager.defaultManager createDirectoryAtPath:p
                          withIntermediateDirectories:YES
                                           attributes:nil
                                                error:nil];
  return p;
}

NSURL* ResolveBookmark() {
  NSData* d = [NSData dataWithContentsOfFile:@(BookmarkPath().c_str())];
  if (!d) return nil;
  BOOL stale = NO;
  NSURL* u = [NSURL URLByResolvingBookmarkData:d options:0 relativeToURL:nil bookmarkDataIsStale:&stale error:nil];
  if (u && stale && [u startAccessingSecurityScopedResource]) {
    NSData* fresh = [u bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:nil];
    [fresh writeToFile:@(BookmarkPath().c_str()) atomically:YES];
    [u stopAccessingSecurityScopedResource];
  }
  return u;
}

// What is synced at start: the picked folder; on the Mac without one, iCloud Drive (unless
// the player turned it off).
NSURL* InitialRemote() {
  if (NSURL* u = ResolveBookmark()) return u;
  if (access(OffPath().c_str(), F_OK) == 0) return nil;
  return MacICloudDrive();
}


// ---- One pass, after HommaRe's cloud_saves (MIT, github.com/livmaster-dev/HommaRe-public):
// each side compared with how it looked at the last pass (a state file), so a save changed on
// one side is copied, a save deleted on one side is deleted on the other, and a save changed on
// both (or two different saves with one name when two devices first meet) keeps the newer one.
// Whatever a pass overwrites or deletes, on either side, is first copied to
// <data>/sync-backup/<date time>/ (the newest 10 kept). A side whose synced saves are all gone
// lost its folder (or had it emptied by hand): the other side's saves go back to it, nothing is
// deleted. The synced folder's files are only touched through NSFileCoordinator; one not
// downloaded yet (an iCloud placeholder) is asked for and waited on a little, else left for
// the next pass, as is a save changed in the last 3 s (the game may be writing it).

struct Stamp {
  int64_t size = -1, mtime_ns = 0;
  bool operator==(const Stamp& o) const { return size == o.size && mtime_ns == o.mtime_ns; }
  bool operator!=(const Stamp& o) const { return !(*this == o); }
};
struct Item {
  std::string path;  // where it is
  Stamp stamp;
  bool pending = false;  // the synced folder's, not on this device yet
};
using Listing = std::map<std::string, Item>;  // name -> item
struct Seen {
  Stamp local, cloud;
};
using State = std::map<std::string, Seen>;

constexpr int64_t kBusyNs = 3'000'000'000ll, kSameTimeNs = 2'000'000'000ll;
constexpr size_t kBackupsKept = 10;

std::string StatePath() { return g_data + "/sync-state.tsv"; }
std::string BackupRoot() { return g_data + "/sync-backup"; }

bool StatPath(const std::string& path, Stamp* out) {
  struct stat st {};
  if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
  out->size = st.st_size;
  out->mtime_ns = int64_t(st.st_mtimespec.tv_sec) * 1'000'000'000ll + st.st_mtimespec.tv_nsec;
  return true;
}

void SetMtime(const std::string& path, int64_t ns) {
  struct timespec t[2];
  t[0].tv_sec = 0;
  t[0].tv_nsec = UTIME_OMIT;
  t[1].tv_sec = time_t(ns / 1'000'000'000ll);
  t[1].tv_nsec = long(ns % 1'000'000'000ll);
  utimensat(AT_FDCWD, path.c_str(), t, 0);
}

std::string Dir(const std::string& path) {
  const size_t k = path.rfind('/');
  return k == std::string::npos ? "." : path.substr(0, k);
}
std::string Base(const std::string& path) {
  const size_t k = path.rfind('/');
  return k == std::string::npos ? path : path.substr(k + 1);
}

// A copy keeping `src`'s modification time, put in place by a rename (a hidden temporary beside
// `dst`: the listings skip dot files).
bool CopyKeepingTime(const std::string& src, const std::string& dst) {
  Stamp st;
  if (!StatPath(src, &st)) return false;
  [NSFileManager.defaultManager createDirectoryAtPath:@(Dir(dst).c_str()) withIntermediateDirectories:YES
                                           attributes:nil error:nil];
  const std::string tmp = Dir(dst) + "/." + Base(dst) + ".sync-tmp";
  std::remove(tmp.c_str());
  NSData* d = [NSData dataWithContentsOfFile:@(src.c_str())];
  if (!d || ![d writeToFile:@(tmp.c_str()) atomically:NO]) {
    std::remove(tmp.c_str());
    return false;
  }
  SetMtime(tmp, st.mtime_ns);
  if (rename(tmp.c_str(), dst.c_str()) != 0) {
    std::remove(tmp.c_str());
    return false;
  }
  return true;
}

bool SameBytes(const std::string& a, const std::string& b) {
  NSData* x = [NSData dataWithContentsOfFile:@(a.c_str())];
  NSData* y = [NSData dataWithContentsOfFile:@(b.c_str())];
  return x && y && [x isEqualToData:y];
}

State LoadState(const std::string& cloud) {
  State s;
  std::ifstream f(StatePath(), std::ios::binary);
  std::string line;
  if (!std::getline(f, line) || line != "#cloud\t" + cloud) return s;  // another folder's: none
  while (std::getline(f, line)) {
    std::vector<std::string> c;
    for (size_t at = 0; at <= line.size();) {
      size_t end = line.find('\t', at);
      if (end == std::string::npos) end = line.size();
      c.push_back(line.substr(at, end - at));
      at = end + 1;
    }
    if (c.size() != 5 || c[0].empty()) continue;
    s[c[0]] = {{atoll(c[1].c_str()), atoll(c[2].c_str())}, {atoll(c[3].c_str()), atoll(c[4].c_str())}};
  }
  return s;
}

void SaveState(const std::string& cloud, const State& s) {
  const std::string tmp = StatePath() + ".tmp";
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    f << "#cloud\t" << cloud << '\n';
    for (const auto& [n, x] : s)
      f << n << '\t' << x.local.size << '\t' << x.local.mtime_ns << '\t' << x.cloud.size << '\t' << x.cloud.mtime_ns
        << '\n';
    if (!f) return;
  }
  rename(tmp.c_str(), StatePath().c_str());
}

// This pass's backup folder, made when something is first put in it; the newest kBackupsKept stay.
class Backup {
 public:
  Backup() {
    const time_t t = time(nullptr);
    struct tm tm {};
    localtime_r(&t, &tm);
    char name[32];
    strftime(name, sizeof name, "%Y-%m-%d %H%M%S", &tm);
    dir_ = BackupRoot() + "/" + name;
  }
  std::string Path(const std::string& name) {
    used_ = true;
    return dir_ + "/" + name;
  }
  void Prune() {
    if (!used_) return;
    NSArray<NSString*>* all = [[NSFileManager.defaultManager contentsOfDirectoryAtPath:@(BackupRoot().c_str())
                                                                                  error:nil]
        sortedArrayUsingSelector:@selector(compare:)];
    for (NSUInteger i = 0; i + kBackupsKept < all.count; i++)
      [NSFileManager.defaultManager removeItemAtPath:[@(BackupRoot().c_str()) stringByAppendingPathComponent:all[i]]
                                               error:nil];
  }

 private:
  std::string dir_;
  bool used_ = false;
};

// The local saves: games/ and the saves the game wrote beside it (its save dialog may start
// there; *.h4s), the newer one when both have a name. Hidden files are skipped.
Listing LocalListing() {
  Listing out;
  auto scan = [&](NSString* dir, bool only_saves) {
    for (NSString* n in [NSFileManager.defaultManager contentsOfDirectoryAtPath:dir error:nil]) {
      if ([n hasPrefix:@"."] || (only_saves && ![n.lowercaseString hasSuffix:@".h4s"])) continue;
      Item it;
      it.path = [dir stringByAppendingPathComponent:n].fileSystemRepresentation;
      if (!StatPath(it.path, &it.stamp)) continue;
      auto have = out.find(n.UTF8String);
      if (have == out.end() || it.stamp.mtime_ns > have->second.stamp.mtime_ns) out[n.UTF8String] = it;
    }
  };
  scan(LocalGames(), false);
  scan(LocalInstall(), true);
  return out;
}

// The synced folder's files (coordinated; metadata only, nothing downloaded). iCloud keeps a file
// not on this device as ".<name>.icloud" (or as the file with no data): pending, its download
// started. False when the folder cannot be read.
bool CloudListingOnce(NSURL* remote, Listing* out, NSError** err) {
  __block bool ok = true;
  NSFileManager* fm = NSFileManager.defaultManager;
  [[[NSFileCoordinator alloc] initWithFilePresenter:nil]
      coordinateReadingItemAtURL:remote
                         options:NSFileCoordinatorReadingImmediatelyAvailableMetadataOnly
                           error:err
                      byAccessor:^(NSURL* at) {
                        NSError* e = nil;
                        NSArray<NSURL*>* items = [fm contentsOfDirectoryAtURL:at
                                                   includingPropertiesForKeys:@[ NSURLIsRegularFileKey ]
                                                                      options:0
                                                                        error:&e];
                        if (!items) {
                          ok = false;
                          return;
                        }
                        for (NSURL* u in items) {
                          NSString* n = u.lastPathComponent;
                          if ([n hasPrefix:@"."]) {
                            if ([n hasSuffix:@".icloud"] && n.length > 8) {
                              NSString* real = [n substringWithRange:NSMakeRange(1, n.length - 8)];
                              NSURL* ru = [at URLByAppendingPathComponent:real];
                              [fm startDownloadingUbiquitousItemAtURL:ru error:nil];
                              Item it;
                              it.path = ru.path.fileSystemRepresentation;
                              it.pending = true;
                              out->emplace(real.UTF8String, it);  // the file itself, if listed, wins
                            }
                            continue;
                          }
                          NSNumber* reg = nil;
                          [u getResourceValue:&reg forKey:NSURLIsRegularFileKey error:nil];
                          if (!reg.boolValue) continue;
                          Item it;
                          it.path = u.path.fileSystemRepresentation;
                          NSNumber* ubiq = nil;
                          NSString* status = nil;
                          [u getResourceValue:&ubiq forKey:NSURLIsUbiquitousItemKey error:nil];
                          if (ubiq.boolValue)
                            [u getResourceValue:&status forKey:NSURLUbiquitousItemDownloadingStatusKey error:nil];
                          if ([status isEqualToString:NSURLUbiquitousItemDownloadingStatusNotDownloaded]) {
                            it.pending = true;
                            [fm startDownloadingUbiquitousItemAtURL:u error:nil];
                          } else if (!StatPath(it.path, &it.stamp)) {
                            continue;
                          }
                          (*out)[n.UTF8String] = it;
                        }
                      }];
  return ok && !(err && *err);
}

// The listing, waiting up to 8 s for the downloads it started.
bool CloudListing(NSURL* remote, Listing* out, NSError** err) {
  const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(8);
  for (;;) {
    Listing l;
    if (!CloudListingOnce(remote, &l, err)) return false;
    bool waiting = false;
    for (const auto& kv : l) waiting |= kv.second.pending;
    if (!waiting || std::chrono::steady_clock::now() >= until) {
      *out = std::move(l);
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

NSURL* UrlOf(const std::string& path) { return [NSURL fileURLWithPath:@(path.c_str())]; }

// The synced folder's file `src` copied to the local `dst` (downloaded first when needed).
bool CloudRead(const std::string& src, const std::string& dst) {
  __block bool ok = false;
  NSError* e = nil;
  [[[NSFileCoordinator alloc] initWithFilePresenter:nil]
      coordinateReadingItemAtURL:UrlOf(src)
                         options:0
                           error:&e
                      byAccessor:^(NSURL* u) { ok = CopyKeepingTime(u.path.fileSystemRepresentation, dst); }];
  return ok && !e;
}

// The local `src` put at `dst` in the synced folder; `*now`: how it is there afterwards.
bool CloudWrite(const std::string& src, const std::string& dst, Stamp* now) {
  __block bool ok = false;
  NSError* e = nil;
  [[[NSFileCoordinator alloc] initWithFilePresenter:nil]
      coordinateWritingItemAtURL:UrlOf(dst)
                         options:NSFileCoordinatorWritingForReplacing
                           error:&e
                      byAccessor:^(NSURL* u) {
                        const std::string to = u.path.fileSystemRepresentation;
                        ok = CopyKeepingTime(src, to) && StatPath(to, now);
                      }];
  return ok && !e;
}

bool CloudRemove(const std::string& path) {
  __block bool ok = false;
  NSError* e = nil;
  [[[NSFileCoordinator alloc] initWithFilePresenter:nil]
      coordinateWritingItemAtURL:UrlOf(path)
                         options:NSFileCoordinatorWritingForDeleting
                           error:&e
                      byAccessor:^(NSURL* u) {
                        NSError* gone = nil;
                        ok = [NSFileManager.defaultManager removeItemAtURL:u error:&gone] ||
                             gone.code == NSFileNoSuchFileError;
                      }];
  return ok && !e;
}

// iCloud's side of a file in the synced folder: uploaded, or why not ("" when it is not iCloud's,
// or uploaded). A full iCloud storage is said as such (its error only says the account cannot
// be reached: "Quota exceeded" is in an underlying error).
bool NotUploaded(NSURL* u, std::string* why) {
  NSNumber* ubiq = nil;
  [u getResourceValue:&ubiq forKey:NSURLIsUbiquitousItemKey error:nil];
  if (!ubiq.boolValue) return false;
  NSNumber* up = nil;
  [u getResourceValue:&up forKey:NSURLUbiquitousItemIsUploadedKey error:nil];
  if (up.boolValue) return false;
  NSError* e = nil;
  [u getResourceValue:&e forKey:NSURLUbiquitousItemUploadingErrorKey error:nil];
  if (e && why->empty()) {
    const bool quota = [e.description rangeOfString:@"quota" options:NSCaseInsensitiveSearch].location != NSNotFound;
    *why = quota ? "iCloud storage is full" : e.localizedDescription.UTF8String;
  }
  return true;
}

void SetStatus(const std::string& s) {
  std::lock_guard<std::mutex> lock(g_state);
  g_status = s;
}

// One pass (the pass thread, or a last one at quitting). Returns what it did.
std::string SyncOnce() {
  std::lock_guard<std::mutex> pass(g_pass);
  NSURL* root;
  {
    std::lock_guard<std::mutex> lock(g_state);
    root = g_remote;
  }
  if (!root) return "";
  @autoreleasepool {
    const BOOL access = [root startAccessingSecurityScopedResource];
    NSURL* remote = SyncFolder(root);
    const std::string cloud = remote.path.fileSystemRepresentation;
    NSError* err = nil;
    if (![NSFileManager.defaultManager fileExistsAtPath:remote.path])  // ReHOMM4, made the first time
      [[[NSFileCoordinator alloc] initWithFilePresenter:nil]
          coordinateWritingItemAtURL:remote options:0 error:&err byAccessor:^(NSURL* u) {
            [NSFileManager.defaultManager createDirectoryAtURL:u withIntermediateDirectories:YES attributes:nil
                                                         error:nil];
          }];
    err = nil;
    Listing there;
    if (!CloudListing(remote, &there, &err)) {
      if (access) [root stopAccessingSecurityScopedResource];
      const std::string s = "the iCloud folder cannot be read" +
                            (err ? std::string(" (") + err.localizedDescription.UTF8String + ")" : "");
      SetStatus(s);
      return s;
    }
    const std::string games = LocalGames().fileSystemRepresentation;
    const Listing here = LocalListing();
    const State state = LoadState(cloud);
    // A side with none of the saves it had lost them (or its folder): it gets them back.
    const bool local_lost = !state.empty() && here.empty();
    const bool cloud_lost = !state.empty() && there.empty();
    Backup backup;
    std::set<std::string> names;
    for (const auto& kv : here) names.insert(kv.first);
    for (const auto& kv : there) names.insert(kv.first);
    for (const auto& kv : state) names.insert(kv.first);
    State next;
    int up = 0, down = 0, gone_here = 0, gone_there = 0, conflicts = 0, failed = 0, waiting = 0;
    const int64_t now = int64_t([NSDate.date timeIntervalSince1970] * 1e9);
    for (const std::string& n : names) {
      const auto li = here.find(n);
      const auto ci = there.find(n);
      const auto si = state.find(n);
      const Item* l = li != here.end() ? &li->second : nullptr;
      const Item* c = ci != there.end() ? &ci->second : nullptr;
      const Seen* s = si != state.end() ? &si->second : nullptr;
      const std::string lpath = l ? l->path : games + "/" + n, cpath = cloud + "/" + n;
      const bool busy = l && now - l->stamp.mtime_ns < kBusyNs && l->stamp.mtime_ns <= now;
      if ((c && c->pending) || busy) {  // next pass
        waiting++;
        if (s) next[n] = *s;
        continue;
      }
      auto push = [&] {
        Seen x;
        x.local = l->stamp;
        if (!CloudWrite(lpath, cpath, &x.cloud)) return false;
        next[n] = x;
        up++;
        return true;
      };
      auto pull = [&] {
        Seen x;
        x.cloud = c->stamp;
        if (!CloudRead(cpath, lpath) || !StatPath(lpath, &x.local)) return false;
        next[n] = x;
        down++;
        return true;
      };
      const bool local_changed = l ? (!s || l->stamp != s->local) : s != nullptr;
      const bool cloud_changed = c ? (!s || c->stamp != s->cloud) : s != nullptr;
      bool ok = true;
      if (l && c) {
        if (s && !local_changed && !cloud_changed) {
          next[n] = {l->stamp, c->stamp};
        } else if (s && local_changed && !cloud_changed) {
          ok = push();
        } else if (s && cloud_changed && !local_changed) {
          ok = pull();
        } else {  // both changed, or both had it before their first pass together
          bool same = false;
          if (l->stamp.size == c->stamp.size) {
            same = llabs(l->stamp.mtime_ns - c->stamp.mtime_ns) < kSameTimeNs;
            if (!same) {
              const std::string probe = Dir(lpath) + "/." + n + ".sync-cmp";
              same = CloudRead(cpath, probe) && SameBytes(probe, lpath);
              std::remove(probe.c_str());
            }
          }
          if (same) {
            next[n] = {l->stamp, c->stamp};
          } else {
            conflicts++;
            ok = l->stamp.mtime_ns >= c->stamp.mtime_ns ? CloudRead(cpath, backup.Path(n)) && push()
                                                        : CopyKeepingTime(lpath, backup.Path(n)) && pull();
          }
        }
      } else if (l) {
        if (!s || local_changed || cloud_lost) {
          ok = push();
        } else {  // deleted in the synced folder (another device), unchanged here
          ok = CopyKeepingTime(lpath, backup.Path(n)) && std::remove(lpath.c_str()) == 0;
          if (ok) gone_here++;
        }
      } else if (c) {
        if (!s || cloud_changed || local_lost) {
          ok = pull();
        } else {  // deleted here (the game's own delete), unchanged there
          ok = CloudRead(cpath, backup.Path(n)) && CloudRemove(cpath);
          if (ok) gone_there++;
        }
      }
      // else: gone on both sides, and from the state
      if (!ok) {
        failed++;
        std::fprintf(stderr, "h4: save sync: %s could not be synced\n", n.c_str());
        if (s) next[n] = *s;  // tried again next pass
      }
    }
    SaveState(cloud, next);
    backup.Prune();
    // What the folder holds now, and what of it iCloud does not have yet.
    int in_folder = 0, stuck = 0;
    std::string why;
    Listing after;
    CloudListingOnce(remote, &after, nullptr);
    for (const auto& kv : after) {
      in_folder++;
      if (!kv.second.pending && NotUploaded(UrlOf(kv.second.path), &why)) stuck++;
    }
    if (access) [root stopAccessingSecurityScopedResource];
    NSDateFormatter* f = [NSDateFormatter new];
    f.dateFormat = @"HH:mm";
    std::string extra;
    if (gone_here || gone_there)
      extra += ", " + std::to_string(gone_here + gone_there) + " deleted (as on the other side)";
    if (conflicts) extra += ", " + std::to_string(conflicts) + " changed on both: the newer kept";
    if (failed) extra += ", " + std::to_string(failed) + " failed";
    if (waiting) extra += ", " + std::to_string(waiting) + " waiting";
    if (stuck)
      extra += "\n⚠️ " + std::to_string(stuck) + " not in iCloud yet" + (why.empty() ? "" : ": " + why) +
               " (other devices don't see them)";
    char b[600];
    std::snprintf(b, sizeof b, "%s: %d save(s) in the folder, %d here; %d sent, %d received%s",
                  [f stringFromDate:NSDate.date].UTF8String, in_folder, int(LocalListing().size()), up, down,
                  extra.c_str());
    static int last_stuck = -1;
    if (up || down || gone_here || gone_there || conflicts || failed || stuck != last_stuck)
      std::fprintf(stderr, "h4: save sync %s\n", b);
    last_stuck = stuck;
    SetStatus(b);
    return b;
  }
}

void SyncSoon() { g_wake = true; }

// The synced folder as the player knows it ("iCloud Drive/ReHOMM4"), "" when off.
std::string FolderName() {
  std::lock_guard<std::mutex> lock(g_state);
  if (!g_remote) return "";
  NSURL* f = SyncFolder(g_remote);
  NSString* parent = f.URLByDeletingLastPathComponent.lastPathComponent;
  if ([parent isEqualToString:@"com~apple~CloudDocs"] || [parent isEqualToString:@"Mobile Documents"])
    parent = @"iCloud Drive";
  return std::string([parent stringByAppendingPathComponent:f.lastPathComponent].UTF8String);
}

// A folder the player picked: remembered (bookmark), synced from now on.
void UsePicked(NSURL* u) {
  if (!u) return;
  const BOOL access = [u startAccessingSecurityScopedResource];
  NSData* bm = [u bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:nil];
  if (access) [u stopAccessingSecurityScopedResource];
  if (!bm || ![bm writeToFile:@(BookmarkPath().c_str()) atomically:YES]) return;
  std::remove(OffPath().c_str());
  {
    std::lock_guard<std::mutex> lock(g_state);
    g_remote = ResolveBookmark();
  }
  SyncSoon();
}

void StopSync() {
  std::lock_guard<std::mutex> lock(g_state);
  g_remote = nil;
  g_status.clear();
  std::remove(BookmarkPath().c_str());
  if (FILE* f = std::fopen(OffPath().c_str(), "w")) std::fclose(f);  // the Mac: stays off
}

// The Mac: an rclone remote (sync.conf's rclone=) kept in step with the synced folder, around
// every third pass and at quitting (save_sync.cpp). Not on iOS (no rclone there).
std::string g_rclone;
std::string RemotePass(bool remote) {
#if TARGET_OS_IOS
  return SyncOnce();
#else
  NSURL* root;
  {
    std::lock_guard<std::mutex> lock(g_state);
    root = g_remote;
  }
  if (g_rclone.empty() || !remote || !root) return SyncOnce();
  const std::string folder = SyncFolder(root).path.UTF8String;
  [NSFileManager.defaultManager createDirectoryAtPath:@(folder.c_str()) withIntermediateDirectories:YES
                                           attributes:nil error:nil];
  const std::string down = h4::RcloneAround(g_rclone, folder, true);
  std::string st = SyncOnce();
  const std::string up = h4::RcloneAround(g_rclone, folder, false);
  const std::string& e = down.empty() ? up : down;
  st += e.empty() ? ", rclone ok" : ", " + e;
  SetStatus(st);
  return st;
#endif
}

std::atomic<bool> g_first_done{false};

// The passes, on a thread of their own; the first one waited for up to 3 s (the game is about to
// read its saves: another device's newer ones should be here by then).
void PassLoop() {
  std::thread([] {
    for (unsigned n = 0;; n++) {
      RemotePass(n % 3 == 0);
      g_first_done = true;
      for (int i = 0; i < 200 && !g_wake.exchange(false); i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }).detach();
  for (int i = 0; i < 30 && g_remote && !g_first_done; i++)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

}  // namespace

#if TARGET_OS_IOS

@interface H4SyncPicker : NSObject <UIDocumentPickerDelegate>
@end
@implementation H4SyncPicker
- (void)documentPicker:(UIDocumentPickerViewController*)c didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
  UsePicked(urls.firstObject);
}
@end

namespace {
// The Files picker for the folder to sync with, opening in iCloud Drive (its provider's root); on
// the main thread, from any.
void PickFolderSoon() {
  dispatch_async(dispatch_get_main_queue(), ^{
    UIViewController* top = nil;
    for (UIScene* sc in UIApplication.sharedApplication.connectedScenes)
      if ([sc isKindOfClass:UIWindowScene.class])
        for (UIWindow* w in ((UIWindowScene*)sc).windows)
          if (w.isKeyWindow && w.rootViewController) top = w.rootViewController;
    while (top.presentedViewController) top = top.presentedViewController;
    if (!top) return;
    static H4SyncPicker* del = [H4SyncPicker new];
    UIDocumentPickerViewController* p =
        [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[ UTTypeFolder ]];
    p.delegate = del;
    p.directoryURL = [NSFileManager.defaultManager URLForUbiquityContainerIdentifier:nil].URLByDeletingLastPathComponent;
    [top presentViewController:p animated:YES completion:nil];
  });
}
}  // namespace

namespace h4 {

void StartSaveSync(const std::string& data_dir) {
  g_data = data_dir;
  g_remote = InitialRemote();
  [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidEnterBackgroundNotification
                                                  object:nil
                                                   queue:nil
                                              usingBlock:^(NSNotification*) {
                                                // A last pass before iOS suspends the app.
                                                UIApplication* app = UIApplication.sharedApplication;
                                                __block UIBackgroundTaskIdentifier t =
                                                    [app beginBackgroundTaskWithExpirationHandler:^{
                                                      [app endBackgroundTask:t];
                                                    }];
                                                const UIBackgroundTaskIdentifier task = t;
                                                std::thread([app, task] {
                                                  SyncOnce();
                                                  dispatch_async(dispatch_get_main_queue(), ^{
                                                    [app endBackgroundTask:task];
                                                  });
                                                }).detach();
                                              }];
  PassLoop();
}

bool ShowSyncButton() { return !FolderName().empty(); }
std::string SavesFolder() { return g_data.empty() ? "" : std::string(LocalGames().UTF8String); }
std::string SyncedFolder() {
  std::lock_guard<std::mutex> lock(g_state);
  return g_remote ? std::string(SyncFolder(g_remote).path.UTF8String) : "";
}

bool AskSaveSyncFolder() {
  const std::string asked = g_data + "/icloud.asked";
  if (g_data.empty() || !FolderName().empty() || access(asked.c_str(), F_OK) == 0 ||
      access(OffPath().c_str(), F_OK) == 0)
    return false;
  if (FILE* f = std::fopen(asked.c_str(), "w")) std::fclose(f);
  return true;
}

bool SetSaveSync(bool on, void*) {
  if (!on) StopSync();
  else if (FolderName().empty()) PickFolderSoon();  // on once picked
  return ShowSyncButton();
}

}  // namespace h4

// app_ui/native.h's save sync on iOS: the Files picker for the folder (iCloud Drive at first).
namespace h4::native {
SyncPanel SyncPanelFor(const std::string& lang) {
  const std::string folder = FolderName();
  SyncPanel p;
  p.title = h4::TrIn(lang, "Saves in iCloud");
  if (folder.empty()) {
    p.text = h4::TrIn(lang, "Open iCloud Drive and tap Open: the game makes a ReHOMM4 folder there and copies "
                            "saved games there and back, on each of your devices with this Apple ID.");
  } else {
    std::lock_guard<std::mutex> lock(g_state);
    p.text = h4::TrIn(lang, "Folder: {1}", {folder}) + "\n" + g_status;
  }
  p.buttons.push_back({h4::TrIn(lang, folder.empty() ? "Choose a folder…" : "Choose another folder…"), [] { PickFolderSoon(); }});
  if (!folder.empty()) {
    p.buttons.push_back({h4::TrIn(lang, "Sync now"), [] { SyncSoon(); }});
    p.buttons.push_back({h4::TrIn(lang, "Stop syncing"), [] { StopSync(); }});
  }
  return p;
}
}  // namespace h4::native

#else  // the Mac app

namespace h4 {

void StartSaveSync(const std::string& data_dir) {
  g_data = data_dir;
  g_remote = InitialRemote();
  g_rclone = h4::ReadSyncConfig(data_dir).rclone;
  if (g_remote)
    std::fprintf(stderr, "h4: save sync with %s%s%s\n", FolderName().c_str(), g_rclone.empty() ? "" : " and rclone ",
                 g_rclone.c_str());
  NSNotificationCenter* nc = NSNotificationCenter.defaultCenter;
  [nc addObserverForName:NSApplicationWillResignActiveNotification object:nil queue:nil
              usingBlock:^(NSNotification*) { SyncSoon(); }];
  [nc addObserverForName:NSApplicationWillTerminateNotification object:nil queue:nil
              usingBlock:^(NSNotification*) { RemotePass(true); }];  // a last pass before quitting
  PassLoop();
}

bool ShowSyncButton() { return !FolderName().empty(); }
std::string SavesFolder() { return g_data.empty() ? "" : std::string(LocalGames().UTF8String); }
std::string SyncedFolder() {
  std::lock_guard<std::mutex> lock(g_state);
  return g_remote ? std::string(SyncFolder(g_remote).path.UTF8String) : "";
}
bool AskSaveSyncFolder() { return false; }  // on by itself (iCloud Drive, unless turned off)

// On: iCloud Drive/ReHOMM4 (a folder picked before is forgotten); off: StopSync (and stays off).
bool SetSaveSync(bool on, void*) {
  if (!on) {
    StopSync();
  } else if (FolderName().empty() && MacICloudDrive()) {
    std::remove(OffPath().c_str());
    std::remove(BookmarkPath().c_str());
    {
      std::lock_guard<std::mutex> lock(g_state);
      g_remote = MacICloudDrive();
    }
    SyncSoon();
  }
  return ShowSyncButton();
}

}  // namespace h4

// app_ui/native.h's save sync on the Mac: iCloud Drive/ReHOMM4, or a folder picked (NSOpenPanel).
namespace h4::native {
SyncPanel SyncPanelFor(const std::string& lang) {
  const std::string folder = FolderName();
  SyncPanel p;
  p.title = h4::TrIn(lang, "Saves in iCloud");
  if (folder.empty()) {
    p.text = MacICloudDrive()
                 ? h4::TrIn(lang, "Syncing is off. Turn it on and the game copies saved games to iCloud Drive/ReHOMM4 "
                                  "and back, on each of your devices with this Apple ID.")
                 : h4::TrIn(lang, "iCloud Drive is off on this Mac (System Settings → Apple ID → iCloud).");
  } else {
    std::lock_guard<std::mutex> lock(g_state);
    p.text = h4::TrIn(lang, "Folder: {1}", {folder}) + "\n" + g_status;
  }
  if (!folder.empty()) p.buttons.push_back({h4::TrIn(lang, "Sync now"), [] { SyncSoon(); }});
  else if (MacICloudDrive()) p.buttons.push_back({h4::TrIn(lang, "Turn on"), [] { SetSaveSync(true, nullptr); }});
  const std::string choose = h4::TrIn(lang, "Choose"), what = h4::TrIn(lang, "The folder to hold ReHOMM4 (or ReHOMM4 itself)");
  p.buttons.push_back({h4::TrIn(lang, "Choose another folder…"), [choose, what] {
                         // The system's folder picker (a window of its own; ReHOMM4 is made inside what
                         // is picked, or it is ReHOMM4), on the main thread.
                         dispatch_async(dispatch_get_main_queue(), ^{
                           NSOpenPanel* o = [NSOpenPanel openPanel];
                           o.canChooseFiles = NO;
                           o.canChooseDirectories = YES;
                           o.canCreateDirectories = YES;
                           o.prompt = @(choose.c_str());
                           o.message = @(what.c_str());
                           o.directoryURL = MacICloudDrive();
                           // A sheet on the game's window (a modal loop would stop the game's frames).
                           NSWindow* w = NSApp.keyWindow ?: NSApp.mainWindow;
                           if (!w) return;
                           [o beginSheetModalForWindow:w
                                     completionHandler:^(NSModalResponse r) {
                                       if (r == NSModalResponseOK) UsePicked(o.URL);
                                     }];
                         });
                       }});
  if (!folder.empty()) p.buttons.push_back({h4::TrIn(lang, "Stop syncing"), [] { StopSync(); }});
  return p;
}
}  // namespace h4::native

#endif

// app_ui/native.h: what both have.
namespace h4::native {
bool HasSyncChoice() { return true; }
bool SyncOn() { return h4::ShowSyncButton(); }
void SetSync(bool on) { h4::SetSaveSync(on, nullptr); }
std::string SavesFolder() { return h4::SavesFolder(); }
std::string SyncedFolder() { return h4::SyncedFolder(); }
void DownloadApk(const std::string&, const std::string&) {}
}  // namespace h4::native
