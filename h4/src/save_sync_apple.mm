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
// Both ways, file by file: the newer copy wins (modification times, copied
// along), a file only one side has is copied over, nothing is deleted. A save
// still being written (changed in the last 3 s) waits for the next pass.
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
#include <strings.h>
#include <sys/stat.h>

#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "menu_button.h"
#include "save_sync.h"

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

NSDate* ModDate(NSURL* u) {
  NSDate* d = nil;
  [u getResourceValue:&d forKey:NSURLContentModificationDateKey error:nil];
  return d;
}

void SetModDate(NSURL* u, NSDate* d) {
  if (d) [NSFileManager.defaultManager setAttributes:@{NSFileModificationDate : d} ofItemAtPath:u.path error:nil];
}

// The regular files of a folder: name -> URL. iCloud keeps a file not downloaded on this device
// as a placeholder (".name.icloud"): listed under its real name too, its URL the real one (a
// coordinated read of it downloads it), and named in `pending`. `suffix`: only those names.
NSMutableDictionary<NSString*, NSURL*>* Files(NSURL* dir, NSMutableSet<NSString*>* pending,
                                              NSString* suffix = nil) {
  NSMutableDictionary<NSString*, NSURL*>* out = [NSMutableDictionary new];
  NSArray<NSURL*>* items = [NSFileManager.defaultManager
        contentsOfDirectoryAtURL:dir
      includingPropertiesForKeys:@[ NSURLIsRegularFileKey, NSURLContentModificationDateKey ]
                         options:0
                           error:nil];
  for (NSURL* u in items) {
    NSString* n = u.lastPathComponent;
    if ([n hasPrefix:@"."]) {
      if (pending && [n hasSuffix:@".icloud"] && n.length > 8) {
        NSString* real = [n substringWithRange:NSMakeRange(1, n.length - 8)];
        if (suffix && ![real.lowercaseString hasSuffix:suffix]) continue;
        NSURL* ru = [dir URLByAppendingPathComponent:real];
        [NSFileManager.defaultManager startDownloadingUbiquitousItemAtURL:ru error:nil];
        out[real] = ru;
        [pending addObject:real];
      }
      continue;
    }
    if (suffix && ![n.lowercaseString hasSuffix:suffix]) continue;
    NSNumber* reg = nil;
    [u getResourceValue:&reg forKey:NSURLIsRegularFileKey error:nil];
    if (reg.boolValue) out[n] = u;
  }
  return out;
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
    NSURL* local = [NSURL fileURLWithPath:LocalGames() isDirectory:YES];
    NSFileCoordinator* co = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
    NSMutableSet<NSString*>* pending = [NSMutableSet new];
    __block NSDictionary<NSString*, NSURL*>* there = nil;
    NSError* err = nil;
    if (![NSFileManager.defaultManager fileExistsAtPath:remote.path])  // ReHOMM4, made the first time
      [co coordinateWritingItemAtURL:remote options:0 error:&err byAccessor:^(NSURL* u) {
        [NSFileManager.defaultManager createDirectoryAtURL:u withIntermediateDirectories:YES attributes:nil error:nil];
      }];
    // The listing needs only what is known here (no download of the folder's files).
    [co coordinateReadingItemAtURL:remote
                           options:NSFileCoordinatorReadingImmediatelyAvailableMetadataOnly
                             error:&err
                        byAccessor:^(NSURL* u) { there = Files(u, pending); }];
    if (!there) {
      if (access) [root stopAccessingSecurityScopedResource];
      const std::string s = "the iCloud folder cannot be read" +
                            (err ? std::string(" (") + err.localizedDescription.UTF8String + ")" : "");
      SetStatus(s);
      return s;
    }
    // Here: games/, and the saves the game wrote beside it (its save dialog may start there);
    // the newer one when both have a name.
    NSMutableDictionary<NSString*, NSURL*>* here = Files(local, nil);
    NSDictionary<NSString*, NSURL*>* beside =
        Files([NSURL fileURLWithPath:LocalInstall() isDirectory:YES], nil, @".h4s");
    for (NSString* n in beside) {
      NSURL* u = beside[n];
      if (!here[n] || [ModDate(u) compare:ModDate(here[n])] == NSOrderedDescending) here[n] = u;
    }
    NSMutableSet<NSString*>* names = [NSMutableSet setWithArray:there.allKeys];
    [names addObjectsFromArray:here.allKeys];
    int up = 0, down = 0, failed = 0, waiting = 0;
    NSDate* now = NSDate.date;
    for (NSString* n in names) {
      NSURL* ru = there[n] ?: [remote URLByAppendingPathComponent:n];
      NSURL* lu = here[n];
      const bool in_cloud = there[n] != nil, not_here_yet = [pending containsObject:n];
      NSDate* r = in_cloud && !not_here_yet ? ModDate(ru) : nil;
      NSDate* l = lu ? ModDate(lu) : nil;
      if (in_cloud && not_here_yet && lu) {  // both have it, the cloud's not downloaded: next pass
        waiting++;
        continue;
      }
      const double diff = (r && l) ? [l timeIntervalSinceDate:r] : 0;
      if (in_cloud && (!lu || diff < -2)) {  // down (downloading first when needed), into games/
        NSURL* dest = lu && [lu.URLByDeletingLastPathComponent.path isEqualToString:local.path]
                          ? lu
                          : [local URLByAppendingPathComponent:n];
        __block bool ok = false;
        [co coordinateReadingItemAtURL:ru options:0 error:nil byAccessor:^(NSURL* u) {
          NSData* d = [NSData dataWithContentsOfURL:u];
          NSURL* tmp = [local URLByAppendingPathComponent:[@".sync-" stringByAppendingString:n]];
          if (d && [d writeToURL:tmp atomically:NO]) {
            SetModDate(tmp, ModDate(u));
            ok = rename(tmp.path.fileSystemRepresentation, dest.path.fileSystemRepresentation) == 0;
          }
        }];
        ok ? down++ : failed++;
      } else if (lu && (!in_cloud || diff > 2) && [now timeIntervalSinceDate:l] > 3) {  // up
        __block bool ok = false;
        NSData* d = [NSData dataWithContentsOfURL:lu];
        if (d)
          [co coordinateWritingItemAtURL:ru options:NSFileCoordinatorWritingForReplacing error:nil
                              byAccessor:^(NSURL* u) {
                                ok = [d writeToURL:u atomically:YES];
                                if (ok) SetModDate(u, l);
                              }];
        ok ? up++ : failed++;
      }
    }
    // What the folder holds now, and what of it iCloud does not have yet.
    int in_folder = 0, stuck = 0;
    std::string why;
    for (NSURL* u in Files(remote, [NSMutableSet new]).allValues) {
      in_folder++;
      if (NotUploaded(u, &why)) stuck++;
    }
    if (access) [root stopAccessingSecurityScopedResource];
    NSDateFormatter* f = [NSDateFormatter new];
    f.dateFormat = @"HH:mm";
    char b[400];
    std::snprintf(b, sizeof b, "%s: %d save(s) in the folder, %d here; %d sent, %d received%s%s%s",
                  [f stringFromDate:now].UTF8String, in_folder, int(here.count), up, down,
                  failed ? (", " + std::to_string(failed) + " failed").c_str() : "",
                  waiting ? (", " + std::to_string(waiting) + " still downloading").c_str() : "",
                  stuck ? ("\n⚠️ " + std::to_string(stuck) + " not in iCloud yet" + (why.empty() ? "" : ": " + why) +
                           " (other devices don't see them)").c_str()
                        : "");
    static int last_stuck = -1;
    if (up || down || failed || stuck != last_stuck) std::fprintf(stderr, "h4: save sync %s\n", b);
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

void PassLoop() {
  std::thread([] {
    for (unsigned n = 0;; n++) {
      RemotePass(n % 3 == 0);
      for (int i = 0; i < 200 && !g_wake.exchange(false); i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }).detach();
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

void ShowSaveSyncSheet(void* source_view, bool ukrainian) {
  UIView* source = (__bridge UIView*)source_view;
  const std::string folder = FolderName();
  std::string status;
  {
    std::lock_guard<std::mutex> lock(g_state);
    status = g_status;
  }
  NSString* msg;
  if (folder.empty())
    msg = ukrainian ? @"Відкрийте iCloud Drive і натисніть «Відкрити»: гра створить там папку ReHOMM4 "
                      @"і копіюватиме збереження туди й звідти — на всіх ваших пристроях з тим самим Apple ID."
                    : @"Open iCloud Drive and tap Open: the game makes a ReHOMM4 folder there and copies "
                      @"saved games there and back, on each of your devices with this Apple ID.";
  else
    msg = [NSString stringWithFormat:@"%@: %s\n%s", ukrainian ? @"Папка" : @"Folder", folder.c_str(),
                                     status.c_str()];
  UIAlertController* sheet =
      [UIAlertController alertControllerWithTitle:ukrainian ? @"Збереження в iCloud" : @"Saves in iCloud"
                                          message:msg
                                   preferredStyle:UIAlertControllerStyleActionSheet];
  UIViewController* top = source.window.rootViewController;
  while (top.presentedViewController) top = top.presentedViewController;
  [sheet addAction:[UIAlertAction
                       actionWithTitle:folder.empty() ? (ukrainian ? @"Обрати папку…" : @"Choose a folder…")
                                                      : (ukrainian ? @"Обрати іншу папку…" : @"Choose another folder…")
                                 style:UIAlertActionStyleDefault
                               handler:^(UIAlertAction*) {
                                 static H4SyncPicker* del = [H4SyncPicker new];
                                 UIDocumentPickerViewController* p = [[UIDocumentPickerViewController alloc]
                                     initForOpeningContentTypes:@[ UTTypeFolder ]];
                                 p.delegate = del;
                                 // Opens in iCloud Drive (its file provider's root).
                                 p.directoryURL = [NSFileManager.defaultManager URLForUbiquityContainerIdentifier:nil]
                                                      .URLByDeletingLastPathComponent;
                                 [top presentViewController:p animated:YES completion:nil];
                               }]];
  if (!folder.empty()) {
    [sheet addAction:[UIAlertAction actionWithTitle:ukrainian ? @"Синхронізувати зараз" : @"Sync now"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction*) { SyncSoon(); }]];
    [sheet addAction:[UIAlertAction actionWithTitle:ukrainian ? @"Припинити синхронізацію" : @"Stop syncing"
                                              style:UIAlertActionStyleDestructive
                                            handler:^(UIAlertAction*) { StopSync(); }]];
  }
  [sheet addAction:[UIAlertAction actionWithTitle:ukrainian ? @"Скасувати" : @"Cancel"
                                            style:UIAlertActionStyleCancel
                                          handler:nil]];
  sheet.popoverPresentationController.sourceView = source;
  sheet.popoverPresentationController.sourceRect = source.bounds;
  [top presentViewController:sheet animated:YES completion:nil];
}

}  // namespace h4

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

// A panel over the game (ShowMacChoice; the game's frames go on), its buttons by what is set.
void ShowSaveSyncSheet(void* source_view, bool ukrainian) {
  NSView* source = (__bridge NSView*)source_view;
  NSWindow* win = source.window;
  if (!win) return;
  const std::string folder = FolderName();
  std::string status;
  {
    std::lock_guard<std::mutex> lock(g_state);
    status = g_status;
  }
  std::string text;
  if (folder.empty())
    text = MacICloudDrive()
        ? (ukrainian ? "Синхронізацію вимкнено. Увімкніть її, і гра копіюватиме збереження в iCloud Drive/ReHOMM4 "
                       "і звідти — на всіх ваших пристроях з тим самим Apple ID."
                     : "Syncing is off. Turn it on and the game copies saved games to iCloud Drive/ReHOMM4 and "
                       "back, on each of your devices with this Apple ID.")
        : (ukrainian ? "iCloud Drive на цьому Маці вимкнено (Системні параметри → Apple ID → iCloud)."
                     : "iCloud Drive is off on this Mac (System Settings → Apple ID → iCloud).");
  else
    text = std::string(ukrainian ? "Папка" : "Folder") + ": " + folder + "\n" + status;
  enum Action { kNow, kOn, kOther, kStop, kCancel };
  std::vector<Action> acts;
  std::vector<std::string> titles;
  auto add = [&](const char* t, Action act) {
    titles.push_back(t);
    acts.push_back(act);
  };
  if (!folder.empty()) add(ukrainian ? "Синхронізувати зараз" : "Sync now", kNow);
  else if (MacICloudDrive()) add(ukrainian ? "Увімкнути" : "Turn on", kOn);
  add(ukrainian ? "Обрати іншу папку…" : "Choose another folder…", kOther);
  if (!folder.empty()) add(ukrainian ? "Припинити синхронізацію" : "Stop syncing", kStop);
  add(ukrainian ? "Скасувати" : "Cancel", kCancel);
  ShowMacChoice(source_view, ukrainian ? "Збереження в iCloud" : "Saves in iCloud", text, titles,
                [acts, win, ukrainian](int r) {
              const Action act = r >= 0 && size_t(r) < acts.size() ? acts[size_t(r)] : kCancel;
              if (act == kNow) SyncSoon();
              if (act == kStop) StopSync();
              if (act == kOn) {
                std::remove(OffPath().c_str());
                std::remove(BookmarkPath().c_str());
                {
                  std::lock_guard<std::mutex> lock(g_state);
                  g_remote = MacICloudDrive();
                }
                SyncSoon();
              }
              if (act == kOther) {
                // A sheet (the system's folder picker); ReHOMM4 is made inside what is picked (or it is ReHOMM4).
                NSOpenPanel* p = [NSOpenPanel openPanel];
                p.canChooseFiles = NO;
                p.canChooseDirectories = YES;
                p.canCreateDirectories = YES;
                p.prompt = ukrainian ? @"Обрати" : @"Choose";
                p.message = ukrainian ? @"Папка, у якій буде (або вже є) ReHOMM4"
                                      : @"The folder to hold ReHOMM4 (or ReHOMM4 itself)";
                p.directoryURL = MacICloudDrive();
                [p beginSheetModalForWindow:win
                          completionHandler:^(NSModalResponse pr) {
                            if (pr == NSModalResponseOK) UsePicked(p.URL);
                          }];
              }
            });
}

}  // namespace h4

#endif
